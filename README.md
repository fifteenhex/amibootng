# amibootng

A native AmigaOS bootstrap for Linux/m68k, built with amipicolibc.

Loads a kernel and optional ramdisk from an AmigaOS filesystem, builds
the m68k bootinfo, and hands control to the kernel — the same job QEMU's
`-kernel` shortcut does, but done properly from inside AmigaOS so it runs
on real hardware and boots from disk.
