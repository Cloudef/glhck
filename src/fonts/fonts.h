#ifndef __fonts_h__
#define __fonts_h__

/* \brief array struct element for internal font bitmap font mapping */
typedef struct _glhckBitmapGlyph {
   const char *character;
   int x, y, w, h;
} _glhckBitmapGlyph;

/* \brief font info struct */
typedef struct _glhckBitmapFontInfo {
   const char *name;
   int fontSize, ascent, descent, lineGap;
   const _glhckBitmapGlyph *glyphs;
   glhckTextureFormat format;
   glhckDataType type;
   int width, height;
   const void *data;
} _glhckBitmapFontInfo;

#endif /* __fonts_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
