/*
 * Choose what to boot: every m68k kernel in a drawer, or the one kernel
 * given, plus AmigaOS.  The menu is on the screen and, with serial, on the
 * serial port as well; the README describes its commands.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boot.h"
#include "menu.h"
#include "serial.h"
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

static void print_entries(const char *const *labels, int n, int sel)
{
    int i;

    for (i = 0; i < n; i++)
        serial_print("%d %s%s", i, labels[i], i == sel ? " *" : "");
    serial_print("ready");
}

/*
 * "args" replaces the arguments for this boot; they are checked now, so a
 * mistake is reported while the menu is still up.
 */
static int override_set;
static struct options override;

static void set_override(const char *words)
{
    static char line[SERIAL_LINE_SIZE], buf[ARGFILE_SIZE];
    char *in[ARGV_MAX], *out[ARGV_MAX], *w;
    int n = 0;

    override_set = 0;
    words += strspn(words, " \t");
    if (!*words) {
        serial_print("args cleared");
        return;
    }
    snprintf(line, sizeof(line), "%s", words);
    for (w = strtok(line, " \t"); w; w = strtok(NULL, " \t")) {
        if (n == ARGV_MAX - 1) {
            error("more than %d arguments", ARGV_MAX - 1);
            return;
        }
        in[n++] = w;
    }
    n = expand_args(n, in, out, buf, sizeof(buf));
    if (n < 0 || parse_options(n, out, &override, 0))
        return;
    override_set = 1;
    serial_print("args %s", words);
}

/* a serial command as a menu event */
static enum menu_event command(char *line, int n, int *entry)
{
    char *end;
    long i;

    if (!strcmp(line, "boot"))
        return MENU_BOOT;
    if (!strcmp(line, "quit"))
        return MENU_CANCEL;
    if (!strcmp(line, "list"))
        return MENU_LIST;
    if (!strncmp(line, "args", 4) && (!line[4] || line[4] == ' ')) {
        set_override(line + 4);
        return MENU_STOP;
    }
    i = strtol(line, &end, 10);
    if (end != line && !*end && i >= 0 && i < n) {
        *entry = i;
        return MENU_PICK;
    }
    serial_print("error: unknown command %s", line);
    return MENU_STOP;
}

/* the next thing the screen or the serial port has for the menu */
static enum menu_event next_event(int *entry, char **line)
{
    enum menu_event event = splash_event(entry);
    int got;

    if (event)
        return event;
    got = serial_read(line);
    if (got == SERIAL_LINE)
        return MENU_LINE;
    return got ? MENU_STOP : MENU_NONE;
}

/*
 * Offer the n labels on the screen and the serial port, counting seconds
 * down to def (0: no countdown).  Returns the entry picked, or -1.
 */
static int choose(const char *const *labels, int n, int def, int seconds)
{
    static const char hint[] = "Return or a click boots the selection";
    int sel = def, ticks = seconds * TICKS_PER_SEC, counting = ticks > 0;
    enum menu_event event;
    char status[64], *line = NULL;
    unsigned long signals;
    int entry;

    print_entries(labels, n, sel);
    if (!counting)
        splash_draw_menu(labels, n, sel, hint);
    for (;;) {
        if (counting && ticks % TICKS_PER_SEC == 0) {
            snprintf(status, sizeof(status), "%s in %d s", labels[sel],
                     ticks / TICKS_PER_SEC);
            splash_draw_menu(labels, n, sel, status);
            serial_print("%s", status);
        }

        while ((event = next_event(&entry, &line))) {
            if (event == MENU_KEY)
                continue;
            if (counting) {
                counting = 0;
                serial_print("stopped");
                splash_draw_menu(labels, n, sel, hint);
            }
            if (event == MENU_LINE)
                event = command(line, n, &entry);
            switch (event) {
            case MENU_BOOT:
                return sel;
            case MENU_CANCEL:
                return -1;
            case MENU_PICK:
                if (entry >= 0 && entry < n)
                    return entry;
                break;
            case MENU_LIST:
                print_entries(labels, n, sel);
                break;
            case MENU_UP:
            case MENU_DOWN:
                sel = (sel + (event == MENU_UP ? n - 1 : 1)) % n;
                serial_print("%d %s *", sel, labels[sel]);
                splash_draw_menu(labels, n, sel, hint);
                break;
            default:
                break;
            }
        }

        signals = splash_signal() | serial_signal();
        if (!counting && !signals)
            return sel;         /* the serial port stopped, no screen */
        if (!counting)
            wait_signals(signals);
        else if ((ticks -= POLL_TICKS) <= 0)
            return sel;
        else
            delay(POLL_TICKS);
    }
}

/* boot kernel with the serial port's args, or its own .args, if any */
static int boot_kernel(const char *kernel, const struct options *o)
{
    static char args_path[PATH_SIZE + 6], buf[ARGFILE_SIZE];
    static char *args[ARGV_MAX];
    char *at = args_path;
    struct options own;
    int n;

    if (override_set) {
        own = override;
    } else {
        snprintf(args_path, sizeof(args_path), "@%s.args", kernel);
        if (file_size(args_path + 1) < 0)
            return boot(kernel, o);
        n = expand_args(1, &at, args, buf, sizeof(buf));
        if (n < 0 || parse_options(n, args, &own, 0))
            return 20;
    }
    if (!own.model)
        own.model = o->model;   /* the machine, not the kernel */
    return boot(kernel, &own);
}

static void serial_message(const char *prefix, const char *msg)
{
    serial_print("%s%s", prefix, msg);
}

int boot_menu(const char *path, const struct options *o)
{
    static char kernel[PATH_SIZE];
    const char *labels[MAX_KERNELS + 1];
    int n = 1, i, choice = 0, is_dir = 0, screen = 0, serial = 0;
    AmigaDir *d;

    if (o->serial) {
        serial = !serial_open(o->serial);
        if (serial)
            set_message_hook(serial_message);
        else
            error("cannot open serial.device");
    }

    d = amigaos_opendir(path);
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
        screen = !splash_open(n + 3);
        if (!screen)
            error("cannot open the screen");
    }
    if (screen || serial)
        choice = choose(labels, n + 1, choice, o->delay);
    if (choice < 0 || choice == n) {
        message("boot cancelled");
        return 5;
    }

    if (is_dir)
        join(kernel, sizeof(kernel), path, kernels[choice]);
    else
        snprintf(kernel, sizeof(kernel), "%s", path);
    splash_status("Loading Linux...");
    message("booting %s", kernel);
    return boot_kernel(kernel, o);
}
