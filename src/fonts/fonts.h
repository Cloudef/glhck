#ifndef __fonts_h__
#define __fonts_h__

#include "../internal.h"

/* \brief font info struct
 * Support only monospaced bitmap fonts.
 * The code-point for glyph is generated using algorithm: (row*columns+column)
 * and is encoded to UTF8 for internal text system.
 *
 * The algorithm for glyph position in pixels is column*fontSize, row*fontSize
 *
 * Thefore glyph at c/r (3x3) from total c/r (3x3) would equal to character
 * 3*3+3 = 12, and the position x/y at pixel size 12 would be 12*3 = 36 = 36,36 */
typedef struct _glhckBitmapFontInfo {
   const char *name;
   short fontSize;
   int ascent, descent, lineGap;
   unsigned int columns, rows;
   glhckTextureFormat format;
   glhckDataType type;
   int width, height;
   const void *data;
} _glhckBitmapFontInfo;

#endif /* __fonts_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
