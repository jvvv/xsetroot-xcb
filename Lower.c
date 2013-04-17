/*

Copyright 1988, 1998  The Open Group

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

/* Taken from libXmu */

#define  XK_LATIN1
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/keysymdef.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * ISO Latin-1 case conversion routine
 */
#define Tolower(c)                                                       \
((c) >= XK_a && (c) <= XK_z ?                                            \
 (c) : (c) >= XK_A && (c) <= XK_Z ?                                      \
 (c) + (XK_a - XK_A) : (c) >= XK_Agrave && (c) <= XK_Odiaeresis ?        \
 (c) + (XK_agrave - XK_Agrave) : (c) >= XK_Ooblique && (c) <= XK_Thorn ? \
 (c) + (XK_oslash - XK_Ooblique) :                                       \
 (c))

/*
 * Implementation
 */
void
CopyISOLatin1Lowered(char *dst, const char *src)
{
    register unsigned char *dest, *source;

    for (dest = (unsigned char *)dst, source = (unsigned char *)src;
         *source;
         source++, dest++)
        *dest = Tolower(*source);
    *dest = '\0';
}
/* vim: set ts=4 sw=4 et cindent: */
