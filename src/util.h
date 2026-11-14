#ifndef AMIBOOTNG_UTIL_H
#define AMIBOOTNG_UTIL_H

#include <stddef.h>
#include "amigaos.h"

#define PATH_SIZE       256

int read_full(AmigaBPTR fh, void *buf, unsigned long len);
long load_file(const char *path, void *buf, unsigned long size);
long file_size(const char *path);
void join(char *out, size_t size, const char *dir, const char *name);

#define TICKS_PER_SEC   50
#define POLL_TICKS      5

/* dos Delay() in ticks, and exec Wait() */
void delay(long ticks);
unsigned long wait_signals(unsigned long signals);

/* print a message, and hand it to the hook if one is set */
void message(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void set_message_hook(void (*hook)(const char *prefix, const char *msg));

/* message() an error, kept for last_error() */
void error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
const char *last_error(void);

#endif
