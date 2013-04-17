/* readbitmap.h */

#ifndef _readbitmap_h
#define _readbitmap_h

#define BitmapSuccess       0
#define BitmapOpenFailed    1
#define BitmapReadFailed    2
#define BitmapFileInvalid   3

extern int read_bitmap_data_from_file(const char *fname,
                               uint8_t **data,
                               uint16_t *width, uint16_t *height,
                               int16_t *xhot, int16_t *yhot);

#endif/*!_readbitmap_h*/

/* vim: set ts=4 sw=4 et cindent: */
