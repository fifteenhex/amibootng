#ifndef AMIBOOTNG_UTIL_H
#define AMIBOOTNG_UTIL_H

#include <stddef.h>
#include "amigaos.h"

#define PATH_SIZE       256

int read_full(AmigaBPTR fh, void *buf, unsigned long len);
long load_file(const char *path, void *buf, unsigned long size);
long file_size(const char *path);
void join(char *out, size_t size, const char *dir, const char *name);

/* print a message and keep it for last_error() */
void error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
const char *last_error(void);

#endif
