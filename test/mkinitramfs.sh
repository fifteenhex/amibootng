#!/bin/sh
# mkinitramfs.sh <rootfs.cpio.gz> <out.gz>
#
# A small initramfs: busybox and the glibc it links against, taken from an
# m68k rootfs, and test/init.  The libraries are the ones a dynamically
# linked glibc busybox needs; adjust them for another busybox.
set -e
here=$(cd "$(dirname "$0")" && pwd)
rootfs=$(realpath "$1")
out=$(realpath -m "$2")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

mkdir "$tmp/src" "$tmp/root"
cd "$tmp/src"
zcat "$rootfs" | cpio -id --quiet bin/busybox 'lib/*'
cd "$tmp/root"
mkdir -p bin lib dev proc sys tmp usr/bin usr/sbin sbin
cp "$tmp/src/bin/busybox" bin/
cp -L "$tmp/src/lib/ld.so.1" "$tmp/src/lib/libc.so.6" \
      "$tmp/src/lib/libresolv.so.2" lib/
cp "$here/init" init
find . | cpio -o -H newc --owner 0:0 --quiet | gzip -9 > "$out"
