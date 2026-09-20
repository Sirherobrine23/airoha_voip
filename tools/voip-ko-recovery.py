#!/usr/bin/env python3
"""Inventory and compare relocatable vendor VoIP kernel modules.

The tool intentionally uses only the Python standard library.  It preserves
the evidence useful for clean-room recovery (ELF metadata, sections, symbols,
relocations, module information and per-function hashes) without copying the
vendor module into the source tree.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct


SHT_SYMTAB = 2
SHT_RELA = 4
SHT_NOBITS = 8
SHT_REL = 9
STT_FUNC = 2


def sha256(data):
    return hashlib.sha256(data).hexdigest()


class ELF:
    def __init__(self, path):
        self.path = Path(path)
        self.raw = self.path.read_bytes()
        if self.raw[:4] != b"\x7fELF":
            raise ValueError(f"{path}: not an ELF file")
        if self.raw[4] not in (1, 2) or self.raw[5] not in (1, 2):
            raise ValueError(f"{path}: unsupported ELF class or byte order")
        self.bits = 32 if self.raw[4] == 1 else 64
        self.byte_order = "little" if self.raw[5] == 1 else "big"
        self.prefix = "<" if self.byte_order == "little" else ">"
        self._read_header()
        self._read_sections()
        self._read_symbols()
        self._read_relocations()

    def _unpack(self, fmt, offset, data=None):
        return struct.unpack_from(self.prefix + fmt, self.raw if data is None else data, offset)

    @staticmethod
    def _cstring(data, offset):
        if offset >= len(data):
            return ""
        end = data.find(b"\0", offset)
        if end < 0:
            end = len(data)
        return data[offset:end].decode("utf-8", errors="replace")

    def _read_header(self):
        if self.bits == 32:
            values = self._unpack("HHIIIIIHHHHHH", 16)
            (self.etype, self.machine, self.version, self.entry, self.phoff,
             self.shoff, self.flags, self.ehsize, self.phentsize, self.phnum,
             self.shentsize, self.shnum, self.shstrndx) = values
        else:
            values = self._unpack("HHIQQQIHHHHHH", 16)
            (self.etype, self.machine, self.version, self.entry, self.phoff,
             self.shoff, self.flags, self.ehsize, self.phentsize, self.phnum,
             self.shentsize, self.shnum, self.shstrndx) = values

    def _read_sections(self):
        headers = []
        fmt = "IIIIIIIIII" if self.bits == 32 else "IIQQQQIIQQ"
        for index in range(self.shnum):
            h = self._unpack(fmt, self.shoff + index * self.shentsize)
            headers.append(h)
        shstr = headers[self.shstrndx]
        strings = self.raw[shstr[4]:shstr[4] + shstr[5]]
        self.sections = []
        for index, h in enumerate(headers):
            data = b"" if h[1] == SHT_NOBITS else self.raw[h[4]:h[4] + h[5]]
            self.sections.append({
                "index": index, "name": self._cstring(strings, h[0]),
                "type": h[1], "flags": h[2], "address": h[3],
                "offset": h[4], "size": h[5], "link": h[6], "info": h[7],
                "alignment": h[8], "entry_size": h[9], "data": data,
            })
        self.by_name = {section["name"]: section for section in self.sections}

    def _read_symbols(self):
        self.symbols = []
        symtab = next((s for s in self.sections if s["type"] == SHT_SYMTAB), None)
        if not symtab or not symtab["entry_size"]:
            return
        strings = self.sections[symtab["link"]]["data"]
        for offset in range(0, symtab["size"], symtab["entry_size"]):
            if self.bits == 32:
                name, value, size, info, other, section = self._unpack(
                    "IIIBBH", offset, symtab["data"])
            else:
                name, info, other, section, value, size = self._unpack(
                    "IBBHQQ", offset, symtab["data"])
            self.symbols.append({
                "name": self._cstring(strings, name), "value": value,
                "size": size, "type": info & 15, "bind": info >> 4,
                "visibility": other, "section": section,
            })

    def _read_relocations(self):
        self.relocations = []
        for section in self.sections:
            if section["type"] not in (SHT_REL, SHT_RELA) or not section["entry_size"]:
                continue
            for offset in range(0, section["size"], section["entry_size"]):
                if self.bits == 32:
                    address, info = self._unpack("II", offset, section["data"])
                    symbol, kind = info >> 8, info & 0xff
                else:
                    address, info = self._unpack("QQ", offset, section["data"])
                    symbol, kind = info >> 32, info & 0xffffffff
                item = {
                    "relocation_section": section["name"],
                    "target_section": self.sections[section["info"]]["name"],
                    "offset": address, "type": kind, "symbol_index": symbol,
                    "symbol": self.symbols[symbol]["name"] if symbol < len(self.symbols) else "",
                }
                self.relocations.append(item)

    def function_bytes(self, symbol):
        if symbol["section"] <= 0 or symbol["section"] >= len(self.sections):
            return b""
        section = self.sections[symbol["section"]]
        start = symbol["value"] - section["address"]
        return section["data"][start:start + symbol["size"]]

    def inventory(self):
        functions = []
        imports = []
        for symbol in self.symbols:
            if symbol["section"] == 0 and symbol["name"]:
                imports.append(symbol["name"])
            if symbol["type"] == STT_FUNC and symbol["section"] != 0:
                data = self.function_bytes(symbol)
                functions.append({
                    "name": symbol["name"], "section": self.sections[symbol["section"]]["name"],
                    "offset": symbol["value"], "size": symbol["size"],
                    "sha256": sha256(data),
                })
        modinfo = {}
        if ".modinfo" in self.by_name:
            for entry in self.by_name[".modinfo"]["data"].split(b"\0"):
                if b"=" in entry:
                    key, value = entry.decode("utf-8", errors="replace").split("=", 1)
                    modinfo.setdefault(key, []).append(value)
        exports = []
        if "__ksymtab_strings" in self.by_name:
            exports = [x.decode("utf-8", errors="replace") for x in
                       self.by_name["__ksymtab_strings"]["data"].split(b"\0") if x]
        return {
            "path": str(self.path), "size": len(self.raw), "sha256": sha256(self.raw),
            "elf": {"bits": self.bits, "byte_order": self.byte_order,
                    "type": self.etype, "machine": self.machine, "flags": self.flags},
            "modinfo": modinfo,
            "debug_sections": [s["name"] for s in self.sections
                               if s["name"].startswith((".debug", ".zdebug", ".stab"))],
            "sections": [{k: s[k] for k in ("index", "name", "type", "flags", "size",
                                               "alignment", "entry_size")}
                         for s in self.sections],
            "functions": sorted(functions, key=lambda x: (x["section"], x["offset"], x["name"])),
            "imports": sorted(set(imports)), "exports": sorted(set(exports)),
            "relocations": self.relocations,
        }


def inventory_command(args):
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    modules = {}
    for module_path in args.modules:
        elf = ELF(module_path)
        data = elf.inventory()
        name = Path(module_path).name
        if name in modules:
            name = f"{Path(module_path).parent.name}-{name}"
        modules[name] = data
        (output / f"{name}.json").write_text(json.dumps(data, indent=2) + "\n")
        print(f"{name}: {len(data['functions'])} functions, "
              f"{len(data['imports'])} imports, {len(data['exports'])} exports, "
              f"{len(data['relocations'])} relocations")
    common = {}
    for name, module in modules.items():
        for function in module["functions"]:
            common.setdefault(function["name"], []).append(name)
    report = {
        "format": 1, "modules": modules,
        "functions_shared_by_modules": {
            name: owners for name, owners in sorted(common.items()) if len(owners) > 1
        },
    }
    (output / "reference.json").write_text(json.dumps(report, indent=2) + "\n")


def compare_command(args):
    reference = json.loads(Path(args.reference).read_text())
    candidate = ELF(args.module).inventory()
    expected = reference["modules"][args.name]
    ok = True
    for key in ("size", "sha256", "elf", "modinfo", "exports", "imports"):
        same = expected[key] == candidate[key]
        print(f"{key}: {'EXACT' if same else 'DIFFERENT'}")
        ok &= same
    expected_functions = {f["name"]: f for f in expected["functions"]}
    candidate_functions = {f["name"]: f for f in candidate["functions"]}
    for name in sorted(set(expected_functions) | set(candidate_functions)):
        if name not in candidate_functions:
            print(f"function {name}: MISSING")
            ok = False
        elif name not in expected_functions:
            print(f"function {name}: EXTRA")
            ok = False
        elif expected_functions[name]["sha256"] != candidate_functions[name]["sha256"]:
            print(f"function {name}: DIFFERENT "
                  f"({expected_functions[name]['size']} -> {candidate_functions[name]['size']} bytes)")
            ok = False
    return 0 if ok else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    inv = sub.add_parser("inventory", help="write a pinned recovery manifest")
    inv.add_argument("output")
    inv.add_argument("modules", nargs="+")
    inv.set_defaults(func=inventory_command)
    comp = sub.add_parser("compare", help="compare a rebuilt module to a manifest entry")
    comp.add_argument("reference")
    comp.add_argument("name")
    comp.add_argument("module")
    comp.set_defaults(func=compare_command)
    args = parser.parse_args()
    result = args.func(args)
    return result if isinstance(result, int) else 0


if __name__ == "__main__":
    raise SystemExit(main())
