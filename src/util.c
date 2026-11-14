#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "lvo.h"
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

void delay(long ticks)
{
    LP1(-198, amigaos_dosbase(), long, ticks, d1);
}

unsigned long wait_signals(unsigned long signals)
{
    return LP1(-318, amigaos_execbase(), unsigned long, signals, d0);
}

static char last[160];
static void (*message_hook)(const char *prefix, const char *msg);

void set_message_hook(void (*hook)(const char *prefix, const char *msg))
{
    message_hook = hook;
}

static void vmessage(char *buf, size_t size, const char *prefix,
                     const char *fmt, va_list ap)
{
    vsnprintf(buf, size, fmt, ap);
    printf("%s\n", buf);
    if (message_hook)
        message_hook(prefix, buf);
}

void message(const char *fmt, ...)
{
    char buf[160];
    va_list ap;

    va_start(ap, fmt);
    vmessage(buf, sizeof(buf), "", fmt, ap);
    va_end(ap);
}

void error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vmessage(last, sizeof(last), "error: ", fmt, ap);
    va_end(ap);
}

const char *last_error(void)
{
    return last;
}
