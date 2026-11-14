#!/usr/bin/env python3
"""
mkhunk.py <objcopy> <elf2hunk> <in.elf> <out>

Turn a partially linked ELF into an AmigaOS hunk file: drop the unwind
tables, run elf2hunk, and fail if the result needs a HUNK_RELRELOC32 (a
PC-relative reference between hunks), which Kickstart 2.x cannot load.
"""
import os
import struct
import subprocess
import sys

HUNK_CODE, HUNK_DATA, HUNK_BSS = 0x3e9, 0x3ea, 0x3eb
HUNK_RELOC32, HUNK_SYMBOL, HUNK_END = 0x3ec, 0x3f0, 0x3f2
HUNK_HEADER, HUNK_RELRELOC32 = 0x3f3, 0x3fd


def check(path):
    with open(path, 'rb') as f:
        data = f.read()
    pos = 0

    def word():
        nonlocal pos
        pos += 4
        return struct.unpack('>I', data[pos - 4:pos])[0]

    if word() != HUNK_HEADER:
        sys.exit(f'{path}: not a hunk file')
    while (n := word()):                        # resident library names
        pos += 4 * n
    word()
    first, last = word(), word()
    pos += 4 * (last - first + 1)
    while pos < len(data):
        kind = word() & 0x3fffffff
        if kind in (HUNK_CODE, HUNK_DATA):
            n = word()
            pos += 4 * n
        elif kind == HUNK_BSS:
            word()
        elif kind in (HUNK_RELOC32, HUNK_SYMBOL):
            while (n := word()):
                pos += 4 * (n + 1)
        elif kind == HUNK_RELRELOC32:
            sys.exit(f'{path}: has a HUNK_RELRELOC32')
        elif kind != HUNK_END:
            sys.exit(f'{path}: unexpected hunk type {kind:#x}')


def main(objcopy, elf2hunk, src, out):
    tmp = out + '.tmp'
    try:
        subprocess.run([objcopy, '--remove-section=.eh_frame', src, tmp],
                       check=True)
        subprocess.run([elf2hunk, tmp, out], check=True,
                       stdout=subprocess.DEVNULL)
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)
    check(out)


if __name__ == '__main__':
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    main(*sys.argv[1:])
