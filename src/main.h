#ifndef AMIBOOTNG_MAIN_H
#define AMIBOOTNG_MAIN_H

#define ARGV_MAX        64
#define ARGFILE_SIZE    2048    /* for the words of @files */

struct options {
    const char *ramdisk;
    const char *model;
    int noboot;
    char cmdline[256];
};

int parse_options(int argc, char **argv, struct options *o);
int expand_args(int argc, char **in, char **out, char *buf,
                unsigned long size);
int boot(const char *kernel, const struct options *o);

#endif
