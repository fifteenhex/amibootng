# amibootng

Boots Linux/m68k from AmigaOS: it loads a kernel from an AmigaOS disk and
hands the machine over to it the way QEMU's `-kernel` does.  It needs a
68020 or better and fast RAM, and is meant to run after `SetPatch`, which
sets up the CPU flags it reports to the kernel.  Only the fast RAM region
the kernel is loaded into is passed on.

    amibootng <kernel> [options] [kernel arguments]

Options:

    @<file>          the words in <file>; a word starting with ';' or '#'
                     comments out the rest of its line
    ramdisk=<file>   initrd
    model=<model>    a500, a500+, a600, a1000, a1200, a2000, a2500, a3000,
                     a3000t, a3000+, a4000, a4000t, cdtv, cd32 or draco
    noboot           load everything, then return to AmigaOS

The model is only found on its own on an A3000, A4000 or A4000T, whose
Kickstart says so; elsewhere the kernel only looks for the chips every
Amiga has unless `model=` is given.

## Building

    git clone --recursive <repo>
    meson setup build --cross-file m68k-linux-gnu.txt
    ninja -C build

This needs `gcc-m68k-linux-gnu`, meson and ninja.

## Licence

amibootng is under the GPL version 2, see `LICENSE`.
