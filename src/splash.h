#ifndef AMIBOOTNG_SPLASH_H
#define AMIBOOTNG_SPLASH_H

/* open the screen with room for lines of text under the logo; 0 on success */
int splash_open(int lines);

/* replace the text under the logo with s, if the screen is open */
void splash_status(const char *s);

/*
 * Offer the n labels, counting seconds down to label def (0: no
 * countdown).  Returns the label picked, or -1 for Esc or the right button.
 * Needs n + 2 lines.
 */
int splash_menu(const char *const *labels, int n, int def, int seconds);

/* show s and wait a while for a key, if the screen is open */
void splash_error(const char *s);

void splash_close(void);

#endif
