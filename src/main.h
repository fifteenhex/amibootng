#ifndef AMIBOOTNG_MAIN_H
#define AMIBOOTNG_MAIN_H

#define ARGV_MAX        64
#define DEFAULT_DELAY   5
#define DEFAULT_BAUD    9600    /* the kernel's serial console default */
#define ARGFILE_SIZE    2048    /* for the words of @files */

struct options {
    const char *ramdisk;
    const char *model;
    int noboot;
    int nosplash;
    int delay;
    unsigned long serial;       /* baud rate, 0 for no serial control */
    char cmdline[256];
};

/* menu: whether delay=, nosplash and serial are allowed */
int parse_options(int argc, char **argv, struct options *o, int menu);
int expand_args(int argc, char **in, char **out, char *buf,
                unsigned long size);
int boot(const char *kernel, const struct options *o);

#endif
