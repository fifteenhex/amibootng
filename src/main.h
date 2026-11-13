#ifndef AMIBOOTNG_MAIN_H
#define AMIBOOTNG_MAIN_H

struct options {
    const char *ramdisk;
    const char *model;
    int noboot;
    char cmdline[256];
};

int parse_options(int argc, char **argv, struct options *o);
int boot(const char *kernel, const struct options *o);

#endif
