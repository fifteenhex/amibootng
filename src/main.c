/*
 * amibootng - boot Linux/m68k from AmigaOS
 *
 * The kernel is loaded into the largest free block of fast RAM, followed
 * by its bootinfo and ramdisk.  Once AmigaOS is shut out the lot is copied
 * down to the start of fast RAM, where the kernel runs, and entered as
 * QEMU's -kernel would enter it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boot.h"
#include "main.h"
#include "menu.h"
#include "serial.h"
#include "splash.h"
#include "util.h"

/*
 * head.S takes its page tables from the memory past _end and paging_init()
 * reserves up to where it stopped, so a ramdisk must start beyond that.
 * The tables take tens of KB on the machines tried; 1MB leaves room.
 */
#define RAMDISK_GAP     0x100000UL
#define ERROR_SECONDS   10

/* the kernel's amiga_models[], from AMI_500 */
static const char *const models[] = {
    "a500", "a500+", "a600", "a1000", "a1200", "a2000", "a2500", "a3000",
    "a3000t", "a3000+", "a4000", "a4000t", "cdtv", "cd32", "draco",
};

/* amibootng's options, and the kernel command line from the rest */
int parse_options(int argc, char **argv, struct options *o, int menu)
{
    char *end;
    int i, menu_only;

    memset(o, 0, sizeof(*o));
    o->delay = DEFAULT_DELAY;
    for (i = 0; i < argc; i++) {
        menu_only = 0;
        if (!strncmp(argv[i], "ramdisk=", 8)) {
            o->ramdisk = argv[i] + 8;
        } else if (!strncmp(argv[i], "model=", 6)) {
            o->model = argv[i] + 6;
        } else if (!strcmp(argv[i], "noboot")) {
            o->noboot = 1;
        } else if (!strncmp(argv[i], "delay=", 6)) {
            o->delay = atoi(argv[i] + 6);
            menu_only = 1;
        } else if (!strcmp(argv[i], "nosplash")) {
            o->nosplash = 1;
            menu_only = 1;
        } else if (!strcmp(argv[i], "serial")) {
            o->serial = DEFAULT_BAUD;
            menu_only = 1;
        } else if (!strncmp(argv[i], "serial=", 7)) {
            o->serial = strtoul(argv[i] + 7, &end, 10);
            if (!o->serial || *end) {
                error("bad baud rate %s", argv[i] + 7);
                return -1;
            }
            menu_only = 1;
        } else {
            size_t len = strlen(o->cmdline);

            if (snprintf(o->cmdline + len, sizeof(o->cmdline) - len, "%s%s",
                         len ? " " : "", argv[i]) >=
                (int)(sizeof(o->cmdline) - len)) {
                error("kernel command line longer than %d",
                      (int)sizeof(o->cmdline) - 1);
                return -1;
            }
        }
        if (menu_only && !menu) {
            error("%s only works on amibootng's command line", argv[i]);
            return -1;
        }
    }
    return 0;
}

/*
 * Kickstart has an "A3000 bonus" or "A4000 bonus" module, and the A4000T's
 * also has an "NCR scsi.device" for its SCSI.  Other machines have to be
 * named with model=, or the kernel is told the model is unknown and only
 * looks for the chips all Amigas have.
 */
static int detect_model(struct boot_params *bp, const char *model)
{
    unsigned bits = amigaos_chiprevbits();
    unsigned i;

    bp->chipset = bits & AMIGA_GFXF_AA_LISA ? CS_AGA :
                  bits & AMIGA_GFXF_HR_DENISE ? CS_ECS : CS_OCS;

    if (model) {
        for (i = 0; i < sizeof(models) / sizeof(models[0]); i++) {
            if (!strcmp(model, models[i])) {
                bp->model = AMI_500 + i;
                return 0;
            }
        }
        error("unknown model %s", model);
        return -1;
    }

    if (amigaos_findresident("A3000 bonus"))
        bp->model = AMI_3000;
    else if (amigaos_findresident("NCR scsi.device"))
        bp->model = AMI_4000T;
    else if (amigaos_findresident("A4000 bonus"))
        bp->model = AMI_4000;
    else
        bp->model = AMI_UNKNOWN;
    return 0;
}

/* the largest free block of fast RAM, retried if something takes it first */
static uint8_t *alloc_largest(unsigned long *size)
{
    uint8_t *p = NULL;
    int tries;

    for (tries = 0; !p && tries < 3; tries++) {
        *size = amigaos_availmem(AMIGA_MEMF_FAST | AMIGA_MEMF_LARGEST);
        p = amigaos_allocmem(*size, AMIGA_MEMF_FAST);
    }
    return p;
}

int boot(const char *kernel, const struct options *o)
{
    struct boot_params bp = { 0 };
    struct kernel_image ki;
    unsigned long lo, hi, chip_lo, chip_hi, base, avail, len, room, bi_len;
    uint8_t *region;
    AmigaBPTR fh = 0;
    int ret = 20;

    if (!amigaos_fast_memory(&lo, &hi)) {
        error("no fast RAM");
        return 20;
    }
    if (!amigaos_chip_memory(&chip_lo, &chip_hi)) {
        error("no chip RAM");
        return 20;
    }
    /* lo is just past the region's MemHeader */
    base = lo & ~(KERNEL_BASE_ALIGN - 1);
    if (lo - base > 0x100) {
        error("fast RAM at 0x%08lx is not 1MB aligned", lo);
        return 20;
    }

    region = alloc_largest(&avail);
    if (!region) {
        error("out of memory");
        return 20;
    }

    fh = amigaos_open(kernel, AMIGA_MODE_OLDFILE);
    if (!fh) {
        error("cannot open %s (error %ld)", kernel, amigaos_ioerr());
        goto out;
    }
    if (elf_load(fh, region, avail, &ki))
        goto out;
    /* the kernel looks for its bootinfo at _end, as QEMU puts it */
    len = ALIGN_UP(ki.high, 2);
    room = avail - len;

    if (o->ramdisk) {
        unsigned long off = ALIGN_UP(len + RAMDISK_GAP, KERNEL_PAGE);
        long rlen;

        if (off >= avail) {
            error("no room for %s", o->ramdisk);
            goto out;
        }
        rlen = load_file(o->ramdisk, region + off, avail - off);
        if (rlen < 0)
            goto out;
        if (!rlen) {
            error("%s is empty", o->ramdisk);
            goto out;
        }
        bp.ramdisk_addr = base + off;
        bp.ramdisk_size = rlen;
        room = off - len;
    }

    cpu_from_attnflags(amigaos_attnflags(), &bp.cpu, &bp.fpu, &bp.mmu);
    if (detect_model(&bp, o->model))
        goto out;
    bp.memchunk_addr = base;
    bp.memchunk_size = hi - base;
    bp.chip_size = chip_hi;        /* chip RAM starts at 0 */
    bp.command_line = o->cmdline[0] ? o->cmdline : NULL;

    bi_len = bootinfo_build(region + len, room, &bp);
    if (!bi_len) {
        error("no room for the bootinfo");
        goto out;
    }
    len = bp.ramdisk_addr ? bp.ramdisk_addr - base + bp.ramdisk_size
                          : len + bi_len;

    if (o->noboot) {
        message("%s: entry 0x%08lx, %lu bytes to 0x%08lx, \"%s\"",
                kernel, base + ki.entry, len, base, o->cmdline);
        ret = 0;
        goto out;
    }

    amigaos_close(fh);
    splash_status("Starting Linux...");
    serial_close();
    /* head.S loads its own stack pointer before using the stack */
    amigaos_takeover_reloc(region, (void *)base, len,
                           (void *)(base + ki.entry),
                           (void *)(base + ki.entry), NULL);
out:
    if (fh)
        amigaos_close(fh);
    amigaos_freemem(region, avail);
    return ret;
}

/*
 * Append the words in path to argv, using buf for them.  A word starting
 * with ';' or '#' comments out the rest of its line.
 */
static int expand_argfile(const char *path, char **argv, int *argc,
                          char *buf, unsigned long size)
{
    char *p = buf, *end;
    long len;

    if (size < 2) {
        error("no room for %s", path);
        return -1;
    }
    len = load_file(path, buf, size - 1);
    if (len < 0)
        return -1;
    end = p + len;
    *end = 0;

    while (p < end) {
        if (*p == ';' || *p == '#') {
            p = strchr(p, '\n') ?: end;
        } else if ((unsigned char)*p > ' ') {
            if (*argc == ARGV_MAX - 1) {
                error("more than %d arguments", ARGV_MAX - 1);
                return -1;
            }
            argv[(*argc)++] = p;
            p += strcspn(p, " \t\r\n");
            *p++ = 0;
        } else {
            p++;
        }
    }
    return len;
}

/*
 * Copy in to out with each @file replaced by its words, which are kept in
 * buf.  The new argc, or -1.
 */
int expand_args(int argc, char **in, char **out, char *buf,
                unsigned long size)
{
    int i, n = 0, used;

    for (i = 0; i < argc; i++) {
        if (in[i][0] == '@') {
            used = expand_argfile(in[i] + 1, out, &n, buf, size);
            if (used < 0)
                return -1;
            buf += used + 1;
            size -= used + 1;
        } else if (n == ARGV_MAX - 1) {
            error("more than %d arguments", ARGV_MAX - 1);
            return -1;
        } else {
            out[n++] = in[i];
        }
    }
    out[n] = NULL;
    return n;
}

/* until a key, a click or a serial line, for up to seconds */
static void wait_for_input(int seconds)
{
    int ticks, entry;
    char *line;

    for (ticks = seconds * TICKS_PER_SEC; ticks > 0; ticks -= POLL_TICKS) {
        if (splash_event(&entry) || serial_read(&line) == SERIAL_LINE)
            return;
        delay(POLL_TICKS);
    }
}

int main(int argc, char **argv)
{
    static char *args[ARGV_MAX];
    static char buf[ARGFILE_SIZE];
    struct options o;
    int n, ret;

    if (argc < 2) {
        printf("usage: amibootng <kernel or drawer> [options] "
               "[kernel arguments]\n");
        return 20;
    }
    n = expand_args(argc - 2, argv + 2, args, buf, sizeof(buf));
    if (n < 0 || parse_options(n, args, &o, 1))
        return 20;

    ret = boot_menu(argv[1], &o);
    if (ret >= 20) {
        message("returning to AmigaOS");
        if (splash_error(last_error()))
            wait_for_input(ERROR_SECONDS);
    }
    splash_close();
    serial_close();
    return ret;
}
