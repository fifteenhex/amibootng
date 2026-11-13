#include <stdarg.h>
#include <stdio.h>
#include "util.h"

/* large single reads came back corrupted from QEMU's A4000 IDE */
#define READ_CHUNK      0x10000UL

int read_full(AmigaBPTR fh, void *buf, unsigned long len)
{
    char *p = buf;

    while (len) {
        unsigned long n = len < READ_CHUNK ? len : READ_CHUNK;

        if (amigaos_read(fh, p, n) != (long)n)
            return -1;
        p += n;
        len -= n;
    }
    return 0;
}

/* read all of path into buf; its size, or -1 */
long load_file(const char *path, void *buf, unsigned long size)
{
    AmigaBPTR fh = amigaos_open(path, AMIGA_MODE_OLDFILE);
    long len;

    if (!fh) {
        error("cannot open %s (error %ld)", path, amigaos_ioerr());
        return -1;
    }
    len = amigaos_size(fh);
    if (len < 0 || (unsigned long)len > size) {
        error("%s does not fit in %lu bytes", path, size);
        len = -1;
    } else if (read_full(fh, buf, len)) {
        error("cannot read %s", path);
        len = -1;
    }
    amigaos_close(fh);
    return len;
}

void error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}
