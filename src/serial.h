#ifndef AMIBOOTNG_SERIAL_H
#define AMIBOOTNG_SERIAL_H

/* the longest command line taken, NUL included */
#define SERIAL_LINE_SIZE        300

/* serial.device at baud; 0 on success */
int serial_open(unsigned long baud);
void serial_close(void);

/* send "amibootng: <fmt>", if the port is open */
void serial_print(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/*
 * Take what has arrived: 0 if nothing, SERIAL_INPUT for bytes that are not
 * yet a line, SERIAL_LINE with *line set to a complete one.
 */
#define SERIAL_INPUT    1
#define SERIAL_LINE     2
int serial_read(char **line);

/* the signal serial_read() has input on, or 0 */
unsigned long serial_signal(void);

#endif
