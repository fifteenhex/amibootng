#ifndef AMIBOOTNG_UTIL_H
#define AMIBOOTNG_UTIL_H

#include "amigaos.h"

int read_full(AmigaBPTR fh, void *buf, unsigned long len);
long load_file(const char *path, void *buf, unsigned long size);

void error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
