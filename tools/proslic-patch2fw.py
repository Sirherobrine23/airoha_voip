#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""
Convert Silicon Labs / Skyworks ProSLIC patch sources into firmware blobs.

The ProSLIC API ships its DSP patches as C files that get compiled into
the driver. That means every supported chipset, revision and BOM variant
is linked in whether the board uses it or not, and adding a variant
means rebuilding the module. This turns each patch into a blob that
request_firmware() can pull at probe time.

The container keeps the field order of the draft header in the
proslic_drivers2 branch (serial, then the four sizes, then patchData,
psRamData, psRamAddr, patchEntries) and adds what a loader actually
needs to be safe: a magic, a version, an explicit little-endian
encoding, a CRC32 and the chipset/revision/BOM the patch belongs to.

Input files may come from any of the ProSLIC API layouts:

    static const uInt32 patchData [] = { ... };     (upstream API)
    static const u32 patchData[] = { ... };         (kernel-style fork)
    const proslicPatch si3219xPatchRevALCQC = {...} (multi-BOM)
    struct proslicPatch si3219_a = { .patchData = ... }  (designated)

Usage:
    proslic-patch2fw.py -o firmware/proslic patch_files/*.c
    proslic-patch2fw.py --dump firmware/proslic/si3219x_a_lcqc.fw
"""

import argparse
import binascii
import os
import re
import struct
import sys

MAGIC = b"PROSLICP"
VERSION = 2
HDR_SIZE = 56

# Limits the API itself enforces (proslic.h).
PATCH_MAX_SIZE = 1024
PATCH_MAX_SUPPORT_RAM = 128
PATCH_NUM_LOW_ENTRIES = 8
PATCH_NUM_ENTRIES = 16  # 8 low + 8 high


class PatchError(Exception):
    pass


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    # Drop preprocessor lines: the multi-BOM #ifdef wraps declarations,
    # not values, so removing them leaves both names visible.
    text = re.sub(r"^\s*#.*$", "", text, flags=re.M)
    return text


def parse_arrays(text):
    """Return {name: [int, ...]} for every static array in the file."""
    arrays = {}
    pattern = re.compile(
        r"(?:static\s+)?const\s+(?:uInt8|uInt16|uInt32|u8|u16|u32|ramData)\s+"
        r"(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;",
        re.S,
    )
    for name, body in pattern.findall(text):
        values = []
        for token in re.split(r"[,\s]+", body.strip()):
            if not token:
                continue
            token = token.rstrip("Ll")
            if not token:
                continue
            try:
                values.append(int(token, 0))
            except ValueError:
                raise PatchError("unparsable value %r in array %s"
                                 % (token, name))
        arrays[name] = values
    return arrays


def find_patch_bodies(text):
    """
    Yield (names, body) for every proslicPatch initialiser.

    Multi-BOM files declare the same object twice around a single body:

        #ifdef SIVOICE_MULTI_BOM_SUPPORT
        const proslicPatch si3219xPatchRevALCQC = {
        #else
        const proslicPatch RevAPatch = {
        #endif
            patchData, patchEntries, ...
        };

    Both names refer to the same patch, so they are collected together
    and the body is taken from the last declaration.
    """
    decl = re.compile(r"(?:const\s+|struct\s+)*proslicPatch\s+(\w+)\s*=\s*\{")
    matches = list(decl.finditer(text))
    i = 0

    while i < len(matches):
        names = [matches[i].group(1)]

        # Consecutive declarations separated only by whitespace share a body.
        while (i + 1 < len(matches) and
               not text[matches[i].end():matches[i + 1].start()].strip()):
            i += 1
            names.append(matches[i].group(1))

        start = matches[i].end()
        depth = 1
        pos = start
        while pos < len(text) and depth:
            if text[pos] == "{":
                depth += 1
            elif text[pos] == "}":
                depth -= 1
            pos += 1

        yield names, text[start:pos - 1]
        i += 1


def parse_patch_objects(text, arrays):
    """
    Return [(names, fields)] with the five members of proslicPatch
    resolved to actual lists / ints.

    Two initialiser styles exist. Positional, from the upstream API:

        const proslicPatch RevAPatch = {
            patchData, patchEntries, 0x05252017L,
            patchSupportAddr, patchSupportData
        };

    and designated, from the kernel-style fork:

        struct proslicPatch si3219_a = {
            .patchSerial = 0x05252017L, .patchData = patchData, ...
        };
    """
    out = []

    for names, body in find_patch_bodies(text):
        fields = {}

        if re.search(r"\.\s*patch(Data|Serial)", body):
            for key, value in re.findall(r"\.\s*(\w+)\s*=\s*([^,}]+)", body):
                value = value.strip().rstrip("Ll")
                if value in arrays:
                    fields[key] = arrays[value]
                elif value.startswith("ARRAY_SIZEOF"):
                    continue          # recomputed below
                else:
                    try:
                        fields[key] = int(value, 0)
                    except ValueError:
                        pass
        else:
            items = [t.strip().rstrip("Ll")
                     for t in body.split(",") if t.strip()]
            if len(items) < 5:
                raise PatchError("%s: expected 5 initialisers, got %d"
                                 % (names[0], len(items)))
            order = ["patchData", "patchEntries", "patchSerial",
                     "psRamAddr", "psRamData"]
            for key, value in zip(order, items):
                if value in arrays:
                    fields[key] = arrays[value]
                else:
                    try:
                        fields[key] = int(value, 0)
                    except ValueError:
                        raise PatchError("%s: cannot resolve %r"
                                         % (names[0], value))

        missing = [k for k in ("patchData", "patchEntries", "patchSerial",
                               "psRamAddr", "psRamData") if k not in fields]
        if missing:
            raise PatchError("%s: missing %s"
                             % (names[0], ", ".join(missing)))

        out.append((names, fields))

    return out


def classify(source_name, object_name):
    """Work out chipset, revision and BOM variant."""
    text = "%s %s" % (source_name, object_name)

    chipset = ""
    m = re.search(r"si\s*(32[0-9]{2})[x_]?", text, re.I)
    if m:
        chipset = "si%sx" % m.group(1)

    revision = ""
    m = re.search(r"[Rr]ev([A-D])", object_name)
    if m:
        revision = m.group(1)
    else:
        m = re.search(r"_patch_([A-D])_|_([A-D])(?:_|\.)", source_name)
        if m:
            revision = (m.group(1) or m.group(2)).upper()

    # BOM from the object name first: one source file can hold several
    # patch objects, so the file name is the weaker signal.
    table = (
        ("TssIso", "TSS_ISO"), ("TSS_ISO", "TSS_ISO"),
        ("Tss", "TSS"), ("TSS", "TSS"),
        ("LCQC", "LCQC"), ("Lcqc", "LCQC"),
        ("Flbk", "FB"), ("Bkbt", "BB"), ("Pbb", "PBB"),
    )
    bom = ""
    for key, name in table:
        if key in object_name:
            bom = name
            break
    if not bom:
        for key, name in (("_TSS_ISO", "TSS_ISO"), ("_TSS", "TSS"),
                          ("_FB", "FB"), ("_BB", "BB")):
            if key in source_name:
                bom = name
                break

    return chipset, revision, bom


def build_blob(fields, chipset, revision, bom):
    data = list(fields["patchData"])
    entries = list(fields["patchEntries"])
    ps_addr = list(fields["psRamAddr"])
    ps_data = list(fields["psRamData"])
    serial = int(fields["patchSerial"])

    # The API walks patchData and psRamAddr until a zero, so the
    # terminator has to survive the conversion.
    if not data or data[-1] != 0:
        data.append(0)
    if not ps_addr or ps_addr[-1] != 0:
        ps_addr.append(0)
        ps_data.append(0)

    if len(ps_data) < len(ps_addr):
        ps_data += [0] * (len(ps_addr) - len(ps_data))

    if len(data) > PATCH_MAX_SIZE:
        raise PatchError("patchData is %d entries, API limit is %d"
                         % (len(data), PATCH_MAX_SIZE))
    if len(ps_addr) > PATCH_MAX_SUPPORT_RAM:
        raise PatchError("support RAM is %d entries, API limit is %d"
                         % (len(ps_addr), PATCH_MAX_SUPPORT_RAM))
    # Some revisions (Si3217x rev B) ship only the 8 low jump-table
    # entries. The API indexes patchEntries[PATCH_NUM_LOW_ENTRIES]
    # unconditionally, so the array is padded to 16 here and the real
    # count is kept in the header for anyone who needs to know.
    n_entries = len(entries)
    if n_entries not in (PATCH_NUM_LOW_ENTRIES, PATCH_NUM_ENTRIES):
        raise PatchError("patchEntries is %d, expected %d or %d"
                         % (n_entries, PATCH_NUM_LOW_ENTRIES,
                            PATCH_NUM_ENTRIES))
    entries = entries + [0] * (PATCH_NUM_ENTRIES - n_entries)

    payload = b""
    payload += struct.pack("<%dI" % len(data), *data)
    payload += struct.pack("<%dI" % len(ps_data), *ps_data)
    payload += struct.pack("<%dH" % len(ps_addr), *ps_addr)
    payload += struct.pack("<%dH" % PATCH_NUM_ENTRIES, *entries)

    crc = binascii.crc32(payload) & 0xFFFFFFFF

    header = struct.pack(
        "<8sHHIHHHHI8s4s12sI",
        MAGIC, VERSION, HDR_SIZE, serial,
        len(data), len(ps_data), len(ps_addr), n_entries,
        crc,
        chipset.encode()[:8], revision.encode()[:4], bom.encode()[:12],
        0,
    )
    assert len(header) == HDR_SIZE, len(header)

    return header + payload


def blob_name(chipset, revision, bom, fallback):
    if not chipset:
        return fallback
    parts = [chipset]
    if revision:
        parts.append(revision.lower())
    if bom:
        parts.append(bom.lower())
    return "_".join(parts)


def convert(paths, outdir, verbose=False):
    found = []

    for path in paths:
        try:
            with open(path, "r", errors="replace") as handle:
                text = strip_comments(handle.read())
            arrays = parse_arrays(text)
            objects = parse_patch_objects(text, arrays)
        except PatchError as exc:
            print("skip %s: %s" % (path, exc), file=sys.stderr)
            continue

        if not objects:
            print("skip %s: no proslicPatch object" % path, file=sys.stderr)
            continue

        source = os.path.basename(path)

        for names, fields in objects:
            try:
                chipset, revision, bom = classify(source, " ".join(names))
                blob = build_blob(fields, chipset, revision, bom)
            except PatchError as exc:
                print("skip %s:%s: %s" % (source, names[0], exc),
                      file=sys.stderr)
                continue

            found.append({
                "blob": blob,
                "digest": binascii.crc32(blob[HDR_SIZE:]) & 0xFFFFFFFF,
                "source": source,
                "names": names,
                "chipset": chipset,
                "revision": revision,
                "bom": bom,
                "serial": int(fields["patchSerial"]),
            })

    # Identical payloads appear in several branches, and the kernel-style
    # fork drops the BOM from the object name. Group by content and keep
    # the best-classified identity.
    groups = {}
    for entry in found:
        groups.setdefault(entry["digest"], []).append(entry)

    def rank(entry):
        return (bool(entry["chipset"]), bool(entry["revision"]),
                bool(entry["bom"]))

    os.makedirs(outdir, exist_ok=True)
    written = []
    by_chip_rev = {}

    for digest, entries in groups.items():
        # One payload can legitimately belong to several parts: the
        # Si3219x rev A patch is byte-identical to the Si3218x one --
        # the Silicon Labs header says it was generated from the same
        # .dsp_prom. So the payload is shared but a file is written for
        # every distinct identity, or the driver would not find the
        # name it asks for.
        identities = {(e["chipset"], e["revision"], e["bom"])
                      for e in entries if e["chipset"] and e["revision"]}
        if not identities:
            identities = {(e["chipset"], e["revision"], e["bom"])
                          for e in entries}

        # (chip, rev, "") is just a less specific spelling of
        # (chip, rev, "SOMEBOM") when both are present.
        specific = {(c, r) for c, r, b in identities if b}
        identities = {(c, r, b) for c, r, b in identities
                      if b or (c, r) not in specific}

        aliases = sorted({n for e in entries for n in e["names"]})
        sources = sorted({e["source"] for e in entries})

        for chipset, revision, bom in sorted(identities):
            proto = next(e for e in entries
                         if e["chipset"] == chipset and
                         e["revision"] == revision)
            best = dict(proto, chipset=chipset, revision=revision, bom=bom)
            blob = build_blob_like(best)

            name = blob_name(chipset, revision, bom,
                             os.path.splitext(proto["source"])[0])
            target = os.path.join(outdir, name + ".fw")

            if os.path.exists(target):
                with open(target, "rb") as handle:
                    if handle.read() != blob:
                        target = os.path.join(
                            outdir, "%s_%08x.fw" % (name, best["serial"]))

            with open(target, "wb") as handle:
                handle.write(blob)

            written.append({
                "path": target, "best": best, "aliases": aliases,
                "sources": sources, "size": len(blob),
            })
            by_chip_rev.setdefault((chipset, revision), []).append(target)

        if verbose and len(entries) > 1:
            print("merged %d source copies -> %s"
                  % (len(entries),
                     ", ".join(sorted(blob_name(c, r, b, "?")
                                      for c, r, b in identities))))

    # When a chipset+revision has exactly one patch, also publish it
    # under the plain name so a driver can fall back when it does not
    # know the board's BOM.
    for (chipset, revision), targets in by_chip_rev.items():
        if len(targets) != 1 or not chipset or not revision:
            continue
        plain = os.path.join(outdir, "%s_%s.fw" % (chipset, revision.lower()))
        if os.path.abspath(plain) == os.path.abspath(targets[0]):
            continue
        with open(targets[0], "rb") as src, open(plain, "wb") as dst:
            dst.write(src.read())
        if verbose:
            print("alias  %s -> %s" % (os.path.basename(plain),
                                       os.path.basename(targets[0])))

    write_index(outdir, written)
    return written


def build_blob_like(entry):
    """Rebuild a blob with the group's best classification."""
    header = bytearray(entry["blob"][:HDR_SIZE])
    struct.pack_into("<8s4s12s", header, 28,
                     entry["chipset"].encode()[:8],
                     entry["revision"].encode()[:4],
                     entry["bom"].encode()[:12])
    return bytes(header) + entry["blob"][HDR_SIZE:]


def write_index(outdir, written):
    """A plain-text map from blob back to where it came from."""
    lines = ["# ProSLIC patch blobs",
             "#",
             "# file | chipset | rev | bom | serial | API symbols | sources",
             ""]
    for item in sorted(written, key=lambda w: w["path"]):
        best = item["best"]
        lines.append("%s | %s | %s | %s | 0x%08x | %s | %s" % (
            os.path.basename(item["path"]),
            best["chipset"] or "?", best["revision"] or "?",
            best["bom"] or "-", best["serial"],
            ",".join(item["aliases"]),
            ",".join(item["sources"])))
    with open(os.path.join(outdir, "INDEX"), "w") as handle:
        handle.write("\n".join(lines) + "\n")


def dump(path):
    with open(path, "rb") as handle:
        blob = handle.read()

    if len(blob) < HDR_SIZE or blob[:8] != MAGIC:
        raise SystemExit("%s: not a ProSLIC firmware blob" % path)

    (magic, version, hdr_size, serial, n_data, n_psdata, n_psaddr,
     n_entries, crc, chipset, revision, bom, _) = struct.unpack_from(
        "<8sHHIHHHHI8s4s12sI", blob, 0)

    payload = blob[hdr_size:]
    actual = binascii.crc32(payload) & 0xFFFFFFFF

    print("file        %s" % path)
    print("version     %d" % version)
    print("chipset     %s rev %s bom %s" % (
        chipset.rstrip(b"\0").decode() or "?",
        revision.rstrip(b"\0").decode() or "?",
        bom.rstrip(b"\0").decode() or "-"))
    print("serial      0x%08x" % serial)
    print("patchData   %d words" % n_data)
    print("psRamData   %d words" % n_psdata)
    print("psRamAddr   %d words" % n_psaddr)
    print("entries     %d words (padded to %d on disk)"
          % (n_entries, PATCH_NUM_ENTRIES))
    print("crc32       0x%08x (%s)" % (
        crc, "ok" if crc == actual else "MISMATCH 0x%08x" % actual))

    expect = n_data * 4 + n_psdata * 4 + n_psaddr * 2 + PATCH_NUM_ENTRIES * 2
    if expect != len(payload):
        print("payload     %d bytes, header implies %d -- TRUNCATED"
              % (len(payload), expect))


def main():
    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inputs", nargs="+",
                        help="patch .c files, or .fw files with --dump")
    parser.add_argument("-o", "--outdir", default="firmware/proslic")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--dump", action="store_true",
                        help="describe existing blobs instead of converting")
    args = parser.parse_args()

    if args.dump:
        for path in args.inputs:
            dump(path)
            print()
        return 0

    written = convert(args.inputs, args.outdir, args.verbose)

    for item in sorted(written, key=lambda w: w["path"]):
        best = item["best"]
        print("%-30s %-7s %-3s %-8s 0x%08x %6d B  (%d source%s)" % (
            os.path.basename(item["path"]), best["chipset"] or "?",
            best["revision"] or "?", best["bom"] or "-", best["serial"],
            item["size"], len(item["sources"]),
            "" if len(item["sources"]) == 1 else "s"))

    print("\n%d blob(s) in %s" % (len(written), args.outdir))
    return 0


if __name__ == "__main__":
    sys.exit(main())
