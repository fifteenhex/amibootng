#include <stdarg.h>
#include <stdio.h>
#include <string.h>
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

/* -1 if path cannot be opened */
long file_size(const char *path)
{
    AmigaBPTR fh = amigaos_open(path, AMIGA_MODE_OLDFILE);
    long len;

    if (!fh)
        return -1;
    len = amigaos_size(fh);
    amigaos_close(fh);
    return len;
}

/* dir/name; no '/' after a volume or one that is already there */
void join(char *out, size_t size, const char *dir, const char *name)
{
    size_t len = strlen(dir);
    int sep = len && dir[len - 1] != ':' && dir[len - 1] != '/';

    snprintf(out, size, "%s%s%s", dir, sep ? "/" : "", name);
}

static char message[160];

void error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(message, sizeof(message), fmt, ap);
    va_end(ap);
    printf("%s\n", message);
}

const char *last_error(void)
{
    return message;
}
