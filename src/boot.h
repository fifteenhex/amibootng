#ifndef AMIBOOTNG_BOOT_H
#define AMIBOOTNG_BOOT_H

#include <stdint.h>
#include "amigaos.h"

/* bootinfo tags, from the kernel's asm/bootinfo.h and bootinfo-amiga.h */
#define BI_LAST                 0x0000
#define BI_MACHTYPE             0x0001
#define BI_CPUTYPE              0x0002
#define BI_FPUTYPE              0x0003
#define BI_MMUTYPE              0x0004
#define BI_MEMCHUNK             0x0005
#define BI_RAMDISK              0x0006
#define BI_COMMAND_LINE         0x0007
#define BI_AMIGA_MODEL          0x8000
#define BI_AMIGA_AUTOCON        0x8001
#define BI_AMIGA_CHIP_SIZE      0x8002
#define BI_AMIGA_VBLANK         0x8003
#define BI_AMIGA_PSFREQ         0x8004
#define BI_AMIGA_ECLOCK         0x8005
#define BI_AMIGA_CHIPSET        0x8006
#define BI_AMIGA_SERPER         0x8007

#define MACH_AMIGA              1

#define CPU_68020               (1 << 0)
#define CPU_68030               (1 << 1)
#define CPU_68040               (1 << 2)
#define CPU_68060               (1 << 3)
#define FPU_68881               (1 << 0)
#define FPU_68882               (1 << 1)
#define FPU_68040               (1 << 2)
#define FPU_68060               (1 << 3)
#define MMU_68851               (1 << 0)
#define MMU_68030               (1 << 1)
#define MMU_68040               (1 << 2)
#define MMU_68060               (1 << 3)

#define AMI_UNKNOWN             0
#define AMI_500                 1       /* the first of amiga_models[] */
#define AMI_3000                8
#define AMI_4000                11
#define AMI_4000T               12
#define CS_OCS                  1
#define CS_ECS                  2
#define CS_AGA                  3

/* the kernel links at 0 and its PT_LOADs are 8KB aligned */
#define KERNEL_PAGE             0x2000UL

/*
 * head.S builds its transition mapping around the load address, so the
 * kernel goes at the start of fast RAM rounded down to 1MB.
 */
#define KERNEL_BASE_ALIGN       0x100000UL

#define ALIGN_UP(x, a)  (((unsigned long)(x) + (a) - 1) & ~((unsigned long)(a) - 1))

struct kernel_image {
    uint32_t entry;     /* offsets from the load address */
    uint32_t high;      /* end of the image, .bss included */
};

struct boot_params {
    uint32_t cpu, fpu, mmu;
    uint32_t model, chipset;
    uint32_t memchunk_addr, memchunk_size;
    uint32_t chip_size;
    uint32_t ramdisk_addr, ramdisk_size;    /* addr 0: none */
    const char *command_line;               /* NULL: none */
};

int elf_check(const char *path);
int elf_load(AmigaBPTR fh, uint8_t *base, unsigned long size,
             struct kernel_image *ki);

void cpu_from_attnflags(unsigned attn, uint32_t *cpu, uint32_t *fpu,
                        uint32_t *mmu);
unsigned long bootinfo_build(uint8_t *buf, unsigned long size,
                             const struct boot_params *p);

#endif
