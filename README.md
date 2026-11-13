# amibootng

Boots Linux/m68k from AmigaOS.  It runs from `S:Startup-Sequence` after
`SetPatch`, shows Tux and a menu of the kernels in a drawer, and hands the
machine to the chosen kernel the way QEMU's `-kernel` does.  It needs a
68020 or better and fast RAM.  Only the fast RAM region the kernel is
loaded into is passed on.

    amibootng <kernel or drawer> [options] [kernel arguments]

A drawer is searched for m68k kernels; `vmlinux` is the default.  In the
menu, Up/Down or a click stop the countdown, Return or a click on an entry
boots it, and Esc or the right mouse button return to AmigaOS.

Options:

    @<file>          the words in <file>; a word starting with ';' or '#'
                     comments out the rest of its line
    delay=<s>        countdown before booting the default (5)
    nosplash         boot the default without the menu
    ramdisk=<file>   initrd
    model=<model>    a500, a500+, a600, a1000, a1200, a2000, a2500, a3000,
                     a3000t, a3000+, a4000, a4000t, cdtv, cd32 or draco
    noboot           load everything, then return to AmigaOS

The model is only found on its own on an A3000, A4000 or A4000T, whose
Kickstart says so; elsewhere the kernel only looks for the chips every
Amiga has unless `model=` is given.

A kernel with a `<kernel>.args` file next to it is booted with the
arguments in that file instead, apart from `model=`, which is kept.

## Installing

`amibootng.adf` is an install floppy for the AmigaOS Installer, which is on
the Install disk of AmigaOS 3.x.  `Install-amibootng` copies amibootng to
`C:`, makes a drawer for the kernels (`SYS:Linux`) with an
`amibootng.args`, and adds amibootng to `S:Startup-Sequence` after
`SetPatch`, keeping the original as `S:Startup-Sequence.pre-amibootng`.
`Remove-amibootng` takes it out again.

## Building

    git clone --recursive <repo>
    meson setup build --cross-file m68k-linux-gnu.txt
    ninja -C build

This needs `gcc-m68k-linux-gnu`, meson and ninja.  The floppy is built
when amitools' `xdftool` is on `PATH`.  The Installer is not included, so
it has to be in `C:` on the Amiga, unless `-Dinstaller=<path>` puts it on
the floppy.

## Licence

amibootng is under the GPL version 2, see `LICENSE`.
`data/logo_linux_vga16.ppm` is the Linux kernel's boot logo; Tux was drawn
by Larry Ewing (lewing@isc.tamu.edu) using The GIMP, and may be used as
long as he is credited.
