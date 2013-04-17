/* bitmap_to_pixmap.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
//#include <limits.h>
#include <errno.h>
#include <err.h>
#include "readbitmap.h"

#define MAXREAD     4096
#define XBM_X10     1
#define XBM_X11     2

static void *xalloc(size_t sz)
{
    void *value = calloc(1, sz ? sz : 1);
    if (!value)
        err(EXIT_FAILURE, NULL);
    return value;
}

static void *xrealloc(void *ptr, size_t sz)
{
    void *value;
    if (!ptr)
        value = calloc(1, sz ? sz : 1);
    else
        value = realloc(ptr, sz ? sz : 1);
    if (!value)
        err(EXIT_FAILURE, NULL);
    return value;
}

static char *eatwhite(char *s)
{
    if (!s || !*s)
        return NULL;
    while (*s && ((*s == ' ') || (*s == '\t'))) ++s;
    return s;
}

int read_bitmap_data_from_file(const char *fname,
                               uint8_t **data_ret,
                               uint16_t *width_ret, uint16_t *height_ret,
                               int16_t *xhot_ret, int16_t *yhot_ret)
{
    int fd, rd, cnt, i, j, value;
    char **lines, *line, *rbuf, *fbuf = NULL;
    char name[80], type[80];
    char *t, *p[20];
    int width, height, xhot, yhot, format;
    int length, padding = 0, bytesperline, bytes = 0;
    long nexti;
    char *tailp;
    uint8_t *data = NULL;

    if (!fname || ((fd = open(fname, O_RDONLY)) == -1))
        return BitmapOpenFailed;

    /* prep read vars and allocate read buffer */
    rd = cnt = 0;
    rbuf = xalloc(MAXREAD);

    /* read in file */
    while ((rd = read(fd, rbuf, MAXREAD)))
    {
        if (rd == -1)
        {
            if (errno == EAGAIN || errno == EINTR)
                /* this should probably pause first */
                continue;
            close(fd);
            free(rbuf);
            free(fbuf);
            return BitmapReadFailed;
        }
        fbuf = (char *)xrealloc(fbuf, cnt + rd + 1);
        memcpy(fbuf+cnt, rbuf, rd);
        cnt += rd;
    }

    free(rbuf);
    close(fd);

    /* make a list of lines */
    lines = (char **)xalloc(sizeof(char *) * 20);
    lines[0] = strtok(fbuf, "\n");
    for (cnt = 0; lines[cnt]; cnt++, lines[cnt] = strtok(NULL, "\n"))
        if (cnt && !(cnt % 20))
            lines = (char **)xrealloc(lines, sizeof(char *) * (cnt + 20));
    lines = (char **)xrealloc(lines, sizeof(char *) * cnt);

    width = height = format = 0;
    xhot = yhot = -1;

    /* parse bitmap header */
    for (i = 0; i < cnt; i++)
    {
        if (!(line = eatwhite(lines[i])))
            continue;
        if (sscanf(line, "#define %s %d", name, &value) == 2)
        {
            p[0] = strtok(name, "_");
            for (j = 0; j < 20 && p[j]; j++, p[j] = strtok(NULL, "_")) ;
            if (j)
            {
                if (!strcmp(p[j-1], "width"))
                    width = value;
                else if (!strcmp(p[j-1], "height"))
                    height = value;
                else if ((j >= 2) && !strcmp(p[j-1], "hot"))
                {
                    if (!strcmp(p[j-2], "x"))
                        xhot = value;
                    else if (!strcmp(p[j-2], "y"))
                        yhot = value;
                }
            }
            continue;
        }
        if ((sscanf(line, "static unsigned %s %s = {", type, name) == 2)
            || (sscanf(line, "static %s %s = {", type, name) == 2))
        {
            if (!strcmp(type, "short"))
                format = XBM_X10;
            else if (!strcmp(type, "char"))
                format = XBM_X11;

            if (!(t = strrchr(name, '_')))
                t = name;
            else
                ++t;
            if (strcmp(t, "bits[]"))
                continue;

            break;
        }
    }

    if (!width || !height || !format)
    {
        free(fbuf);
        free(lines);
        return BitmapFileInvalid;
    }

    if ((format & XBM_X10) && (width % 16) && ((width % 16) < 9))
        padding++;
    bytesperline = (width + 7) / 8 + padding;
    length = bytesperline * height;
    data = xalloc(length);

    /* parse bitmap data */
    for (; i < cnt; i++)
    {
        if (!(line = eatwhite(lines[i])))
            continue;

        while(*line)
        {
            errno = 0;
            nexti = strtoul(line, &tailp, 0);
            if (!tailp || errno)
                break;
            if (tailp == line)
            {
                ++line;
                continue;
            }
            line = tailp;
            data[bytes++] = nexti;
            if ((format == XBM_X10) && (!padding || ((bytes + 2) % bytesperline)))
                data[bytes++] = nexti >> 8;
            if (*line == ',')
                ++line;
        }
        if (*line)
            break;
    }

    free(fbuf);
    free(lines);

    if (bytes < length)
    {
        free(data);
        return BitmapFileInvalid;
    }

    *data_ret = data;
    *width_ret = width;
    *height_ret = height;
    if (xhot_ret)
        *xhot_ret = xhot;
    if (yhot_ret)
        *yhot_ret = yhot;

    return BitmapSuccess;
}
/* vim:set ts=4 sw=4 et cindent:*/
