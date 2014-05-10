#include "internal.h"
#include "lut/lut.h"
#include <limits.h> /* for SHRT_MAX */
#include <stdio.h>

/* builtin fonts */
#include "fonts/fonts.h"
#include "fonts/kakwafont.h"

/* traching channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXT

/* stb_truetype */
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)  _glhckMalloc(x)
#define STBTT_free(x,u)    _glhckFree(x)
#include "helper/stb_truetype.h"

#define GLHCK_FONT_LUT_SIZE 256
#define GLHCK_TEXT_VERT_COUNT (6 * 128)

#define GLHCK_INVALID_GLYPH (chckPoolIndex)-1
#define GLHCK_INVALID_TEXTURE (chckPoolIndex)-1

/* \brief font types */
typedef enum _glhckTextFontType {
   GLHCK_FONT_TTF,
   GLHCK_FONT_BMP,
} _glhckTextFontType;

/* \brief quad helper struct */
typedef struct __GLHCKtextQuad {
#if GLHCK_TEXT_FLOAT_PRECISION /* floats ftw */
   glhckVector2f v1, v2;
   glhckVector2f t1, t2;
#else /* short version */
   glhckVector2s v1, v2;
   glhckVector2s t1, t2;
#endif
} __GLHCKtextQuad;

/* \brief glyph object */
typedef struct __GLHCKtextGlyph
{
   chckPoolIndex textureId;
   chckPoolIndex next;
   float xadv, xoff, yoff;
   unsigned int code;
   int x1, y1, x2, y2;
   short size;
} __GLHCKtextGlyph;

/* \brief font object */
typedef struct __GLHCKtextFont
{
   struct stbtt_fontinfo font;
   chckPoolIndex textureId; /* only used on bitmap fonts */
   chckLut *lut;
   chckPool *glyphs;
   void *data;
   float ascender, descender, lineHeight;
   _glhckTextFontType type;
} __GLHCKtextFont;

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

/* utf8 table */
static const unsigned char utf8d[] = {
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

/* decode utf8 */
static unsigned int decutf8(unsigned int *state, unsigned int *codep, unsigned int byte)
{
   unsigned int type = utf8d[byte];
   *codep = (*state != UTF8_ACCEPT ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte));
   *state = utf8d[256 + *state*16 + type];
   return *state;
}

/* \brief get utf8 encoded length */
static int encutf8len(unsigned int ch) {
   if (ch < 0x80) return 1;
   if (ch < 0x800) return 2;
   if (ch < 0x10000) return 3;
   return 4;
}

/* \brief encode code point to utf8 */
static int encutf8(unsigned int ch, char *buffer, int bufferSize) {
   unsigned char *str = (unsigned char *)buffer;

   int i, j;
   if ((i = j = encutf8len(ch)) == 1) {
      *str = ch;
      return 1;
   }

   if (bufferSize < i)
      return 0;

   for (; j > 1; j--) str[j-1] = 0x80 | (0x3F & (ch >> ((i - j) * 6)));
   *str = (~0) << (8 - i);
   *str |= (ch >> (i * 6 - 6));
   return i;
}

static int _glhckTextGeometryAllocateMore(__GLHCKtextGeometry *geometry)
{
   int newCount = geometry->vertexCount + GLHCK_TEXT_VERT_COUNT;

#if GLHCK_TEXT_FLOAT_PRECISION
   glhckVertexData2f *newData;
   if (!(newData = _glhckRealloc(geometry->vertexData, geometry->allocatedCount, newCount, sizeof(glhckVertexData2f))))
      goto fail;
#else
   glhckVertexData2s *newData;
   if (!(newData = _glhckRealloc(geometry->vertexData, geometry->allocatedCount, newCount, sizeof(glhckVertexData2s))))
      goto fail;
#endif

   geometry->vertexData = newData;
   geometry->allocatedCount = newCount;
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief creates new texture to the cache */
static __GLHCKtextTexture* _glhckTextTextureNew(glhckText *object, chckPoolIndex *id, glhckTextureFormat format, glhckDataType type, const void *data)
{
   glhckTexture *texture;
   chckIterPool *rowsPool = NULL;
   CALL(0, "%p, %u, %u, %p", object, format, type, data);
   assert(object);

   if (!(texture = glhckTextureNew()))
      goto fail;

   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, object->cacheWidth, object->cacheHeight, 0, 0, format, type, 0, data) != RETURN_OK)
      goto fail;

   /* set default params */
   glhckTextureParameter(texture, glhckTextureDefaultLinearParameters());

   if (!(rowsPool = chckIterPoolNew(32, 32, sizeof(__GLHCKtextTextureRow))))
      goto fail;

   __GLHCKtextTexture *textTexture;
   if (!(textTexture = chckPoolAdd(object->textures, NULL, id)))
      goto fail;

   /* set internal texture dimensions for mapping */
   textTexture->internalWidth = (float)1.0f/object->cacheWidth;
   textTexture->internalHeight = (float)1.0f/object->cacheHeight;

   textTexture->texture = texture;
   textTexture->rows = rowsPool;

   RET(0, "%p", textTexture);
   return textTexture;

fail:
   IFDO(glhckTextureFree, texture);
   IFDO(chckIterPoolFree, rowsPool);
   RET(0, "%d", RETURN_FAIL);
   return NULL;
}

/* \brief free cache texture from text object
 * NOTE: all glyphs from fonts pointing to this cache texture are flushed after this! */
static void _glhckTextTextureFree(glhckText *object, chckPoolIndex textTextureId)
{
   __GLHCKtextTexture *t;
   CALL(1, "%p, %zu", object, textTextureId);
   assert(object);

   if (!(t = chckPoolGet(object->textures, textTextureId)))
      return;

   /* remove glyphs from fonts that contain this texture */
   __GLHCKtextFont *f;
   chckPoolIndex iterFonts;
   for (iterFonts = 0; (f = chckPoolIter(object->fonts, &iterFonts));) {
      __GLHCKtextGlyph *g;
      chckPoolIndex iterGlyphs;
      for (iterGlyphs = 0; (g = chckPoolIter(f->glyphs, &iterGlyphs));) {
         if (g->textureId != textTextureId)
            continue;

         IFDO(chckPoolFree, f->glyphs);
         chckLutFlush(f->lut);
         break;
      }
   }

   IFDO(glhckTextureFree, t->texture);
   IFDO(chckIterPoolFree, t->rows);
   IFDO(_glhckFree, t->geometry.vertexData);
   chckPoolRemove(object->textures, textTextureId);
}

/* \brief get texture where to cache the glyph, will allocate new cache page if glyph doesn't fit any existing pages. */
static chckPoolIndex _glhckTextGetCacheTexture(glhckText *object, int gw, int gh, __GLHCKtextTextureRow **row)
{
   chckPoolIndex iterCache = 0;
   __GLHCKtextTextureRow *br = NULL;
   int rh = (gh + 7) & ~7;

   if (row)
      *row = NULL;

   /* glyph should not be larger than texture */
   if (gw >= object->cacheWidth || gh >= object->cacheHeight)
      return GLHCK_INVALID_TEXTURE;

   while (!br) {
      /* find next texture with space */
      __GLHCKtextTexture *t;
      while ((t = chckPoolIter(object->textures, &iterCache)) && t->bitmap);

      if (!t) {
         if (!(t = _glhckTextTextureNew(object, &iterCache, GLHCK_ALPHA, GLHCK_UNSIGNED_BYTE, NULL))) {
            return GLHCK_INVALID_TEXTURE;
         } else {
            iterCache++;
         }
      }

      /* search for fitting row */
      for (chckPoolIndex iter = 0; (br = chckIterPoolIter(t->rows, &iter));) {
         if (br->h == rh && br->x + gw + 1 <= object->cacheWidth)
            break;
      }

      /* we got row, break */
      if (br)
         break;

      /* if no row found:
       * - add new row
       * - try next texture
       * - create new texture */

      int py = 0;

      /* this texture is at least used */
      __GLHCKtextTextureRow *lr;
      if ((lr = chckIterPoolGetLast(t->rows))) {
         py = lr->y + lr->h + 1;

         /* not enough space */
         if (py + rh > object->cacheHeight)
            continue;
      }

      /* add new row */
      br = chckIterPoolAdd(t->rows, NULL, NULL);
      br->x = 0;
      br->y = py;
      br->h = rh;
   }

   if (row)
      *row = br;

   return (br ? iterCache - 1 : GLHCK_INVALID_TEXTURE);
}

/* \brief get glyph from font */
__GLHCKtextGlyph* _glhckTextGetGlyph(glhckText *object, __GLHCKtextFont *font, unsigned int code, short size)
{
   __GLHCKtextGlyph *glyph;

   /* find code and size */
   if (font->glyphs) {
      chckPoolIndex iter = *(chckPoolIndex*)chckLutGet(font->lut, code);
      while (iter != GLHCK_INVALID_GLYPH && (glyph = chckPoolGet(font->glyphs, iter))) {
         if (glyph->code == code && (glyph->size == size || font->type == GLHCK_FONT_BMP))
            return glyph;
         iter = glyph->next;
      }
   }

   /* user hasn't added the glyph */
   if (font->type == GLHCK_FONT_BMP)
      return NULL;

   /* create glyph pool */
   if (!font->glyphs && !(font->glyphs = chckPoolNew(32, 32, sizeof(__GLHCKtextGlyph))))
      return NULL;

   /* create glyph if ttf font */
   int x1, y1, x2, y2, advance, lsb;
   float scale = stbtt_ScaleForPixelHeight(&font->font, size);
   int gid = stbtt_FindGlyphIndex(&font->font, code);
   stbtt_GetGlyphHMetrics(&font->font, gid, &advance, &lsb);
   stbtt_GetGlyphBitmapBox(&font->font, gid, scale, scale, &x1, &y1, &x2, &y2);

   int gw = x2 - x1;
   int gh = y2 - y1;

   /* get cache texture where to store the glyph */
   __GLHCKtextTextureRow *row;
   chckPoolIndex textTextureId;
   if ((textTextureId = _glhckTextGetCacheTexture(object, gw, gh, &row)) == GLHCK_INVALID_TEXTURE)
      return NULL;

   /* create new glyph */
   chckPoolIndex glyphId;
   if (!(glyph = chckPoolAdd(font->glyphs, NULL, &glyphId)))
      return NULL;

   /* init glyph */
   glyph->code = code;
   glyph->size = size;
   glyph->textureId = textTextureId;
   glyph->x1 = row->x;
   glyph->y1 = row->y;
   glyph->x2 = glyph->x1 + gw;
   glyph->y2 = glyph->y1 + gh;
   glyph->xadv = scale * advance;
   glyph->xoff = (float)x1;
   glyph->yoff = (float)y1;

   /* insert to hash lookup */
   glyph->next = *(chckPoolIndex*)chckLutGet(font->lut, code);
   chckLutSet(font->lut, code, &glyphId);

   /* this glyph is a space, don't render it */
   if (!gw || !gh)
      return glyph;

   /* advance in row */
   row->x += gw + 1;

   /* rasterize */
   unsigned char *data;
   __GLHCKtextTexture *t = chckPoolGet(object->textures, textTextureId);
   assert(!t->bitmap);
   if (t && (data = _glhckMalloc(gw * gh))) {
      stbtt_MakeGlyphBitmap(&font->font, data, gw, gh, gw, scale, scale, gid);
      glhckTextureFill(t->texture, 0, glyph->x1, glyph->y1, 0, gw, gh, 0, GLHCK_ALPHA, GLHCK_UNSIGNED_BYTE, gw * gh, data);
      _glhckFree(data);
   }

   return glyph;
}

/* \brief get quad for text drawing */
static int _getQuad(glhckText *object, __GLHCKtextFont *font, __GLHCKtextGlyph *glyph, short size, float *x, float *y, __GLHCKtextQuad *q)
{
   glhckVector2f v1, v2, t1, t2;
   assert(object && font && glyph);
   assert(x && y && q);
#if GLHCK_TEXT_FLOAT_PRECISION
   (void)object;
#endif

   float scale = (font->type == GLHCK_FONT_BMP ? (float)size/glyph->size : 1.0f);
   int rx = floorf(*x + scale * glyph->xoff);
   int ry = floorf(*y + scale * glyph->yoff);

   v2.x = rx;
   v1.y = ry;
   v1.x = rx + scale * (glyph->x2 - glyph->x1);
   v2.y = ry + scale * (glyph->y2 - glyph->y1);

   __GLHCKtextTexture *t;
   if ((t = chckPoolGet(object->textures, glyph->textureId))) {
      t2.x = glyph->x1 * t->internalWidth;
      t1.y = glyph->y1 * t->internalHeight;
      t1.x = glyph->x2 * t->internalWidth;
      t2.y = glyph->y2 * t->internalHeight;
   }

#if GLHCK_TEXT_FLOAT_PRECISION /* floats ftw */
   memcpy(&q->v1, &v1, sizeof(glhckVector2f));
   memcpy(&q->v2, &v2, sizeof(glhckVector2f));
#else /* short precision version */
   glhckSetV2(&q->v1, &v1);
   glhckSetV2(&q->v2, &v2);
#endif

#if GLHCK_TEXT_FLOAT_PRECISION /* floats ftw */
   memcpy(&q->t1, &t1, sizeof(glhckVector2f));
   memcpy(&q->t2, &t2, sizeof(glhckVector2f));
#else /* short precision version */
   q->t1.x = t1.x * object->textureRange;
   q->t1.y = t1.y * object->textureRange;
   q->t2.x = t2.x * object->textureRange;
   q->t2.y = t2.y * object->textureRange;
#endif

   *x += scale * glyph->xadv;
   return RETURN_OK;
}

/* \brief create new internal font */
static glhckFont glhckTextFontNewInternal(glhckText *object, const _glhckBitmapFontInfo *font, int *nativeSize)
{
   glhckTexture *texture;
   assert(object && font);

   if (nativeSize)
      *nativeSize = 0;

   /* create texture for bitmap font */
   if (!(texture = glhckTextureNew()))
      goto fail;

   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, font->width, font->height, 0, 0, font->format, font->type, 0, font->data) != RETURN_OK)
      goto fail;

   /* set default params */
   glhckTextureParameter(texture, glhckTextureDefaultSpriteParameters());

   /* create font from texture */
   glhckFont id;
   if ((id = glhckTextFontNewFromTexture(object, texture, font->ascent, font->descent, font->lineGap)) == GLHCK_INVALID_FONT)
      return GLHCK_INVALID_FONT;

   /* fill glyphs to the monospaced bitmap font */
   {
      unsigned int r, c;
      for (r = 0; r < font->rows; ++r) {
         for (c = 0; c < font->columns; ++c) {
            char utf8buf[5] = { 0, 0, 0, 0 };
            encutf8(r * font->columns + c, utf8buf, sizeof(utf8buf));
            glhckTextGlyphNew(object, id, utf8buf, font->fontSize, font->fontSize, c * font->fontSize, r * font->fontSize, font->fontSize / 2, font->fontSize, 0, 0, font->fontSize / 2);
         }
      }
   }

   if (nativeSize)
      *nativeSize = font->fontSize;

   return id;

fail:
   IFDO(glhckTextureFree, texture);
   return GLHCK_INVALID_FONT;
}

/***
 * public api
 ***/

/* \brief create new text stack */
GLHCKAPI glhckText* glhckTextNew(int cacheWidth, int cacheHeight)
{
   glhckText *object;
   CALL(0, "%d, %d", cacheWidth, cacheHeight);

   /* allocate text stack */
   if (!(object = _glhckCalloc(1, sizeof(glhckText))))
      goto fail;

   if (!(object->fonts = chckPoolNew(32, 0, sizeof(__GLHCKtextFont))))
      goto fail;

   if (!(object->textures = chckPoolNew(32, 0, sizeof(__GLHCKtextTexture))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* find next pow and limit */
   _glhckNextPow2(cacheWidth, cacheHeight, 0, &object->cacheWidth, &object->cacheHeight, NULL, GLHCKRF()->texture.maxTextureSize);

#if GLHCK_TEXT_FLOAT_PRECISION
   object->textureRange = 1;
#else
   object->textureRange = SHRT_MAX;
#endif

   /* default color */
   glhckTextColorb(object, 255, 255, 255, 255);

   /* insert to world */
   _glhckWorldAdd(&GLHCKW()->texts, object);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckTextFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference text stack */
GLHCKAPI glhckText* glhckTextRef(glhckText *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free text stack */
GLHCKAPI unsigned int glhckTextFree(glhckText *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free texture cache */
   __GLHCKtextTexture *t;
   for (chckPoolIndex iter = 0; (t = chckPoolIter(object->textures, &iter));) {
      IFDO(glhckTextureFree, t->texture);
      IFDO(chckIterPoolFree, t->rows);
      IFDO(_glhckFree, t->geometry.vertexData);
   }

   /* free font cache */
   __GLHCKtextFont *f;
   for (chckPoolIndex iter = 0; (f = chckPoolIter(object->fonts, &iter));) {
      IFDO(chckPoolFree, f->glyphs);
      IFDO(chckLutFree, f->lut);
      IFDO(_glhckFree, f->data);
   }

   IFDO(chckPoolFree, object->textures);
   IFDO(chckPoolFree, object->fonts);

   /* free shader */
   glhckTextShader(object, NULL);

   /* remove text */
   _glhckWorldRemove(&GLHCKW()->texts, object);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", (object ? object->refCounter : 0));
   return (object ? object->refCounter : 0);
}

/* \brief free font from text */
GLHCKAPI void glhckTextFontFree(glhckText *object, glhckFont fontId)
{
   __GLHCKtextFont *f;
   CALL(1, "%p, %u", object, fontId);
   assert(object);
   assert(fontId != GLHCK_INVALID_FONT);

   if (!(f = chckPoolGet(object->fonts, fontId)))
      return;

   /* free font */
   __GLHCKtextGlyph *g;
   for (chckPoolIndex iter = 0; (g = chckPoolIter(f->glyphs, &iter));)
      _glhckTextTextureFree(object, g->textureId);

   IFDO(chckPoolFree, f->glyphs);
   IFDO(chckLutFree, f->lut);
   chckPoolRemove(object->fonts, fontId);
}

/* \brief flush text glyph cache */
GLHCKAPI void glhckTextFlushCache(glhckText *object)
{
   CALL(1, "%p", object);
   assert(object);

   /* free texture cache */
   __GLHCKtextTexture *t;
   for (chckPoolIndex iter = 0; (t = chckPoolIter(object->textures, &iter));) {
      /* skip bitmap font textures */
      if (t->bitmap)
         continue;

      IFDO(chckIterPoolFree, t->rows);
      IFDO(glhckTextureFree, t->texture);
      IFDO(_glhckFree, t->geometry.vertexData);
      chckPoolRemove(object->textures, iter - 1);
   }

   /* free font cache */
   __GLHCKtextFont *f;
   for (chckPoolIndex iter = 0; (f = chckPoolIter(object->fonts, &iter));) {
      if (f->type == GLHCK_FONT_BMP)
         continue;

      IFDO(chckPoolFree, f->glyphs);
      chckLutFlush(f->lut);
   }
}

/* \brief get text metrics */
GLHCKAPI void glhckTextGetMetrics(glhckText *object, glhckFont fontId, float size, float *ascender, float *descender, float *lineHeight)
{
   CALL(1, "%p, %d, %f, %p, %p, %p", object, fontId, size, ascender, descender, lineHeight);
   assert(object);
   assert(fontId != GLHCK_INVALID_FONT);

   if (ascender) *ascender = 0.0f;
   if (descender) *descender = 0.0f;
   if (lineHeight) *lineHeight = 0.0f;

   __GLHCKtextFont *f;
   if (!(f = chckPoolGet(object->fonts, fontId)))
      return;

   if (ascender) *ascender = f->ascender * size;
   if (descender) *descender = f->descender * size;
   if (lineHeight) *lineHeight = f->lineHeight * size;
}

/* \brief get min max of text */
GLHCKAPI void glhckTextGetMinMax(glhckText *object, glhckFont fontId, float size, const char *s, kmVec2 *min, kmVec2 *max)
{
   CALL(1, "%p, %u, %f, %s, %p, %p", object, fontId, size, s, min, max);
   assert(object && s && min && max);
   assert(fontId != GLHCK_INVALID_FONT);

   if (min) memset(min, 0, sizeof(kmVec2));
   if (max) memset(max, 0, sizeof(kmVec2));

   __GLHCKtextFont *f;
   if (!(f = chckPoolGet(object->fonts, fontId)))
      return;

   float x, y;
   unsigned int codepoint = 0, state = 0;
   for (x = 0, y = 0; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s))
         continue;

      __GLHCKtextGlyph *g;
      if (!(g = _glhckTextGetGlyph(object, f, codepoint, size)))
         continue;

      __GLHCKtextQuad q;
      if (_getQuad(object, f, g, size, &x, &y, &q) != RETURN_OK)
         continue;

      if (min) glhckMinV2(min, &q.v1);
      if (max) glhckMaxV2(max, &q.v1);
      if (min) glhckMinV2(min, &q.v2);
      if (max) glhckMaxV2(max, &q.v2);
   }
}

/* \brief set text color */
GLHCKAPI void glhckTextColor(glhckText *object, const glhckColorb *color)
{
   CALL(2 ,"%p, "COLBS, object, COLB(color));
   assert(object && color);
   memcpy(&object->color, color, sizeof(glhckColorb));
}

/* \brief set text color (with unsigned char) */
GLHCKAPI void glhckTextColorb(glhckText *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   const glhckColorb color = { r, g, b, a };
   glhckTextColor(object, &color);
}

/* \brief get text color */
GLHCKAPI const glhckColorb* glhckTextGetColor(glhckText *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%p", &object->color);
   return &object->color;
}

/* \brief new internal font */
GLHCKAPI glhckFont glhckTextFontNewKakwafont(glhckText *object, int *nativeSize)
{
   CALL(0, "%p", object);
   assert(object);
   glhckFont id = glhckTextFontNewInternal(object, &kakwafont, nativeSize);
   RET(0, "%d", id);
   return id;
}

/* \brief new font from memory */
GLHCKAPI glhckFont glhckTextFontNewFromMemory(glhckText *object, const void *data, size_t size)
{
   __GLHCKtextFont *font;
   CALL(0, "%p, %p", object, data);
   assert(object && data);

   /* allocate font */
   chckPoolIndex id;
   if (!(font = chckPoolAdd(object->fonts, NULL, &id)))
      goto fail;

   if (!(font->lut = chckLutNew(GLHCK_FONT_LUT_SIZE, (int)GLHCK_INVALID_GLYPH, sizeof(chckPoolIndex))))
      goto fail;

   /* copy the data */
   if (!(font->data = _glhckCopy(data, size)))
      goto fail;

   /* create truetype font */
   if (!stbtt_InitFont(&font->font, font->data, 0))
      goto fail;

   /* store normalized line height */
   int ascent, descent, lineGap;
   stbtt_GetFontVMetrics(&font->font, &ascent, &descent, &lineGap);

   /* store normalized line height */
   int fh = fabs(ascent - descent);
   if (fh <= 0)
      fh = 1;

   font->ascender = (float)ascent/fh;
   font->descender = (float)descent/fh;
   font->lineHeight = (float)(fh + lineGap)/fh;
   font->type = GLHCK_FONT_TTF;

   RET(0, "%zu", id);
   return id;

fail:
   if (font) {
      IFDO(_glhckFree, font->data);
      chckPoolRemove(object->fonts, id);
   }
   RET(0, "%d", 0);
   return GLHCK_INVALID_FONT;
}

/* \brief new truetype font */
GLHCKAPI glhckFont glhckTextFontNew(glhckText *object, const char *file)
{
   FILE *f;
   unsigned char *data = NULL;
   CALL(0, "%p, %s", object, file);
   assert(object && file);

   /* open font */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 0, SEEK_END);
   size_t size = ftell(f);
   fseek(f, 0, SEEK_SET);

   if (!(data = _glhckMalloc(size)))
      goto fail;

   /* read data */
   if (fread(data, 1, size, f) != size)
      goto read_fail;
   NULLDO(fclose, f);

   /* read and add the new font to stash */
   glhckFont id;
   if ((id = glhckTextFontNewFromMemory(object, data, size)) == GLHCK_INVALID_FONT)
      goto fail;

   /* data not needed anymore */
   _glhckFree(data);

   RET(0, "%d", id);
   return id;

read_fail:
   DEBUG(GLHCK_DBG_WARNING, "\1Failed to open font: \5%s", file);
fail:
   IFDO(fclose, f);
   IFDO(_glhckFree, data);
   RET(0, "%d", 0);
   return GLHCK_INVALID_FONT;
}

/* \brief new bitmap font from texture */
GLHCKAPI glhckFont glhckTextFontNewFromTexture(glhckText *object, glhckTexture *texture, int ascent, int descent, int lineGap)
{
   __GLHCKtextFont *font;
   __GLHCKtextTexture *textTexture = NULL;
   CALL(0, "%p, %p, %d, %d, %d", object, texture, ascent, descent, lineGap);
   assert(object && texture);

   /* allocate font */
   chckPoolIndex fId;
   if (!(font = chckPoolAdd(object->fonts, NULL, &fId)))
      goto fail;

   if (!(font->lut = chckLutNew(GLHCK_FONT_LUT_SIZE, (int)GLHCK_INVALID_GLYPH, sizeof(chckPoolIndex))))
      goto fail;

   /* allocate text texture */
   chckPoolIndex tId;
   if (!(textTexture = chckPoolAdd(object->textures, NULL, &tId)))
      goto fail;

   /* reference texture */
   textTexture->texture = glhckTextureRef(texture);

   /* make sure no glyphs are cached to this texture */
   textTexture->bitmap = 1;

   /* store internal dimensions for mapping */
   textTexture->internalWidth = (float)1.0f/texture->width;
   textTexture->internalHeight = (float)1.0f/texture->height;

   /* store normalized line height */
   int fh = fabs(ascent - descent);
   if (fh == 0)
      fh = 1;

   font->ascender = (float)ascent/fh;
   font->descender = (float)descent/fh;
   font->lineHeight = (float)(fh + lineGap)/fh;
   font->textureId = tId;
   font->type = GLHCK_FONT_BMP;

   RET(0, "%zu", fId);
   return fId;

fail:
   if (textTexture) {
      IFDO(glhckTextureFree, textTexture->texture);
      chckPoolRemove(object->textures, tId);
   }
   if (font) {
      chckPoolRemove(object->fonts, fId);
   }
   RET(0, "%d", 0);
   return GLHCK_INVALID_FONT;
}

/* \brief new bitmap font */
GLHCKAPI glhckFont glhckTextFontNewFromBitmap(glhckText *object, const char *file, int ascent, int descent, int lineGap)
{
   glhckTexture *texture;
   CALL(0, "%p, %s, %d, %d, %d", object, file, ascent, descent, lineGap);
   assert(object && file);

   /* load image */
   if (!(texture = glhckTextureNewFromFile(file, NULL, NULL)))
      goto fail;

   /* set default parameters */
   glhckTextureParameter(texture, glhckTextureDefaultSpriteParameters());

   glhckFont id;
   if ((id = glhckTextFontNewFromTexture(object, texture, ascent, descent, lineGap)) == GLHCK_INVALID_FONT)
      goto fail;

   glhckTextureFree(texture);
   RET(0, "%d", id);
   return id;

fail:
   IFDO(glhckTextureFree, texture);
   RET(0, "%d", 0);
   return GLHCK_INVALID_FONT;
}

/* \brief add new glyph to bitmap font */
GLHCKAPI void glhckTextGlyphNew(glhckText *object,
      glhckFont fontId, const char *s,
      short size, short base, int x, int y, int w, int h,
      float xoff, float yoff, float xadvance)
{
   CALL(0, "%p, \"%s\", %d, %d, %d, %d, %d, %d, %f, %f, %f",
         object, s, size, base, w, y, w, h, xoff, yoff, xadvance);
   assert(object && s);
   assert(fontId != GLHCK_INVALID_FONT);

   __GLHCKtextFont *f;
   if (!(f = chckPoolGet(object->fonts, fontId)))
      return;

   assert(f->type == GLHCK_FONT_BMP);

   /* decode utf8 character */
   unsigned int codepoint = 0, state = 0;
   for (; *s; ++s) {
      if (!decutf8(&state, &codepoint, *(unsigned char*)s))
         break;
   }

   if (state != UTF8_ACCEPT)
      return;

   if (!f->glyphs && !(f->glyphs = chckPoolNew(32, 32, sizeof(__GLHCKtextGlyph))))
      return;

   /* allocate space for new glyph */
   chckPoolIndex id;
   __GLHCKtextGlyph *g;
   if (!(g = chckPoolAdd(f->glyphs, NULL, &id)))
      return;

   /* init glyph */
   g->code = codepoint;
   g->textureId = f->textureId;
   g->size = size;
   g->x1 = x;
   g->y1 = y;
   g->x2 = x + w;
   g->y2 = y + h;
   g->xoff = xoff;
   g->yoff = yoff - base;
   g->xadv = xadvance;

   /* insert to hash lookup */
   g->next = *(chckPoolIndex*)chckLutGet(f->lut, codepoint);
   chckLutSet(f->lut, codepoint, &id);
}

/* \brief render all drawn text */
GLHCKAPI void glhckTextRender(glhckText *object)
{
   CALL(2, "%p", object);
   assert(object);
   GLHCKRA()->textRender(object);
}

/* \brief clear text from stash */
GLHCKAPI void glhckTextClear(glhckText *object)
{
   CALL(2, "%p", object);
   assert(object);

   __GLHCKtextTexture *t;
   for (chckPoolIndex iter = 0; (t = chckPoolIter(object->textures, &iter));)
      t->geometry.vertexCount = 0;
}

/* \brief draw text using font */
GLHCKAPI void glhckTextStash(glhckText *object, glhckFont fontId, float size, float x, float y, const char *s, float *width)
{
   CALL(2, "%p, %u, %f, %f, %f, %s, %p", object, fontId, size, x, y, s, width);
   assert(object && s);
   assert(fontId != GLHCK_INVALID_FONT);

   if (width)
      *width = 0;

   __GLHCKtextFont *f;
   if (!(f = chckPoolGet(object->fonts, fontId)))
      return;

   unsigned int codepoint = 0, state = 0;
   for (; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s))
         continue;

      __GLHCKtextGlyph *g;
      if (!(g = _glhckTextGetGlyph(object, f, codepoint, size)))
         continue;

      __GLHCKtextTexture *t;
      if (!(t = chckPoolGet(object->textures, g->textureId)))
         continue;

      int vcount = t->geometry.vertexCount;
#if GLHCK_TRISTRIP
      int newVcount = (vcount ? vcount + 6 : 4);
#else
      int newVcount = vcount + 6;
#endif
      if (newVcount >= t->geometry.allocatedCount) {
         if (_glhckTextGeometryAllocateMore(&t->geometry) != RETURN_OK) {
            DEBUG(GLHCK_DBG_WARNING, "TEXT :: [%p] out of memory!", object);
            continue;
         }
      }

      /* should not ever fail */
      __GLHCKtextQuad q;
      if (_getQuad(object, f, g, size, &x, &y, &q) != RETURN_OK)
         continue;

      /* insert geometry data */
#if GLHCK_TEXT_FLOAT_PRECISION
      glhckVertexData2f *v = &t->geometry.vertexData[vcount];
#else
      glhckVertexData2s *v = &t->geometry.vertexData[vcount];
#endif

      /* NOTE: Text geometry has inverse winding to glhck's planes.
       * This is so we don't need to toggle winding order in GL when drawing.
       *
       * The end effect is basically same as using glhckRenderFlip,
       * only these vertices are pre-multiplied. */

      unsigned int i = 0;
#if GLHCK_TRISTRIP
      /* degenerate */
      if (vcount) {
#if GLHCK_TEXT_FLOAT_PRECISION
         glhckVertexData2f *d = &t->geometry.vertexData[vcount-1];
#else
         glhckVertexData2s *d = &t->geometry.vertexData[vcount-1];
#endif
         v[i].vertex.x = d->vertex.x; v[i+0].vertex.y = d->vertex.y;
         v[i].coord.x  = d->coord.x;  v[i++].coord.y  = d->coord.y;
         v[i].vertex.x = q.v2.x; v[i+0].vertex.y = q.v2.y;
         v[i].coord.x  = q.t2.x; v[i++].coord.y  = q.t2.y;
      }

      /* tristrip */
      v[i].vertex.x = q.v2.x; v[i+0].vertex.y = q.v2.y;
      v[i].coord.x  = q.t2.x; v[i++].coord.y  = q.t2.y;
      v[i].vertex.x = q.v1.x; v[i+0].vertex.y = q.v2.y;
      v[i].coord.x  = q.t1.x; v[i++].coord.y  = q.t2.y;
      v[i].vertex.x = q.v2.x; v[i+0].vertex.y = q.v1.y;
      v[i].coord.x  = q.t2.x; v[i++].coord.y  = q.t1.y;
      v[i].vertex.x = q.v1.x; v[i+0].vertex.y = q.v1.y;
      v[i].coord.x  = q.t1.x; v[i++].coord.y  = q.t1.y;
#else
      /* triangle */
      v[i].vertex.x = q.v1.x; v[i+0].vertex.y = q.v1.y;
      v[i].coord.x  = q.t1.x; v[i++].coord.y  = q.t1.y;
      v[i].vertex.x = q.v2.x; v[i+0].vertex.y = q.v1.y;
      v[i].coord.x  = q.t2.x; v[i++].coord.y  = q.t1.y;
      v[i].vertex.x = q.v2.x; v[i+0].vertex.y = q.v2.y;
      v[i].coord.x  = q.t2.x; v[i++].coord.y  = q.t2.y;

      v[i].vertex.x = q.v1.x; v[i+0].vertex.y = q.v1.y;
      v[i].coord.x  = q.t1.x; v[i++].coord.y  = q.t1.y;
      v[i].vertex.x = q.v2.x; v[i+0].vertex.y = q.v2.y;
      v[i].coord.x  = q.t2.x; v[i++].coord.y  = q.t2.y;
      v[i].vertex.x = q.v1.x; v[i+0].vertex.y = q.v2.y;
      v[i].coord.x  = q.t1.x; v[i++].coord.y  = q.t2.y;
#endif

      t->geometry.vertexCount += i;
   }

   if (width)
      *width = x;
}

/* \brief set shader to text */
GLHCKAPI void glhckTextShader(glhckText *object, glhckShader *shader)
{
   CALL(1, "%p, %p", object, shader);
   assert(object);

   if (object->shader == shader)
      return;

   if (object->shader)
      glhckShaderFree(object->shader);

   object->shader = (shader ? glhckShaderRef(shader) : NULL);
}

/* \brief get text's shader */
GLHCKAPI glhckShader* glhckTextGetShader(const glhckText *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%p", object->shader);
   return object->shader;
}

/* \brief create texture from text */
GLHCKAPI glhckTexture* glhckTextRTT(glhckText *object, glhckFont fontId, float size, const char *s, const glhckTextureParameters *params)
{
   glhckTexture *texture = NULL;
   glhckFramebuffer *fbo = NULL;
   CALL(1, "%p, %u, %f, %s", object, fontId, size, s);
   assert(object);
   assert(fontId != GLHCK_INVALID_FONT);

   if (!(texture = glhckTextureNew()))
      goto fail;

   float linew, ascender, descender;
   glhckTextClear(object);
   glhckTextGetMetrics(object, fontId, 1.0f, &ascender, &descender, NULL);
   glhckTextStash(object, fontId, size, 0, size + descender * size, s, &linew);

   if (linew <= 0.0f)
      goto fail;

   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, linew, size, 0, 0, GLHCK_RGBA, GLHCK_UNSIGNED_BYTE, 0, NULL) != RETURN_OK)
      goto fail;

   glhckTextureParameters nparams;
   memcpy(&nparams, (params ? params : glhckTextureDefaultLinearParameters()), sizeof(glhckTextureParameters));
   glhckTextureParameter(texture, &nparams);

   if (!(fbo = glhckFramebufferNew(GLHCK_FRAMEBUFFER)))
      goto fail;

   if (glhckFramebufferAttachTexture(fbo, texture, GLHCK_COLOR_ATTACHMENT0) != RETURN_OK)
      goto fail;

   glhckFramebufferRecti(fbo, 0, 0, linew, size);
   glhckFramebufferBegin(fbo);
   glhckRenderPass(GLHCK_PASS_TEXTURE);
   glhckRenderClearColorb(0,0,0,0);
   glhckRenderClear(GLHCK_COLOR_BUFFER_BIT);
   glhckTextRender(object);
   glhckFramebufferEnd(fbo);

   texture->importFlags |= GLHCK_TEXTURE_IMPORT_TEXT;
   glhckFramebufferFree(fbo);
   glhckTextClear(object);

   RET(1, "%p", texture);
   return texture;

fail:
   glhckTextClear(object);
   IFDO(glhckFramebufferFree, fbo);
   IFDO(glhckTextureFree, texture);
   RET(1, "%p", NULL);
   return NULL;
}

/* \brief create plane object from text */
GLHCKAPI glhckObject* glhckTextPlane(glhckText *object, glhckFont fontId, float size, const char *s, const glhckTextureParameters *params)
{
   glhckTexture *texture;
   glhckObject *sprite = NULL;
   assert(object);
   assert(fontId != GLHCK_INVALID_FONT);

   if (!(texture = glhckTextRTT(object, fontId, size, s, params)))
      goto fail;

   if (!(sprite = glhckSpriteNew(texture, 0, 0)))
      goto fail;

   glhckTextureFree(texture);
   return sprite;

fail:
   IFDO(glhckTextureFree, texture);
   IFDO(glhckObjectFree, sprite);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
