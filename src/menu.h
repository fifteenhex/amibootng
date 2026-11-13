#ifndef AMIBOOTNG_MENU_H
#define AMIBOOTNG_MENU_H

#include "main.h"

/* boot path, a kernel or a drawer of them, offering a menu unless nosplash */
int boot_menu(const char *path, const struct options *o);

#endif
