#include "internal.h"
#include <ft2build.h>
#include FT_FREETYPE_H

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXT

static char *characters =  "abcdefghijklmnopqrstuvwxyz" \
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                           "1234567890~!@#$%^&*()-=+;:" \
                           "äöÄÖあいしてる"             \
                           "'\",./?[]|\\ <>`\xFF";

/* \brief glyph data */
typedef struct _glhckGlyph {
   int x, y, width;
   unsigned int code;
} _glhckGlyph;

/* \brief font data */
typedef struct _glhckFont {
   int width, height, maxASDE;
   unsigned int numGlyphs;
   struct _glhckGlyph *glyph;
   unsigned char *data;
} _glhckFont;

/* Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details. */

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const char utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

unsigned int inline
decode(unsigned int* state, unsigned int* codep, unsigned int byte) {
  unsigned int type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state*16 + type];
  return *state;
}

/* \brief create internal font */
static int _glhckFontNew(const char *font_path, size_t pixel_size, _glhckFont *font)
{
   char *s;
   unsigned char *data = NULL;
   FT_Face face = NULL;
   FT_Library freetype;
   _glhckGlyph *glyph = NULL;
   unsigned int codepoint, state = 0, char_index, i = 0, row, pixel;
   int image_width = 256, image_height = 0, margin = 4, x, y;
   int max_descent = 0, max_ascent = 0, target_height = 0;
   int space_on_line = image_width - margin, lines = 1, advance;

   CALL("%s, %zu", font_path, pixel_size);
   assert(font);

   /* init */
   if (FT_Init_FreeType(&freetype) != 0)
      goto fail;

   /* load font */
   if (FT_New_Face(freetype, font_path, 0, &face) != 0)
      goto fail;

   /* check if selected font is good */
   if (!(face->face_flags & FT_FACE_FLAG_SCALABLE) ||
       !(face->face_flags & FT_FACE_FLAG_HORIZONTAL))
      goto fail;

   /* set font size */
   FT_Set_Pixel_Sizes(face, pixel_size, 0);

   /* figure out dimensions needed for this */
   i = 0;
   for (s = characters; *s; ++s) {
      if (decode(&state, &codepoint, *(unsigned char*)s) != UTF8_ACCEPT)
         continue;

      /* get freetype gluph */
      char_index = FT_Get_Char_Index(face, codepoint);
      FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT);
      FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

      /* calculate how much to advance */
      advance = (face->glyph->metrics.horiAdvance >> 6) + margin;

      /* check if new line should be started */
      if (advance > space_on_line) {
         space_on_line = image_width - margin;
         ++lines;
      }
      space_on_line -= advance;

      /* calculate max ascent && descent */
      max_ascent = face->glyph->bitmap_top>max_ascent?
         face->glyph->bitmap_top:max_ascent;
      max_descent = face->glyph->bitmap.rows-face->glyph->bitmap_top>max_descent?
         face->glyph->bitmap.rows-face->glyph->bitmap_top:max_descent;

      ++i;
   }

   /* target height */
   target_height = (max_ascent + max_descent + margin) * lines + margin;
   for (image_height = 16; image_height < target_height; image_height *= 2);

   /* allocate data */
   if (!(data = _glhckMalloc(image_height * image_width * sizeof(unsigned char))))
      goto fail;

   if (!(glyph = _glhckMalloc(i * sizeof(_glhckGlyph))))
      goto fail;

   /* init */
   memset(data, 0, image_height * image_width * sizeof(unsigned char));
   memset(glyph, 0, i * sizeof(_glhckGlyph));

   /* draw glyphs */
   i = 0; state = 0;
   for (x = margin, y = margin + max_ascent, s = characters; *s; ++s) {
      if (decode(&state, &codepoint, *(unsigned char*)s) != UTF8_ACCEPT)
         continue;

      /* get freetype glyph */
      char_index = FT_Get_Char_Index(face, codepoint);
      FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT);
      FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

      /* calcualte how much to advance */
      advance = (face->glyph->metrics.horiAdvance >> 6) + margin;

      /* check if new line should be started */
      if (advance > image_width - x) {
         x = margin;
         y += (max_ascent + max_descent + margin);
      }

      /* store glyph information */
      glyph[i].code = codepoint;
      glyph[i].width = advance - 3;
      glyph[i].x = x;
      glyph[i].y = y - max_ascent;

      /* fill bitmap data */
      for (row = 0; row != face->glyph->bitmap.rows; ++row) {
         for (pixel = 0; pixel < face->glyph->bitmap.width; ++pixel)
            data[(x + face->glyph->bitmap_left + pixel) +
                 (y - face->glyph->bitmap_top  + row) * image_width] =
            face->glyph->bitmap.buffer[pixel + row * face->glyph->bitmap.pitch];
      }

      /* iterate && advance */
      ++i;
      x += advance;
   }

   FT_Done_FreeType(freetype);

   /* fill data */
   font->width     = image_width;
   font->height    = image_height;
   font->maxASDE   = max_ascent + max_descent;
   font->numGlyphs = i;
   font->glyph     = glyph;
   font->data      = data;

   RET("%d\n", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(free, face);
   IFDO(_glhckFree, data);
   IFDO(_glhckFree, glyph);
   RET("%d\n", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief free font data */
static void _glhckFontFree(_glhckFont *font)
{
   CALL("%p", font);
   assert(font);
   _glhckFree(font->data);
   _glhckFree(font->glyph);
}

/* public api */
#include <GL/gl.h>

/* \brief create new text object */
GLHCKAPI glhckObject* glhckTextNew(const char *font_path, size_t pixel_size)
{
   _glhckTexture  *texture = NULL;
   _glhckObject   *font = NULL;
   _glhckFont     fontData;
   CALL("%s, %zu", font_path, pixel_size);
   assert(font_path);

   /* load font, generate atlas glyph */
   if (_glhckFontNew(font_path, pixel_size, &fontData)
         != RETURN_OK)
      goto fail;

   /* create texture */
   if (!(texture = glhckTextureNew(NULL, 0)))
      goto fail;

   printf("%dx%d\n", fontData.width, fontData.height);

   /* fill texture with data */
   if (glhckTextureCreate(texture, fontData.data,
            fontData.width, fontData.height, GLHCK_ALPHA, 0)
         != RETURN_OK)
      goto fail;

   /* create font object */
#if 0
   if (!(font = glhckObjectNew()))
      goto fail;
#else
   if (!(font = glhckPlaneNew(8)))
      goto fail;
   glhckObjectScalef(font, (float)texture->width/8, (float)texture->height/8, 1);
   glhckObjectRotatef(font, 0, 0, 180);
#endif

   _glhckObjectSetFile(font, font_path);
   glhckObjectSetTexture(font, texture);
   glhckTextureFree(texture);

   RET("%p", font);
   return font;

fail:
   IFDO(glhckObjectFree, font);
   IFDO(glhckTextureFree, texture);
   RET("%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
