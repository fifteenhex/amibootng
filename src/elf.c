#include <string.h>
#include "boot.h"
#include "util.h"

#define EM_68K          4
#define ET_EXEC         2
#define PT_LOAD         1
#define EF_M68K_M68000  0x01000000
#define MAX_PHDRS       16

struct elf32_hdr {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct elf32_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

static int read_at(AmigaBPTR fh, unsigned long off, void *buf,
                   unsigned long len)
{
    if (amigaos_seek(fh, off, AMIGA_SEEK_BEGINNING) < 0)
        return -1;
    return read_full(fh, buf, len);
}

/* a big-endian m68k executable */
static int read_header(AmigaBPTR fh, struct elf32_hdr *eh)
{
    if (read_at(fh, 0, eh, sizeof(*eh)))
        return -1;
    if (memcmp(eh->e_ident, "\177ELF\1\2", 6))
        return -1;
    if (eh->e_type != ET_EXEC || eh->e_machine != EM_68K)
        return -1;
    return 0;
}

int elf_check(const char *path)
{
    struct elf32_hdr eh;
    AmigaBPTR fh = amigaos_open(path, AMIGA_MODE_OLDFILE);
    int ret;

    if (!fh)
        return 0;
    ret = !read_header(fh, &eh);
    amigaos_close(fh);
    return ret;
}

/*
 * Load the PT_LOAD segments at base + p_paddr (the kernel links at 0),
 * zeroing .bss.  size is the room at base.
 */
int elf_load(AmigaBPTR fh, uint8_t *base, unsigned long size,
             struct kernel_image *ki)
{
    static struct elf32_phdr ph[MAX_PHDRS];
    struct elf32_hdr eh;
    unsigned long high = 0;
    int i;

    if (read_header(fh, &eh) || eh.e_phentsize != sizeof(ph[0]) ||
        !eh.e_phnum) {
        error("kernel is not an m68k executable");
        return -1;
    }
    if (eh.e_phnum > MAX_PHDRS) {
        error("kernel has %d program headers", eh.e_phnum);
        return -1;
    }
    if (read_at(fh, eh.e_phoff, ph, eh.e_phnum * sizeof(ph[0]))) {
        error("cannot read kernel");
        return -1;
    }
    if (eh.e_flags & EF_M68K_M68000) {
        error("kernel is built for the 68000");
        return -1;
    }

    for (i = 0; i < eh.e_phnum; i++)
        if (ph[i].p_type == PT_LOAD)
            expect_read(ph[i].p_filesz);

    for (i = 0; i < eh.e_phnum; i++) {
        struct elf32_phdr *p = &ph[i];

        if (p->p_type != PT_LOAD || !p->p_memsz)
            continue;
        if (p->p_filesz > p->p_memsz || p->p_paddr > size ||
            p->p_memsz > size - p->p_paddr) {
            error("kernel does not fit in %lu bytes", size);
            return -1;
        }
        if (read_at(fh, p->p_offset, base + p->p_paddr, p->p_filesz)) {
            error("cannot read kernel");
            return -1;
        }
        memset(base + p->p_paddr + p->p_filesz, 0, p->p_memsz - p->p_filesz);
        if (p->p_paddr + p->p_memsz > high)
            high = p->p_paddr + p->p_memsz;
    }
    if (!high) {
        error("kernel has nothing to load");
        return -1;
    }

    ki->entry = eh.e_entry;
    ki->high = high;
    return 0;
}
