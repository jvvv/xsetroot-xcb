/*
 *
Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 */

/*
 * xsetroot.c     MIT Project Athena, X Window System root window 
 *        parameter setting utility.  This program will set 
 *        various parameters of the X root window.
 *
 *  Author:    Mark Lillibridge, MIT Project Athena
 *        11-Jun-87
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_cursor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/bitmaps/gray>
#include "CurUtil.h"
#include "readbitmap.h"

#define Dynamic 1

static char *program_name;
static xcb_connection_t *dpy;
static xcb_screen_t *screen;
static xcb_window_t root;
static int screen_nbr;
static char *fore_color = NULL;
static char *back_color = NULL;
static uint32_t fg_pixel;
static uint32_t bg_pixel;
static int reverse = 0;
static int save_colors = 0;
static int unsave_past = 0;
static xcb_pixmap_t save_pixmap = (xcb_pixmap_t)XCB_NONE;
static const char *cursor_font = "cursor";

static void usage(void);
static const char *GetDisplayName(const char *display_name);
static void FixupState(void);
static void SetBackgroundToBitmap(xcb_pixmap_t bitmap, uint16_t width, uint16_t height);
static xcb_cursor_t CreateCursorFromFiles(char *cursor_file, char *mask_file);
static xcb_cursor_t CreateCursorFromName(char *name);
static xcb_pixmap_t MakeModulaBitmap(int mod_x, int mod_y);
static xcb_coloritem_t NameToColor(char *name, uint32_t pixel);
static uint32_t NameToPixel(char *name, uint32_t pixel);
static xcb_pixmap_t ReadBitmapFile(char *filename, uint16_t *width, uint16_t *height, int16_t *x_hot, int16_t *y_hot);

static void
usage(void)
{
    fprintf(stderr, "usage: %s [options]\n%s\n", program_name,
            "  where options are:\n"
            "  -display <display>   or   -d <display>\n"
            "  -fg <color>   or   -foreground <color>\n"
            "  -bg <color>   or   -background <color>\n"
            "  -rv   or   -reverse\n"
            "  -def   or   -default\n"
            "  -name <string>\n"
            "  -cursor <cursor file> <mask file>\n"
            "  -cursor_name <cursor-font name>\n"
            "  -xcf <ARGB cursor file> <cursor size>\n"
            "  -solid <color>\n"
            "  -gray   or   -grey\n"
            "  -bitmap <filename>\n"
            "  -mod <x> <y>\n"
            "  -help\n"
            "  -version\n"
            );
    exit(1);
    /*NOTREACHED*/
}


int
main(int argc, char *argv[]) 
{
    int excl = 0;
    int nonexcl = 0;
    int restore_defaults = 0;
    char *display_name = NULL;
    char *name = NULL;
    char *cursor_file = NULL;
    char *cursor_mask = NULL;
    char *cursor_name = NULL;
    char *solid_color = NULL;
    char *xcf = NULL;
    int xcf_size = 32;
    xcb_cursor_t cursor;
    int gray = 0;
    char *bitmap_file = NULL;
    int mod_x = 0;
    int mod_y = 0;
    register int i;
    uint16_t ww, hh;
    xcb_pixmap_t bitmap;
    uint32_t params[2];

    program_name=argv[0];

    for (i = 1; i < argc; i++) {
        if (!strcmp ("-display", argv[i]) || !strcmp ("-d", argv[i])) {
            if (++i>=argc) usage ();
            display_name = argv[i];
            continue;
        }
        if (!strcmp("-help", argv[i])) {
            usage();
        }
        if (!strcmp("-version", argv[i])) {
            printf("%s\n", PACKAGE_STRING);
            exit(0);
        }
        if (!strcmp("-def", argv[i]) || !strcmp("-default", argv[i])) {
            restore_defaults = 1;
            continue;
        }
        if (!strcmp("-name", argv[i])) {
            if (++i>=argc) usage();
            name = argv[i];
            nonexcl++;
            continue;
        }
        if (!strcmp("-cursor", argv[i])) {
            if (++i>=argc) usage();
            cursor_file = argv[i];
            if (++i>=argc) usage();
            cursor_mask = argv[i];
            nonexcl++;
            continue;
        }
        if (!strcmp("-cursor_name", argv[i])) {
            if (++i>=argc) usage();
            cursor_name = argv[i];
            nonexcl++;
            continue;
        }
        if (!strcmp("-xcf", argv[i])) {
            if (++i>=argc) usage();
            xcf = argv[i];
            if (++i>=argc) usage();
            xcf_size = atoi(argv[i]);
            if (xcf_size <= 0)
                xcf_size = 32;
            nonexcl++;
            continue;
        }

        if (!strcmp("-fg",argv[i]) || !strcmp("-foreground",argv[i])) {
            if (++i>=argc) usage();
            fore_color = argv[i];
            continue;
        }
        if (!strcmp("-bg",argv[i]) || !strcmp("-background",argv[i])) {
            if (++i>=argc) usage();
            back_color = argv[i];
            continue;
        }
        if (!strcmp("-solid", argv[i])) {
            if (++i>=argc) usage();
            solid_color = argv[i];
            excl++;
            continue;
        }
        if (!strcmp("-gray", argv[i]) || !strcmp("-grey", argv[i])) {
            gray = 1;
            excl++;
            continue;
        }
        if (!strcmp("-bitmap", argv[i])) {
            if (++i>=argc) usage();
            bitmap_file = argv[i];
            excl++;
            continue;
        }
        if (!strcmp("-mod", argv[i])) {
            if (++i>=argc) usage();
            mod_x = atoi(argv[i]);
            if (mod_x <= 0) mod_x = 1;
            if (++i>=argc) usage();
            mod_y = atoi(argv[i]);
            if (mod_y <= 0) mod_y = 1;
            excl++;
            continue;
        }
        if (!strcmp("-rv",argv[i]) || !strcmp("-reverse",argv[i])) {
            reverse = 1;
            continue;
        }
        usage();
    } 

    /* Check for multiple use of exclusive options */
    if (excl > 1) {
    fprintf(stderr, "%s: choose only one of {solid, gray, bitmap, mod}\n",
        program_name);
        usage();
    }

    dpy = xcb_connect(display_name, &screen_nbr);
    if (xcb_connection_has_error(dpy)) {
        fprintf(stderr, "%s:  unable to open display '%s'\n",
                program_name, GetDisplayName(display_name));
        exit(2);
    }
    screen = xcb_aux_get_screen(dpy, screen_nbr);
    root = screen->root;
  
    /* If there are no arguments then restore defaults. */
    if (!excl && !nonexcl)
        restore_defaults = 1;

    /* Handle '-reverse' early to do it only once. */
    if (reverse) {
        if (fore_color) {
            char *temp = fore_color;
            fore_color = back_color;
            back_color = temp;
        }
        fg_pixel = screen->white_pixel;
        bg_pixel = screen->black_pixel;
    }
    else
    {
        fg_pixel = screen->black_pixel;
        bg_pixel = screen->white_pixel;
    }
  
    /* Handle a cursor file */
    if (cursor_file) {
        cursor = CreateCursorFromFiles(cursor_file, cursor_mask);
        if (cursor) {
            xcb_change_window_attributes(dpy, root, XCB_CW_CURSOR, &cursor);
            xcb_free_cursor(dpy, cursor);
        }
        else {
            fprintf(stderr, "%s: Error creating cursor\n", program_name);
            exit(1);
        }
    }
  
    if (cursor_name) {
        cursor = CreateCursorFromName(cursor_name);
        if (cursor)
        {
            xcb_change_window_attributes(dpy, root, XCB_CW_CURSOR, &cursor);
            xcb_free_cursor(dpy, cursor);
        }
        else {
            fprintf(stderr, "%s: Error creating cursor\n", program_name);
            exit(1);
        }
    }
    /* XXX xcb-cursor */
    if (xcf) {
        xcb_cursor_context_t *ctx;

        if (xcb_cursor_context_new(dpy, screen, &ctx) < 0) {
            fprintf(stderr, "%s: Error initializing xcb-cursor\n",
                    program_name);
            exit(2);
        }
        cursor = xcb_cursor_load_cursor(ctx, xcf);
        if (cursor) {
            xcb_change_window_attributes(dpy, root, XCB_CW_CURSOR, &cursor);
            xcb_free_cursor(dpy, cursor);
        }
        else {
            fprintf(stderr, "%s: Error creating cursor\n", program_name);
            exit(1);
        }
    }
/*    
    if (xcf) {
        XcursorImages *images = XcursorFilenameLoadImages(xcf, xcf_size);
        if (!images) {
            fprintf(stderr, "Invalid cursor file \"%s\"\n", xcf);
        } else {
            cursor = XcursorImagesLoadCursor(dpy, images);
            if (cursor)
            {
                xcb_change_window_attributes(dpy, root, XCB_CW_CURSOR, &cursor);
                xcb_free_cursor(dpy, cursor);
            }
        }
    }
*/
    /* Handle -gray and -grey options */
    if (gray) {
        bitmap = xcb_create_pixmap_from_bitmap_data(dpy, root, (uint8_t *)gray_bits,
                        gray_width, gray_height, screen->root_depth,
                        NameToPixel(fore_color, fg_pixel),
                        NameToPixel(back_color, bg_pixel), NULL);
        SetBackgroundToBitmap(bitmap, gray_width, gray_height);
    }
  
    /* Handle -solid option */
    if (solid_color) {
        params[0] = NameToPixel(solid_color, screen->black_pixel);
        xcb_change_window_attributes(dpy, root, XCB_CW_BACK_PIXEL, params);
        xcb_clear_area(dpy, 0, root, 0, 0, 0, 0);
        unsave_past = 1;
    }
  
    /* Handle -bitmap option */
    if (bitmap_file) {
        bitmap = ReadBitmapFile(bitmap_file, &ww, &hh, (int16_t *)NULL, (int16_t *)NULL);
        SetBackgroundToBitmap(bitmap, ww, hh);
    }
  
    /* Handle set background to a modula pattern */
    if (mod_x) {
        bitmap = MakeModulaBitmap(mod_x, mod_y);
        SetBackgroundToBitmap(bitmap, 16, 16);
    }
  
    /* Handle set name */
    if (name)
        xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, root, XCB_ATOM_WM_NAME,
                            XCB_ATOM_STRING, 8, strlen(name), name);
  
    /* Handle restore defaults */
    if (restore_defaults) {
        if (!cursor_file) {
            cursor = 0;
            xcb_change_window_attributes(dpy, root, XCB_CW_CURSOR, &cursor);
        }
        if (!excl) {
            params[0] = XCB_NONE;
            xcb_change_window_attributes(dpy, root, XCB_CW_BACK_PIXMAP, params);
            xcb_clear_area(dpy, 0, root, 0, 0, 0, 0);
            unsave_past = 1;
        }
    }

    xcb_flush(dpy); 
    FixupState();
    xcb_disconnect(dpy);
    exit (0);
}

/* Return a safe string representing for the Display. */
const char *
GetDisplayName(const char *display_name)
{
    const char *name = display_name;

    if (!name) {
        name = getenv("DISPLAY");
        if (!name)
            name = "";
    }
    return name;
}

/* Free past incarnation if needed, and retain state if needed. */
static void
FixupState(void)
{
    xcb_intern_atom_cookie_t ia_c;
    xcb_intern_atom_reply_t *ia_r;
    xcb_get_property_cookie_t gp_c;
    xcb_get_property_reply_t *gp_r;
    xcb_atom_t prop;
    unsigned char *data;

    if (!(xcb_aux_get_visualtype(dpy, screen_nbr, screen->root_visual)->_class & Dynamic))
        unsave_past = 0;
    if (!unsave_past && !save_colors)
        return;
    ia_c = xcb_intern_atom_unchecked(dpy, 0, strlen("_XSETROOT_ID"), "_XSETROOT_ID");
    ia_r = xcb_intern_atom_reply(dpy, ia_c, NULL);
    if (ia_r) {
        prop = ia_r->atom;
        free(ia_r);
    }
    else {
        fprintf(stderr, "%s: error: failed to intern _XSETROOT_ID property atom\n",
                program_name);
        return;
    }

    if (unsave_past) {
        gp_c = xcb_get_property_unchecked(dpy, 0, root, prop, XCB_ATOM_ANY, 0, 1L);
        gp_r = xcb_get_property_reply(dpy, gp_c, NULL);
        if (!gp_r || (gp_r->type != XCB_ATOM_PIXMAP) || (gp_r->format != 32) ||
            (gp_r->length != 1) || (gp_r->bytes_after != 0)) {
            free(gp_r);
            fprintf(stderr, "%s: warning: _XSETROOT_ID property is garbage\n", program_name);
        }
        else {
            data = xcb_get_property_value(gp_r);
            xcb_kill_client(dpy, *((xcb_pixmap_t *)data));
            free(gp_r);
        }
    }
    if (save_colors) {
        if (!save_pixmap) {
            save_pixmap = xcb_generate_id(dpy);
            xcb_create_pixmap(dpy, screen->root_depth, save_pixmap, root, 1, 1);
        }
        xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, root, prop, XCB_ATOM_PIXMAP,
                            32, 1, (void *)&save_pixmap);
        xcb_set_close_down_mode(dpy, XCB_CLOSE_DOWN_RETAIN_PERMANENT);
    }
}

/*
 * SetBackgroundToBitmap: Set the root window background to a caller supplied 
 *                        bitmap.
 */
static void
SetBackgroundToBitmap(xcb_pixmap_t bitmap, uint16_t width, uint16_t height)
{
    xcb_pixmap_t pix;
    xcb_gcontext_t gc;
    uint32_t params[2];

    params[0] = NameToPixel(fore_color, fg_pixel);
    params[1] = NameToPixel(back_color, bg_pixel);
    gc = xcb_generate_id(dpy);
    xcb_create_gc(dpy, gc, root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, params);
    pix = xcb_generate_id(dpy);
    xcb_create_pixmap(dpy, screen->root_depth, pix, root, width, height);
    xcb_copy_plane(dpy, bitmap, pix, gc, 0, 0, 0, 0, width, height, 1);
    xcb_change_window_attributes(dpy, root, XCB_CW_BACK_PIXMAP, &pix);
    xcb_free_gc(dpy, gc);
    xcb_free_pixmap(dpy, bitmap);
    if (save_colors)
        save_pixmap = pix;
    else
        xcb_free_pixmap(dpy, pix);
    xcb_clear_area(dpy, 0, root, 0, 0, 0, 0);
    unsave_past = 1;
}


/*
 * CreateCursorFromFiles: make a cursor of the right colors from two bitmap
 *                        files.
 */
#define BITMAP_HOT_DEFAULT 8

static xcb_cursor_t
CreateCursorFromFiles(char *cursor_file, char *mask_file)
{
    xcb_pixmap_t cursor_bitmap, mask_bitmap;
    uint16_t width, height, ww, hh;
    int16_t x_hot, y_hot;
    xcb_cursor_t cursor;
    xcb_coloritem_t fg, bg;

    fg = NameToColor(fore_color, fg_pixel);
    bg = NameToColor(back_color, bg_pixel);

    cursor_bitmap = ReadBitmapFile(cursor_file, &width, &height, &x_hot, &y_hot);
    mask_bitmap = ReadBitmapFile(mask_file, &ww, &hh, (int16_t *)NULL, (int16_t *)NULL);

    if (width != ww || height != hh) {
        fprintf(stderr, 
                "%s: dimensions of cursor bitmap and cursor mask bitmap are different\n",
                program_name);
       exit(1);
       /*NOTREACHED*/
    }

    if ((x_hot == -1) && (y_hot == -1)) {
        x_hot = BITMAP_HOT_DEFAULT;
        y_hot = BITMAP_HOT_DEFAULT;
    }
    if ((x_hot < 0) || (x_hot >= width) ||
        (y_hot < 0) || (y_hot >= height)) {
        fprintf(stderr, "%s: hotspot is outside cursor bounds\n", program_name);
        exit(1);
        /*NOTREACHED*/
    }

    cursor = xcb_generate_id(dpy);
    xcb_create_cursor(dpy, cursor, cursor_bitmap, mask_bitmap,
                               fg.red, fg.green, fg.blue,
                               bg.red, bg.green, bg.blue, x_hot, y_hot);
    xcb_clear_area(dpy, 0, root, 0, 0, 0, 0);
    xcb_aux_sync(dpy);
    xcb_free_pixmap(dpy, cursor_bitmap);
    xcb_free_pixmap(dpy, mask_bitmap);

    return cursor;
}

static xcb_cursor_t
CreateCursorFromName(char *name)
{
    xcb_void_cookie_t cookie;
    xcb_coloritem_t fg, bg;
    xcb_font_t fid;
    xcb_cursor_t cursor;
    int i;

    fg = NameToColor(fore_color, fg_pixel);
    bg = NameToColor(back_color, bg_pixel);
    i = CursorNameToIndex (name);
    if (i == -1)
        return (xcb_cursor_t) 0;
    fid = xcb_generate_id(dpy);
    cookie = xcb_open_font_checked(dpy, fid, strlen(cursor_font), cursor_font);
    if (xcb_request_check(dpy, cookie))
        return (xcb_cursor_t) 0;
    cursor = xcb_generate_id(dpy);
    xcb_create_glyph_cursor(dpy, cursor, fid, fid, i, i+1,
                            fg.red, fg.green, fg.blue,
                            bg.red, bg.green, bg.blue);
    return cursor;
}

/*
 * MakeModulaBitmap: Returns a modula bitmap based on an x & y mod.
 */
static xcb_pixmap_t 
MakeModulaBitmap(int mod_x, int mod_y)
{
    int i;
    long pattern_line = 0;
    uint8_t modula_data[16*16/8];

    for (i=16; i--; ) {
        pattern_line <<=1;
        if ((i % mod_x) == 0) pattern_line |= 0x0001;
    }
    for (i=0; i<16; i++) {
        if ((i % mod_y) == 0) {
            modula_data[i*2] = (char)0xff;
            modula_data[i*2+1] = (char)0xff;
        } else {
            modula_data[i*2] = pattern_line & 0xff;
            modula_data[i*2+1] = (pattern_line>>8) & 0xff;
        }
    }

    return xcb_create_pixmap_from_bitmap_data(dpy, root, modula_data, 16, 16,
                                              screen->root_depth,
                                              NameToPixel(fore_color, fg_pixel),
                                              NameToPixel(back_color, bg_pixel), NULL);
}


/*
 * NameToColor: Convert the name of a color to its xcb_coloritem_t value.
 */
static xcb_coloritem_t 
NameToColor(char *name, uint32_t pixel)
{
    xcb_query_colors_cookie_t qc_c;
    xcb_query_colors_reply_t *qc_r;
    xcb_lookup_color_cookie_t lc_c;
    xcb_lookup_color_reply_t *lc_r;
    xcb_coloritem_t c;
    xcb_rgb_t *rgb;
    
    if (!name || !*name) {
        c.pixel = pixel;
        qc_c = xcb_query_colors_unchecked(dpy, screen->default_colormap, 1, &pixel);
        qc_r = xcb_query_colors_reply(dpy, qc_c, NULL);
        if (qc_r) {
            rgb = xcb_query_colors_colors(qc_r);
            c.red = rgb->red;
            c.green = rgb->green;
            c.blue = rgb->blue;
            c.flags = XCB_COLOR_FLAG_RED | XCB_COLOR_FLAG_GREEN | XCB_COLOR_FLAG_BLUE;
            free(qc_r);
        }
        else {
            fprintf(stderr, "%s: bad pixel value: %d\n", program_name, pixel);
            exit(1);
        }
    }
    else {
        lc_c = xcb_lookup_color_unchecked(dpy, screen->default_colormap, strlen(name), name);
        lc_r = xcb_lookup_color_reply(dpy, lc_c, NULL);
        if (lc_r) {
            c.red = lc_r->exact_red;
            c.green = lc_r->exact_green;
            c.blue = lc_r->exact_blue;
            c.flags = XCB_COLOR_FLAG_RED | XCB_COLOR_FLAG_GREEN | XCB_COLOR_FLAG_BLUE;
            free(lc_r);
        }
        else {
            fprintf(stderr, "%s: unknown color or bad color format: %s\n", program_name, name);
            exit(1);
        }
        /*NOTREACHED*/
    }
    return(c);
}

static uint32_t 
NameToPixel(char *name, uint32_t pixel)
{
    xcb_lookup_color_cookie_t lc_c;
    xcb_lookup_color_reply_t *lc_r;
    xcb_alloc_color_cookie_t ac_c;
    xcb_alloc_color_reply_t *ac_r;
    xcb_coloritem_t ecolor;

    if (!name || !*name)
        return pixel;
    lc_c = xcb_lookup_color_unchecked(dpy, screen->default_colormap, strlen(name), name);
    lc_r = xcb_lookup_color_reply(dpy, lc_c, NULL);
    if (lc_r) {
        ecolor.red = lc_r->exact_red;
        ecolor.green = lc_r->exact_green;
        ecolor.blue = lc_r->exact_blue;
        ecolor.flags = XCB_COLOR_FLAG_RED | XCB_COLOR_FLAG_GREEN | XCB_COLOR_FLAG_BLUE;
        free(lc_r);
    }
    else {
        fprintf(stderr, "%s: unknown color \"%s\"\n", program_name, name);
        exit(1);
        /*NOTREACHED*/
    }

    ac_c = xcb_alloc_color_unchecked(dpy, screen->default_colormap,
                                     ecolor.red, ecolor.green, ecolor.blue);
    ac_r = xcb_alloc_color_reply(dpy, ac_c, NULL);
    if (ac_r) {
        ecolor.pixel = ac_r->pixel;
        ecolor.red = ac_r->red;
        ecolor.green = ac_r->green;
        ecolor.blue = ac_r->blue;
    }
    else {
        fprintf(stderr, "%s:  unable to allocate color for \"%s\"\n",
                program_name, name);
        exit(1);
        /*NOTREACHED*/
    }

    if ((ecolor.pixel != screen->black_pixel) &&
        (ecolor.pixel != screen->white_pixel) &&
        (xcb_aux_get_visualtype(dpy, screen_nbr, screen->root_visual)->_class & Dynamic))
        save_colors = 1;

    return ecolor.pixel;
}

static xcb_pixmap_t 
ReadBitmapFile(char *filename, uint16_t *width, uint16_t *height, 
               int16_t *x_hot, int16_t *y_hot)
{
    uint8_t *data;
    int status;

    status = read_bitmap_data_from_file(filename, &data, width, height, x_hot, y_hot);
    if (status == BitmapSuccess)
        /* Is this a memory leak? Does data need to be free'd before return? */
        return xcb_create_pixmap_from_bitmap_data(dpy, root, data, *width, *height, 1,
                                                  NameToPixel(fore_color, fg_pixel),
                                                  NameToPixel(back_color, bg_pixel), NULL);
    else if (status == BitmapOpenFailed)
        fprintf(stderr, "%s: can't open file: %s\n", program_name, filename);
    else if (status == BitmapReadFailed)
        fprintf(stderr, "%s: error reading file: %s\n", program_name, filename);
    else if (status == BitmapFileInvalid)
        fprintf(stderr, "%s: bad bitmap format file: %s\n", program_name, filename);
    exit(1);
    /*NOTREACHED*/
}
/* vim: set ts=4 sw=4 et cindent: */
