/*
 * The menu over the serial port, through serial.device: commands come in
 * as lines, and everything sent starts with "amibootng: ".
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "amigaos.h"
#include "lvo.h"
#include "serial.h"
#include "util.h"

static void *sysbase;

#define CreateMsgPort()         ((void *)LP0(-666, sysbase))
#define DeleteMsgPort(p)        LP1(-672, sysbase, void *, p, a0)
#define CreateIORequest(p, s)   ((void *)LP2(-654, sysbase,             \
        void *, p, a0, long, s, d0))
#define DeleteIORequest(r)      LP1(-660, sysbase, void *, r, a0)
#define OpenDevice(n, u, r, f)  LP4(-444, sysbase, const char *, n, a0, \
        long, u, d0, void *, r, a1, long, f, d1)
#define CloseDevice(r)          LP1(-450, sysbase, void *, r, a1)
#define DoIO(r)                 LP1(-456, sysbase, void *, r, a1)
#define SendIO(r)               LP1(-462, sysbase, void *, r, a1)
#define CheckIO(r)              ((void *)LP1(-468, sysbase, void *, r, a1))
#define WaitIO(r)               LP1(-474, sysbase, void *, r, a1)
#define AbortIO(r)              LP1(-480, sysbase, void *, r, a1)

/* struct IOExtSer (devices/serial.h) and MsgPort fields */
#define IOEXTSER_SIZE           82
#define IO_COMMAND              28      /* UWORD */
#define IO_ERROR                31      /* BYTE */
#define IO_LENGTH               36      /* ULONG */
#define IO_DATA                 40      /* APTR */
#define IO_BAUD                 60      /* ULONG */
#define IO_READLEN              76      /* UBYTE */
#define IO_WRITELEN             77
#define IO_STOPBITS             78
#define IO_SERFLAGS             79
#define MP_SIGBIT               15

#define CMD_READ                2
#define CMD_WRITE               3
#define SDCMD_SETPARAMS         11
#define SERF_SHARED             0x20
#define SERF_XDISABLED          0x80

/* io_Error values after which the byte read is still good */
#define SERERR_LINEERR          6
#define SERERR_PARITYERR        9
#define SERERR_BUFOVERFLOW      12
#define SERERR_DETECTEDBREAK    15

static void *port, *rd, *wr;
static int device_open, reading;
static unsigned char rxbyte;
static char line[SERIAL_LINE_SIZE];
static int line_len, too_long;

static void put8(void *req, int off, unsigned char v)
{
    *((unsigned char *)req + off) = v;
}

static void put16(void *req, int off, unsigned short v)
{
    memcpy((char *)req + off, &v, 2);
}

static void put32(void *req, int off, unsigned long v)
{
    memcpy((char *)req + off, &v, 4);
}

static void start_read(void)
{
    put16(rd, IO_COMMAND, CMD_READ);
    put32(rd, IO_LENGTH, 1);
    put32(rd, IO_DATA, (unsigned long)&rxbyte);
    trace("serial: SendIO read");
    SendIO(rd);
    reading = 1;
}

int serial_open(unsigned long baud)
{
    sysbase = amigaos_execbase();
    trace("serial: open %lu", baud);
    port = CreateMsgPort();
    trace("serial: port %p", port);
    if (port)
        rd = CreateIORequest(port, IOEXTSER_SIZE);
    if (rd)
        wr = CreateIORequest(port, IOEXTSER_SIZE);
    trace("serial: rd %p wr %p", rd, wr);
    if (!wr)
        goto fail;

    /* OpenDevice() looks at SERF_SHARED */
    put8(rd, IO_SERFLAGS, SERF_SHARED | SERF_XDISABLED);
    if (OpenDevice("serial.device", 0, rd, 0))
        goto fail;
    device_open = 1;
    trace("serial: opened, rbuflen %lu baud %lu flags %02x",
          *(unsigned long *)((char *)rd + 52),
          *(unsigned long *)((char *)rd + IO_BAUD),
          *((unsigned char *)rd + IO_SERFLAGS));

    /* no XON/XOFF: commands are plain text, and so is the kernel console */
    put16(rd, IO_COMMAND, SDCMD_SETPARAMS);
    put32(rd, IO_BAUD, baud);
    put8(rd, IO_READLEN, 8);
    put8(rd, IO_WRITELEN, 8);
    put8(rd, IO_STOPBITS, 1);
    put8(rd, IO_SERFLAGS, SERF_SHARED | SERF_XDISABLED);
    trace("serial: SETPARAMS");
    if (DoIO(rd))
        goto fail;
    trace("serial: SETPARAMS done");

    memcpy(wr, rd, IOEXTSER_SIZE);
    start_read();
    return 0;

fail:
    serial_close();
    return -1;
}

void serial_close(void)
{
    trace("serial: close, reading %d", reading);
    if (reading) {
        AbortIO(rd);
        WaitIO(rd);
    }
    if (device_open)
        CloseDevice(rd);
    if (wr)
        DeleteIORequest(wr);
    if (rd)
        DeleteIORequest(rd);
    if (port)
        DeleteMsgPort(port);
    device_open = reading = 0;
    port = rd = wr = NULL;
}

void serial_print(const char *fmt, ...)
{
    char buf[SERIAL_LINE_SIZE + 16];
    int len;
    va_list ap;

    if (!device_open)
        return;
    /* the text, cut short if need be to leave room for CR LF */
    len = snprintf(buf, sizeof(buf) - 2, "amibootng: ");
    va_start(ap, fmt);
    len += vsnprintf(buf + len, sizeof(buf) - 2 - len, fmt, ap);
    va_end(ap);
    if (len > (int)sizeof(buf) - 3)
        len = sizeof(buf) - 3;
    memcpy(buf + len, "\r\n", 2);

    put16(wr, IO_COMMAND, CMD_WRITE);
    put32(wr, IO_LENGTH, len + 2);
    put32(wr, IO_DATA, (unsigned long)buf);
    trace("serial: write %d at %p", len + 2, buf);
    DoIO(wr);
    trace("serial: write done, error %d",
          *((signed char *)wr + IO_ERROR));
}

int serial_read(char **out)
{
    int got = 0;

    while (reading && CheckIO(rd)) {
        unsigned char c = rxbyte;
        int err = WaitIO(rd);

        trace("serial: read %02x error %d", c, err);
        reading = 0;
        if (err && err != SERERR_LINEERR && err != SERERR_PARITYERR &&
            err != SERERR_BUFOVERFLOW && err != SERERR_DETECTEDBREAK) {
            serial_print("error: serial.device error %d, not reading", err);
            break;
        }
        start_read();
        if (err)
            continue;
        got = SERIAL_INPUT;
        if (c == '\r' || c == '\n') {
            if (too_long)
                serial_print("error: line longer than %d",
                             SERIAL_LINE_SIZE - 1);
            if (!line_len || too_long) {
                line_len = too_long = 0;
                continue;
            }
            line[line_len] = 0;
            line_len = 0;
            *out = line;
            return SERIAL_LINE;
        }
        if (c == '\t')
            c = ' ';
        if (c < ' ')
            continue;
        if (line_len == SERIAL_LINE_SIZE - 1)
            too_long = 1;
        else
            line[line_len++] = c;
    }
    return got;
}

unsigned long serial_signal(void)
{
    return reading ? 1UL << *((unsigned char *)port + MP_SIGBIT) : 0;
}
