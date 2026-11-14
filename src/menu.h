#ifndef AMIBOOTNG_MENU_H
#define AMIBOOTNG_MENU_H

#include "main.h"

enum menu_event {
    MENU_NONE,
    MENU_UP,
    MENU_DOWN,
    MENU_BOOT,
    MENU_PICK,
    MENU_CANCEL,
    MENU_LIST,
    MENU_LINE,          /* a command from the serial port */
    MENU_KEY,           /* any other key */
    MENU_STOP,          /* input that only stops the countdown */
};

/* boot path, a kernel or a drawer of them, offering a menu unless nosplash */
int boot_menu(const char *path, const struct options *o);

#endif
