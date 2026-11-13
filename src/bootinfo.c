/*
 * The bootinfo records the kernel finds after its _end, laid out as QEMU's
 * hw/m68k/amiga.c builds them for -kernel.
 */
#include <string.h>
#include "boot.h"

#define SERIAL_BAUD             9600    /* the kernel console's default */
#define CONFIGDEV_SIZE          68

/* ExecBase fields (exec/execbase.h, V36) */
#define EXEC_VBLANK_FREQ        0x212   /* UBYTE */
#define EXEC_POWER_FREQ         0x213   /* UBYTE */
#define EXEC_ECLOCK_FREQ        0x238   /* ULONG */

struct bi {
    uint8_t *p, *end;
};

/* reserve a record for size bytes of data; NULL once out of room */
static uint8_t *record(struct bi *b, uint16_t tag, unsigned size)
{
    uint16_t hdr[2] = { tag, 4 + ALIGN_UP(size, 4) };
    uint8_t *data = b->p + 4;

    if (!b->p || hdr[1] > b->end - b->p) {
        b->p = NULL;
        return NULL;
    }
    memcpy(b->p, hdr, 4);
    memset(data, 0, hdr[1] - 4);
    b->p += hdr[1];
    return data;
}

static void bi_data(struct bi *b, uint16_t tag, const void *data,
                    unsigned size)
{
    uint8_t *p = record(b, tag, size);

    if (p)
        memcpy(p, data, size);
}

static void bi_u32(struct bi *b, uint16_t tag, uint32_t v)
{
    bi_data(b, tag, &v, 4);
}

static void bi_u16(struct bi *b, uint16_t tag, uint16_t v)
{
    bi_data(b, tag, &v, 2);
}

static void bi_u8(struct bi *b, uint16_t tag, uint8_t v)
{
    bi_data(b, tag, &v, 1);
}

void cpu_from_attnflags(unsigned a, uint32_t *cpu, uint32_t *fpu,
                        uint32_t *mmu)
{
    /* 68881/68882 on a 68020/030; on a 68040/060, FPU40 is clear for LC parts */
    uint32_t fpu8x = a & (1 << AMIGA_AFB_68882) ? FPU_68882 :
                     a & (1 << AMIGA_AFB_68881) ? FPU_68881 : 0;
    int fpu40 = !!(a & (1 << AMIGA_AFB_FPU40));

    if (a & (1 << AMIGA_AFB_68060)) {
        *cpu = CPU_68060; *mmu = MMU_68060; *fpu = fpu40 ? FPU_68060 : 0;
    } else if (a & (1 << AMIGA_AFB_68040)) {
        *cpu = CPU_68040; *mmu = MMU_68040; *fpu = fpu40 ? FPU_68040 : 0;
    } else if (a & (1 << AMIGA_AFB_68030)) {
        *cpu = CPU_68030; *mmu = MMU_68030; *fpu = fpu8x;
    } else if (a & (1 << AMIGA_AFB_68020)) {
        *cpu = CPU_68020; *mmu = MMU_68851; *fpu = fpu8x;
    } else {
        *cpu = *mmu = *fpu = 0;
    }
}

/* the length of the records built at buf, or 0 if they need more than size */
unsigned long bootinfo_build(uint8_t *buf, unsigned long size,
                             const struct boot_params *p)
{
    const uint8_t *exec = amigaos_execbase();
    uint32_t eclock, chunk[2] = { p->memchunk_addr, p->memchunk_size };
    uint32_t ramdisk[2] = { p->ramdisk_addr, p->ramdisk_size };
    struct bi b = { buf, buf + size };
    void *cd;

    memcpy(&eclock, exec + EXEC_ECLOCK_FREQ, 4);

    bi_u32(&b, BI_MACHTYPE, MACH_AMIGA);
    bi_u32(&b, BI_CPUTYPE, p->cpu);
    bi_u32(&b, BI_FPUTYPE, p->fpu);
    bi_u32(&b, BI_MMUTYPE, p->mmu);
    /* the first memory chunk is the one the kernel is loaded into */
    bi_data(&b, BI_MEMCHUNK, chunk, sizeof(chunk));
    bi_u32(&b, BI_AMIGA_MODEL, p->model);
    bi_u32(&b, BI_AMIGA_CHIPSET, p->chipset);
    bi_u32(&b, BI_AMIGA_CHIP_SIZE, p->chip_size);
    bi_u8(&b, BI_AMIGA_VBLANK, exec[EXEC_VBLANK_FREQ]);
    bi_u8(&b, BI_AMIGA_PSFREQ, exec[EXEC_POWER_FREQ]);
    bi_u32(&b, BI_AMIGA_ECLOCK, eclock);
    bi_u16(&b, BI_AMIGA_SERPER, 5 * eclock / SERIAL_BAUD - 1);
    /* the kernel's zorro code reads a copy of each struct ConfigDev */
    for (cd = amigaos_next_configdev(NULL); cd; cd = amigaos_next_configdev(cd))
        bi_data(&b, BI_AMIGA_AUTOCON, cd, CONFIGDEV_SIZE);
    if (p->command_line)
        bi_data(&b, BI_COMMAND_LINE, p->command_line,
                strlen(p->command_line) + 1);
    if (p->ramdisk_addr)
        bi_data(&b, BI_RAMDISK, ramdisk, sizeof(ramdisk));
    record(&b, BI_LAST, 0);
    return b.p ? (unsigned long)(b.p - buf) : 0;
}
