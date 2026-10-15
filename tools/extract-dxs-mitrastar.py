#!/usr/bin/env python3
"""Recover the known MitraStar DXS PRAM image from an OEM slic3_intel module.

This is byte extraction only; the module is never executed or loaded.
"""
import argparse
import hashlib
from pathlib import Path

SIZE = 20168
DIGEST = '779afa0255e079319ccb264acba714816505c7bcda60c73b967b0c2259dae824'
HEADER = bytes.fromhex('d000000100004e9c4458534602010300')


def extract(data):
    cursor = 0
    while True:
        offset = data.find(HEADER, cursor)
        if offset < 0:
            raise ValueError('Known MitraStar DXS firmware not found or checksum differs')
        blob = data[offset:offset + SIZE]
        if hashlib.sha256(blob).hexdigest() == DIGEST:
            return offset, blob
        cursor = offset + 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    offset, blob = extract(args.module.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(blob)
    print(f'{args.output}: {len(blob)} bytes, module offset {offset}, sha256 {DIGEST}')


if __name__ == '__main__':
    main()
