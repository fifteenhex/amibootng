/*
 * Tux on a custom screen, with the boot menu below it.  The logo is the
 * kernel's 16-colour one (drivers/video/logo/logo_linux_vga16.ppm), drawn
 * by Larry Ewing (lewing@isc.tamu.edu) using The GIMP.
 */
#include <stdio.h>
#include <string.h>
#include "amigaos.h"
#include "lvo.h"
#include "splash.h"
#include "util.h"
#include "logo.h"

static void *sysbase, *intuitionbase, *gfxbase;

#define OpenLibrary(n, v)       ((void *)LP2(-552, sysbase,             \
        const char *, n, a1, long, v, d0))
#define CloseLibrary(l)         LP1(-414, sysbase, void *, l, a1)
#define GetMsg(port)            ((void *)LP1(-372, sysbase, void *, port, a0))
#define ReplyMsg(m)             LP1(-378, sysbase, void *, m, a1)
#define OpenScreenTagList(t)    ((void *)LP2(-612, intuitionbase,       \
        void *, NULL, a0, const unsigned long *, t, a1))
#define CloseScreen(s)          LP1(-66, intuitionbase, void *, s, a0)
#define OpenWindowTagList(t)    ((void *)LP2(-606, intuitionbase,       \
        void *, NULL, a0, const unsigned long *, t, a1))
#define CloseWindow(w)          LP1(-72, intuitionbase, void *, w, a0)
#define ScreenToFront(s)        LP1(-252, intuitionbase, void *, s, a0)
#define ActivateWindow(w)       LP1(-450, intuitionbase, void *, w, a0)
#define TextLength(rp, s, n)    ((short)LP3(-54, gfxbase,               \
        void *, rp, a1, const char *, s, a0, long, n, d0))
#define Text(rp, s, n)          LP3(-60, gfxbase,                       \
        void *, rp, a1, const char *, s, a0, long, n, d0)
#define SetRast(rp, pen)        LP2(-234, gfxbase, void *, rp, a1, long, pen, d0)
#define Move(rp, x, y)          LP3(-240, gfxbase,                      \
        void *, rp, a1, long, x, d0, long, y, d1)
#define RectFill(rp, x0, y0, x1, y1) LP5(-306, gfxbase, void *, rp, a1, \
        long, x0, d0, long, y0, d1, long, x1, d2, long, y1, d3)
#define SetAPen(rp, pen)        LP2(-342, gfxbase, void *, rp, a1, long, pen, d0)
#define SetDrMd(rp, mode)       LP2(-354, gfxbase, void *, rp, a1, long, mode, d0)

/* struct Screen, Window, IntuiMessage and RastPort fields */
#define SC_WIDTH                12
#define SC_HEIGHT               14
#define WD_RPORT                50
#define WD_USERPORT             86
#define MP_SIGBIT               15
#define IM_CLASS                20
#define IM_CODE                 24
#define IM_MOUSEY               34
#define RP_TXHEIGHT             58
#define RP_TXBASELINE           62

#define STDSCREENWIDTH          -1
#define STDSCREENHEIGHT         -1
#define SA_Width                0x80000023
#define SA_Height               0x80000024
#define SA_Depth                0x80000025
#define SA_Colors               0x80000029
#define SA_DisplayID            0x80000032
#define SA_ShowTitle            0x80000036
#define SA_Quiet                0x80000038
#define WA_Width                0x80000066
#define WA_Height               0x80000067
#define WA_IDCMP                0x8000006A
#define WA_Flags                0x8000006B
#define WA_CustomScreen         0x80000070

#define WFLG_SIMPLE_REFRESH     0x00000040
#define WFLG_BACKDROP           0x00000100
#define WFLG_BORDERLESS         0x00000800
#define WFLG_ACTIVATE           0x00001000
#define WFLG_RMBTRAP            0x00010000
#define WFLG_NOCAREREFRESH      0x00020000
#define IDCMP_MOUSEBUTTONS      0x00000008
#define IDCMP_RAWKEY            0x00000400

#define SELECTDOWN              0x68
#define MENUDOWN                0x69
#define KEY_ENTER               0x43
#define KEY_RETURN              0x44
#define KEY_ESC                 0x45
#define KEY_UP                  0x4C
#define KEY_DOWN                0x4D

#define JAM1                    0
#define PEN_BLACK               0       /* fixed by tools/logo.py */
#define PEN_WHITE               1
#define PEN_BAR                 2

#define LOGO_GAP                8       /* between the logo and the text */

struct colorspec {
    short index;
    unsigned short red, green, blue;
};

static void *screen, *window, *rport;
static int scale, logo_x, logo_y, text_y, line_h, logo = -1;
static int bar_end = -1;        /* how far the progress bar is filled */

static unsigned short field16(void *p, int off)
{
    return *(unsigned short *)((char *)p + off);
}

static void *field_ptr(void *p, int off)
{
    return *(void **)((char *)p + off);
}

void splash_logo(int which)
{
    static const int index[] = { [SPLASH_TUX] = LOGO_TUX,
                                 [SPLASH_BOING] = LOGO_BOING };
    const unsigned char (*pixels)[LOGO_WIDTH] = logo_pixels[index[which]];
    int x, y, end;

    if (!screen || !scale || which == logo)
        return;
    logo = which;
    trace("splash: logo %d", which);
    SetAPen(rport, PEN_BLACK);
    RectFill(rport, logo_x, logo_y, logo_x + LOGO_WIDTH * scale - 1,
             logo_y + LOGO_HEIGHT * scale - 1);
    for (y = 0; y < LOGO_HEIGHT; y++) {
        for (x = 0; x < LOGO_WIDTH; x = end) {
            int pen = pixels[y][x];

            for (end = x + 1; end < LOGO_WIDTH && pixels[y][end] == pen;
                 end++)
                ;
            if (pen == PEN_BLACK)
                continue;
            SetAPen(rport, pen);
            RectFill(rport, logo_x + x * scale, logo_y + y * scale,
                     logo_x + end * scale - 1, logo_y + (y + 1) * scale - 1);
        }
    }
    trace("splash: logo drawn");
}

/* text line n below the logo, centred, optionally on a bar */
static void draw_line(int n, const char *s, int bar)
{
    int top = text_y + n * line_h;
    int w = field16(screen, SC_WIDTH);
    int len = strlen(s);

    while (len && TextLength(rport, s, len) > w)
        len--;

    SetAPen(rport, PEN_BLACK);
    RectFill(rport, 0, top, w - 1, top + line_h - 1);
    if (bar) {
        SetAPen(rport, PEN_BAR);
        RectFill(rport, w / 8, top, w - w / 8 - 1, top + line_h - 1);
    }
    SetAPen(rport, bar ? PEN_BLACK : PEN_WHITE);
    Move(rport, (w - TextLength(rport, s, len)) / 2,
         top + field16(rport, RP_TXBASELINE) + 1);
    Text(rport, s, len);
}

static void *open_window(int w, int h)
{
    unsigned long tags[] = {
        WA_CustomScreen, (unsigned long)screen, WA_Width, w, WA_Height, h,
        WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY,
        WA_Flags, WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE |
                  WFLG_RMBTRAP | WFLG_SIMPLE_REFRESH | WFLG_NOCAREREFRESH,
        0
    };

    return OpenWindowTagList(tags);
}

int splash_open(int lines)
{
    static struct colorspec colours[LOGO_COLOURS + 1];
    static unsigned long screen_tags[] = {
        SA_Width, STDSCREENWIDTH, SA_Height, STDSCREENHEIGHT,
        SA_Depth, LOGO_DEPTH, SA_DisplayID, 0, SA_ShowTitle, 0, SA_Quiet, 1,
        SA_Colors, (unsigned long)colours, 0
    };
    int i, w, h, text_h;

    sysbase = amigaos_execbase();
    intuitionbase = OpenLibrary("intuition.library", 36);
    gfxbase = OpenLibrary("graphics.library", 36);
    if (!intuitionbase || !gfxbase)
        goto fail;

    for (i = 0; i < LOGO_COLOURS; i++) {
        colours[i].index = i;
        colours[i].red = logo_palette[i][0];
        colours[i].green = logo_palette[i][1];
        colours[i].blue = logo_palette[i][2];
    }
    colours[i].index = -1;

    trace("splash: OpenScreen");
    screen = OpenScreenTagList(screen_tags);
    trace("splash: screen %p", screen);
    if (!screen)
        goto fail;
    w = field16(screen, SC_WIDTH);
    h = field16(screen, SC_HEIGHT);
    window = open_window(w, h);
    trace("splash: window %p, %dx%d", window, w, h);
    if (!window)
        goto fail;

    rport = field_ptr(window, WD_RPORT);
    line_h = field16(rport, RP_TXHEIGHT) + 2;
    text_h = lines * line_h;
    if (text_h > h)
        goto fail;
    SetRast(rport, PEN_BLACK);
    SetDrMd(rport, JAM1);

    /* the logo doubled, as it is, or not at all: whatever fits */
    scale = 2;
    if (h < (LOGO_HEIGHT + LOGO_GAP) * 2 + text_h)
        scale = 1;
    if (h < LOGO_HEIGHT + LOGO_GAP + text_h)
        scale = 0;
    logo_x = (w - LOGO_WIDTH * scale) / 2;
    logo_y = (h - (LOGO_HEIGHT + LOGO_GAP) * scale - text_h) / 2;
    text_y = logo_y + (LOGO_HEIGHT + LOGO_GAP) * scale;
    splash_logo(SPLASH_TUX);
    quiet_console(1);
    return 0;

fail:
    splash_close();
    return -1;
}

void splash_status(const char *s)
{
    if (screen) {
        SetAPen(rport, PEN_BLACK);
        RectFill(rport, 0, text_y, field16(screen, SC_WIDTH) - 1,
                 field16(screen, SC_HEIGHT) - 1);
        draw_line(0, s, 0);
        bar_end = -1;
    }
}

/* the next IDCMP message's class, code and mouse y; 0 if there is none */
static int get_event(unsigned long *class, int *code, int *y)
{
    void *msg = GetMsg(field_ptr(window, WD_USERPORT));

    if (!msg)
        return 0;
    *class = *(unsigned long *)((char *)msg + IM_CLASS);
    *code = field16(msg, IM_CODE);
    *y = (short)field16(msg, IM_MOUSEY);
    ReplyMsg(msg);
    return 1;
}

void splash_progress(unsigned long done, unsigned long total)
{
    int top = text_y + line_h, w, x0, x1, end;

    if (!screen || !total)
        return;
    w = field16(screen, SC_WIDTH);
    x0 = w / 8;
    x1 = w - w / 8 - 1;
    if (bar_end < 0) {
        SetAPen(rport, PEN_WHITE);
        RectFill(rport, x0, top + 1, x1, top + line_h - 2);
        SetAPen(rport, PEN_BLACK);
        RectFill(rport, x0 + 1, top + 2, x1 - 1, top + line_h - 3);
        bar_end = x0;
    }
    if (done > total)
        done = total;
    /* in 256-byte units the product stays well inside 32 bits */
    end = x0 + (x1 - x0 - 1) * (done >> 8) / ((total >> 8) + 1);
    if (end > bar_end) {
        SetAPen(rport, PEN_BAR);
        RectFill(rport, bar_end + 1, top + 2, end, top + line_h - 3);
        bar_end = end;
    }
}

void splash_draw_menu(const char *const *labels, int n, int sel,
                      const char *status)
{
    int i;

    if (!screen)
        return;
    trace("splash: draw_menu");
    for (i = 0; i < n; i++)
        draw_line(i, labels[i], i == sel);
    draw_line(n, status, 0);
    draw_line(n + 1, "Esc or right button: Workbench", 0);
}

enum menu_event splash_event(int *entry)
{
    unsigned long class;
    int code, y;

    while (screen && get_event(&class, &code, &y)) {
        if (class == IDCMP_RAWKEY) {
            if (code == KEY_RETURN || code == KEY_ENTER)
                return MENU_BOOT;
            if (code == KEY_ESC)
                return MENU_CANCEL;
            if (code == KEY_UP)
                return MENU_UP;
            if (code == KEY_DOWN)
                return MENU_DOWN;
            if (code < 0x80)
                return MENU_KEY;
        } else if (class == IDCMP_MOUSEBUTTONS) {
            if (code == MENUDOWN)
                return MENU_CANCEL;
            if (code != SELECTDOWN)
                continue;
            *entry = y >= text_y ? (y - text_y) / line_h : -1;
            return MENU_PICK;
        }
    }
    return MENU_NONE;
}

unsigned long splash_signal(void)
{
    return screen ? 1UL << *((unsigned char *)field_ptr(window, WD_USERPORT)
                             + MP_SIGBIT) : 0;
}

int splash_error(const char *s)
{
    if (!screen)
        return 0;
    /* the boot console may have opened in front of us */
    ScreenToFront(screen);
    ActivateWindow(window);
    splash_status(s);
    draw_line(1, "Press a key to go to Workbench", 0);
    return 1;
}

void splash_close(void)
{
    if (window)
        CloseWindow(window);
    if (screen)
        CloseScreen(screen);
    if (gfxbase)
        CloseLibrary(gfxbase);
    if (intuitionbase)
        CloseLibrary(intuitionbase);
    window = screen = gfxbase = intuitionbase = NULL;
    logo = -1;
    quiet_console(0);
}
