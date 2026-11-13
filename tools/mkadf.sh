#!/bin/sh
# mkadf.sh <xdftool> <out.adf> [-i <Installer>] <README.md> <file>...
#
# The install floppy.  It is not bootable: the installer edits SYS:, which
# has to be the hard disk.
set -e
xdftool=$1 out=$2
shift 2

rm -f "$out"
"$xdftool" "$out" create + format amibootng ffs
if [ "$1" = -i ]; then
    "$xdftool" "$out" write "$2" Installer
    shift 2
fi
"$xdftool" "$out" write "$1" README
shift
for f; do
    "$xdftool" "$out" write "$f" "$(basename "$f")"
done
