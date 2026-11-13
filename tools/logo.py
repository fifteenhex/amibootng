#!/usr/bin/env python3
"""
logo.py header <logo.ppm> <out.h>

Convert a kernel boot logo (an ASCII PPM from drivers/video/logo/) to
pen-indexed pixels and a 4-bit palette for the splash screen.  Pens 0, 1
and 2 are black, white and light grey.
"""
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


if __name__ == '__main__':
    if len(sys.argv) == 4 and sys.argv[1] == 'header':
        header(read_ppm(sys.argv[2]), sys.argv[3])
    else:
        sys.exit(__doc__)
