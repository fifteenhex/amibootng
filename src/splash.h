#ifndef AMIBOOTNG_SPLASH_H
#define AMIBOOTNG_SPLASH_H

#include "menu.h"

/* open the screen with room for lines of text under the logo; 0 on success */
int splash_open(int lines);

/* replace the text under the logo with s, if the screen is open */
void splash_status(const char *s);

/* a bar under the status line, done of total */
void splash_progress(unsigned long done, unsigned long total);

/* which logo to show */
#define SPLASH_TUX      0
#define SPLASH_BOING    1
void splash_logo(int which);

/* the n labels with sel highlighted, status under them, and a hint */
void splash_draw_menu(const char *const *labels, int n, int sel,
                      const char *status);

/* what the user did on the screen; MENU_PICK sets *entry to the line */
enum menu_event splash_event(int *entry);

/* the signal splash_event() has input on, or 0 */
unsigned long splash_signal(void);

/* show the error s, if the screen is open; whether it is */
int splash_error(const char *s);

void splash_close(void);

#endif
