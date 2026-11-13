/*
 * Choose what to boot: every m68k kernel in a drawer, or the one kernel
 * given, plus AmigaOS.
 */
#include <stdio.h>
#include <string.h>
#include "boot.h"
#include "menu.h"
#include "splash.h"
#include "util.h"

#define MAX_KERNELS     6
#define NAME_SIZE       108     /* fib_FileName */

static char kernels[MAX_KERNELS][NAME_SIZE];

/* the kernels in dir, sorted by name, and which is vmlinux (or 0) */
static int scan(AmigaDir *d, const char *dir, int *def)
{
    char name[NAME_SIZE], path[PATH_SIZE];
    int n = 0, isdir, i;

    while (amigaos_readdir(d, name, sizeof(name), NULL, &isdir)) {
        join(path, sizeof(path), dir, name);
        if (isdir || !elf_check(path))
            continue;
        if (n == MAX_KERNELS) {
            printf("too many kernels, skipping %s\n", name);
            continue;
        }
        for (i = n++; i > 0 && strcmp(kernels[i - 1], name) > 0; i--)
            strcpy(kernels[i], kernels[i - 1]);
        strcpy(kernels[i], name);
    }

    *def = 0;
    for (i = 0; i < n; i++)
        if (!strcmp(kernels[i], "vmlinux"))
            *def = i;
    return n;
}

int boot_menu(const char *path, const struct options *o)
{
    static char kernel[PATH_SIZE];
    const char *labels[MAX_KERNELS + 1];
    int n = 1, i, choice = 0, is_dir = 0;
    AmigaDir *d = amigaos_opendir(path);

    if (d) {
        is_dir = 1;
        n = scan(d, path, &choice);
        amigaos_closedir(d);
        if (!n) {
            error("no kernels in %s", path);
            return 5;
        }
        for (i = 0; i < n; i++)
            labels[i] = kernels[i];
    } else {
        labels[0] = path;
    }
    labels[n] = "AmigaOS (Workbench)";

    if (!o->nosplash) {
        if (splash_open(n + 3))
            error("cannot open the screen");
        else
            choice = splash_menu(labels, n + 1, choice, o->delay);
    }
    if (choice < 0 || choice == n) {
        printf("boot cancelled\n");
        return 5;
    }
    splash_status("Loading Linux...");

    if (!is_dir)
        return boot(path, o);
    join(kernel, sizeof(kernel), path, kernels[choice]);
    return boot(kernel, o);
}
