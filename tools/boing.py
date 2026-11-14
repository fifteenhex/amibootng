#!/usr/bin/env python3
"""
boing.py <out.ppm>

The Boing ball, as an 80x80 ASCII PPM in the style of the kernel's logo:
red and white checks, eight rows by sixteen columns, tilted, with a darker
rim, on black.
"""
import math
import sys

SIZE = 80
RADIUS = 37.5
TILT = math.radians(-17)        # the axis leans right...
PITCH = math.radians(12)        # ...and towards the viewer
SPIN = math.radians(11)
RIM = 0.92

RED, DARK_RED = (255, 0, 0), (136, 0, 0)
WHITE, GREY = (255, 255, 255), (170, 170, 170)
BLACK = (0, 0, 0)


def pixel(px, py):
    x = (px + 0.5 - SIZE / 2) / RADIUS
    y = (py + 0.5 - SIZE / 2) / RADIUS
    r2 = x * x + y * y
    if r2 > 1:
        return BLACK
    z = math.sqrt(1 - r2)
    # into the ball's frame: undo the lean, then the pitch
    x, y = (x * math.cos(TILT) - y * math.sin(TILT),
            x * math.sin(TILT) + y * math.cos(TILT))
    y, z = (y * math.cos(PITCH) - z * math.sin(PITCH),
            y * math.sin(PITCH) + z * math.cos(PITCH))
    lon = math.atan2(x, z) + SPIN
    lat = math.asin(max(-1.0, min(1.0, y)))
    row = int((lat + math.pi / 2) / math.pi * 8)
    col = int((lon + math.pi) / (2 * math.pi) * 16)
    red = (row + col) % 2
    if r2 > RIM * RIM:
        return DARK_RED if red else GREY
    return RED if red else WHITE


def main(out):
    with open(out, 'w') as f:
        f.write(f'P3\n{SIZE} {SIZE}\n255\n')
        for py in range(SIZE):
            f.write(' '.join('%d %d %d' % pixel(px, py)
                             for px in range(SIZE)) + '\n')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
