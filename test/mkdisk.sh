#!/bin/sh
# mkdisk.sh <workbench.img> <out.img> <initramfs> <vmlinux>...
#
# A copy of a Workbench hard disk image (an RDB disk with a second
# partition, HD1:) with amibootng in C:, the stripped kernels, the
# initramfs and amibootng.args in HD1:Linux, and the block that
# amibootng-startup (src/startup.c) adds to S:Startup-Sequence.  Then:
#
#   qemu-system-m68k -M a3000 -m 16M -bios kick31_a3000.rom \
#     -drive if=scsi,file=out.img,format=raw -serial stdio
set -e
xdftool=${XDFTOOL:-xdftool}
strip=${STRIP:-m68k-linux-gnu-strip}
build=${BUILD:-$(dirname "$0")/../build}
wb=$1 out=$2 initramfs=$3
shift 3

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

echo "ramdisk=HD1:Linux/initramfs.gz console=tty0 console=ttyS0" \
    > "$tmp/amibootng.args"
"$xdftool" "$wb" type S/Startup-Sequence | awk '
    # as find_setpatch() in src/startup.c
    !done && tolower($1) ~ /(^|[:\/])setpatch$/ && $1 !~ /^;/ {
        print
        print "; BEGIN amibootng"
        print "If EXISTS \"HD1:Linux\""
        print "  FailAt 21"
        print "  C:amibootng \"HD1:Linux\" \"@HD1:Linux/amibootng.args\""
        print "  FailAt 10"
        print "EndIf"
        print "; END amibootng"
        done = 1
        next
    }
    { print }' > "$tmp/startup"

cp "$wb" "$out"
"$xdftool" "$out" write "$build/amibootng" C/amibootng + \
    delete S/Startup-Sequence + write "$tmp/startup" S/Startup-Sequence
"$xdftool" "$out" open part=1 + makedir Linux + \
    write "$initramfs" Linux/initramfs.gz + \
    write "$tmp/amibootng.args" Linux/amibootng.args
for k; do
    "$strip" -o "$tmp/kernel" "$k"
    "$xdftool" "$out" open part=1 + write "$tmp/kernel" "Linux/$(basename "$k")"
done
