/*
 * amibootng-startup ADD <drawer> | REMOVE
 *
 * Put amibootng in S:Startup-Sequence right after SetPatch (which sets up
 * the AttnFlags amibootng reads the CPU from), or take it out again.  The
 * AmigaOS Installer, which runs the rest of the installation, can only
 * append to S:User-Startup.  The original file is kept as
 * S:Startup-Sequence.pre-amibootng.
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "util.h"
#include "version.h"

AMIGA_VERSTAG("amibootng-startup", AMIBOOTNG_VERSION, AMIBOOTNG_DATE);

#define STARTUP         "S:Startup-Sequence"
#define BACKUP          STARTUP ".pre-amibootng"
#define NEW             STARTUP ".amibootng-new"
#define OLD             STARTUP ".amibootng-old"
#define BEGIN           "; BEGIN amibootng"
#define END             "; END amibootng"

static int write_file(const char *path, const char *const *piece,
                      const unsigned long *len, int n)
{
    AmigaBPTR fh = amigaos_open(path, AMIGA_MODE_NEWFILE);
    int i, ret = 0;

    if (!fh)
        return -1;
    for (i = 0; i < n && !ret; i++)
        if (amigaos_write(fh, piece[i], len[i]) != (long)len[i])
            ret = -1;
    amigaos_close(fh);
    return ret;
}

static char *next_line(char *p)
{
    p += strcspn(p, "\n");
    return *p ? p + 1 : p;
}

static char *find_line(char *text, const char *prefix)
{
    for (; *text; text = next_line(text))
        if (!strncmp(text, prefix, strlen(prefix)))
            return text;
    return NULL;
}

/*
 * The line running SetPatch, as SetPatch, C:SetPatch, SYS:C/SetPatch...
 * test/mkdisk.sh looks for it the same way.
 */
static char *find_setpatch(char *text)
{
    for (; *text; text = next_line(text)) {
        char *cmd = text + strspn(text, " \t");
        size_t len = strcspn(cmd, " \t\n");

        if (*cmd == ';')
            continue;
        if (len >= 8 && !strncasecmp(cmd + len - 8, "setpatch", 8) &&
            (len == 8 || strchr(":/", cmd[len - 9])))
            return text;
    }
    return NULL;
}

/* cut an earlier block out of text; non-zero if there was one */
static int remove_block(char *text, unsigned long *len)
{
    char *begin = find_line(text, BEGIN), *end;

    if (!begin)
        return 0;
    end = find_line(begin, END);
    end = next_line(end ? end : begin);
    memmove(begin, end, text + *len - end + 1);
    *len -= end - begin;
    return 1;
}

/* the old file stays in place until the new one is complete */
static int replace(const char *const *piece, const unsigned long *len, int n)
{
    amigaos_delete(OLD);        /* left by an interrupted run */
    if (write_file(NEW, piece, len, n)) {
        error("cannot write %s (error %ld)", NEW, amigaos_ioerr());
        amigaos_delete(NEW);
        return 20;
    }
    if (!amigaos_rename(STARTUP, OLD)) {
        error("cannot rename %s (error %ld)", STARTUP, amigaos_ioerr());
        amigaos_delete(NEW);
        return 20;
    }
    if (!amigaos_rename(NEW, STARTUP)) {
        error("cannot rename %s (error %ld)", NEW, amigaos_ioerr());
        amigaos_rename(OLD, STARTUP);
        amigaos_delete(NEW);
        return 20;
    }
    amigaos_delete(OLD);
    return 0;
}

static int add(char *text, unsigned long len, const char *dir)
{
    static char block[1024];
    char args[PATH_SIZE];
    const char *piece[4];
    unsigned long plen[4];
    char *at;
    int n = 0, size;

    join(args, sizeof(args), dir, "amibootng.args");
    size = snprintf(block, sizeof(block),
                    BEGIN "\n"
                    "If EXISTS \"%s\"\n"
                    "  FailAt 21\n"
                    "  C:amibootng \"%s\" \"@%s\"\n"
                    "  FailAt 10\n"
                    "EndIf\n"
                    END "\n", dir, dir, args);
    if (size >= (int)sizeof(block)) {
        error("%s is too long", dir);
        return 20;
    }

    remove_block(text, &len);
    at = find_setpatch(text);
    if (!at) {
        error("no SetPatch in %s", STARTUP);
        printf("add this after it:\n%s", block);
        return 10;
    }
    at = next_line(at);
    piece[n] = text;
    plen[n++] = at - text;
    if (at[-1] != '\n') {
        piece[n] = "\n";
        plen[n++] = 1;
    }
    piece[n] = block;
    plen[n++] = size;
    piece[n] = at;
    plen[n++] = text + len - at;
    return replace(piece, plen, n);
}

int main(int argc, char **argv)
{
    unsigned long len;
    long size, got;
    const char *piece;
    char *text;
    int ret;

    if (!(argc == 3 && !strcmp(argv[1], "ADD") && argv[2][0]) &&
        !(argc == 2 && !strcmp(argv[1], "REMOVE"))) {
        printf("usage: amibootng-startup ADD <drawer> | REMOVE\n");
        return 20;
    }

    size = file_size(STARTUP);
    if (size < 0) {
        error("cannot open %s", STARTUP);
        return 20;
    }
    text = amigaos_allocmem(size + 1, AMIGA_MEMF_PUBLIC);
    if (!text) {
        error("out of memory");
        return 20;
    }
    got = load_file(STARTUP, text, size);
    if (got < 0) {
        amigaos_freemem(text, size + 1);
        return 20;
    }
    len = got;
    text[len] = 0;

    if (argc == 3) {
        if (file_size(BACKUP) < 0 &&
            write_file(BACKUP, (const char **)&text, &len, 1)) {
            error("cannot write %s", BACKUP);
            ret = 20;
        } else {
            ret = add(text, len, argv[2]);
        }
    } else if (!remove_block(text, &len)) {
        error("amibootng is not in %s", STARTUP);
        ret = 5;
    } else {
        piece = text;
        ret = replace(&piece, &len, 1);
    }
    amigaos_freemem(text, size + 1);
    return ret;
}
