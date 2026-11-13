#!/usr/bin/env python3
"""
Convert a kernel boot logo (an ASCII PPM from drivers/video/logo/):

  logo.py header <logo.ppm> <out.h>
      pen-indexed pixels and a 4-bit palette for the splash screen; pens
      0, 1 and 2 are black, white and light grey
  logo.py icon <logo.ppm> <out.info> <default tool> [tooltype...]
      a Workbench project icon of the logo at half size
"""
import struct
import sys


def read_ppm(path):
    """rows of (r, g, b) pixels scaled to 0..255"""
    toks = []
    with open(path) as f:
        for line in f:
            toks += line.split('#', 1)[0].split()
    if toks[0] != 'P3':
        sys.exit(f'{path}: not an ASCII PPM')
    w, h, maxval = map(int, toks[1:4])
    v = [(int(t) * 255 + maxval // 2) // maxval
         for t in toks[4:4 + w * h * 3]]
    px = [tuple(v[i:i + 3]) for i in range(0, len(v), 3)]
    return [px[y * w:(y + 1) * w] for y in range(h)]


def header(rows, out):
    pal = [(0, 0, 0), (255, 255, 255), (170, 170, 170)]
    pal += [c for c in dict.fromkeys(c for row in rows for c in row)
            if c not in pal]
    if len(pal) > 32:
        sys.exit(f'{len(pal)} colours, a lowres screen has 32')
    with open(out, 'w') as f:
        f.write(f'#define LOGO_WIDTH {len(rows[0])}\n'
                f'#define LOGO_HEIGHT {len(rows)}\n'
                f'#define LOGO_COLOURS {len(pal)}\n'
                f'#define LOGO_DEPTH {max(1, (len(pal) - 1).bit_length())}\n'
                'static const unsigned char logo_palette[][3] = {\n')
        for c in pal:
            f.write('    { %d, %d, %d },\n' % tuple((x * 15 + 127) // 255
                                                    for x in c))
        f.write('};\nstatic const unsigned char '
                'logo_pixels[LOGO_HEIGHT][LOGO_WIDTH] = {\n')
        for row in rows:
            f.write('    { %s },\n' % ','.join(str(pal.index(c)) for c in row))
        f.write('};\n')


WB_PALETTE = [(0xaa, 0xaa, 0xaa), (0, 0, 0), (0xff, 0xff, 0xff),
              (0x66, 0x88, 0xbb)]


def icon_pens(rows):
    """the logo in Workbench pens, its black background made pen 0"""
    def nearest(c):
        return min(range(4), key=lambda i: sum((a - b) ** 2 for a, b in
                                               zip(c, WB_PALETTE[i])))
    pens = [[nearest(c) for c in row] for row in rows]
    h, w = len(pens), len(pens[0])
    todo = [(x, y) for x in range(w) for y in (0, h - 1)] + \
           [(x, y) for y in range(h) for x in (0, w - 1)]
    while todo:
        x, y = todo.pop()
        if 0 <= x < w and 0 <= y < h and pens[y][x] == 1:
            pens[y][x] = 0
            todo += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    return [row[::2] for row in pens[::2]]


def cstring(s):
    """a length-prefixed, NUL-terminated string as in a .info file"""
    b = s.encode('latin-1') + b'\0'
    return struct.pack('>I', len(b)) + b


def icon(rows, out, tool, tooltypes):
    pens = icon_pens(rows)
    h, w = len(pens), len(pens[0])
    # DiskObject: magic, version, embedded Gadget, WBPROJECT
    data = struct.pack('>HH', 0xe310, 1)
    data += struct.pack('>IhhhhHHHIIIIIHI', 0, 0, 0, w, h, 4, 0, 1,
                        1, 0, 0, 0, 0, 0, 0)
    data += struct.pack('>BBIIIIIII', 4, 0, 1, bool(tooltypes),
                        0x80000000, 0x80000000, 0, 0, 8192)
    # the Image: two planes, rows padded to words
    data += struct.pack('>hhhhhIBBI', 0, 0, w, h, 2, 1, 3, 0, 0)
    for plane in (0, 1):
        for row in pens:
            row = row + [0] * (-w % 16)
            for x in range(0, len(row), 16):
                bits = sum(1 << (15 - i) for i in range(16)
                           if row[x + i] >> plane & 1)
                data += struct.pack('>H', bits)
    data += cstring(tool)
    if tooltypes:
        data += struct.pack('>I', (len(tooltypes) + 1) * 4)
        data += b''.join(cstring(t) for t in tooltypes)
    with open(out, 'wb') as f:
        f.write(data)


if __name__ == '__main__':
    if len(sys.argv) == 4 and sys.argv[1] == 'header':
        header(read_ppm(sys.argv[2]), sys.argv[3])
    elif len(sys.argv) >= 5 and sys.argv[1] == 'icon':
        icon(read_ppm(sys.argv[2]), sys.argv[3], sys.argv[4], sys.argv[5:])
    else:
        sys.exit(__doc__)
