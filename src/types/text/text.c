#include "text.h"

#include <glhck/glhck.h>
#include <stdio.h>

#include "trace.h"
#include "handle.h"
#include "system/tls.h"
#include "pool/pool.h"
#include "lut/lut.h"

/* builtin fonts */
#include "fonts.h"
#include "fonts/kakwafont.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXT

/* stb_truetype */
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define GLHCK_FONT_LUT_SIZE 256
#define GLHCK_TEXT_VERT_COUNT (6 * 128)

#define GLHCK_INVALID_GLYPH (chckPoolIndex)-1
#define GLHCK_INVALID_TEXTURE (chckPoolIndex)-1

/* assign the max units from vectors to v1 */
#define glhckMaxV2(v1, v2) \
   if ((v1)->x < (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y < (v2)->y) (v1)->y = (v2)->y
#define glhckMaxV3(v1, v2) \
   glhckMaxV2(v1, v2);     \
   if ((v1)->z < (v2)->z) (v1)->z = (v2)->z

/* assign the min units from vectors to v1 */
#define glhckMinV2(v1, v2) \
   if ((v1)->x > (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y > (v2)->y) (v1)->y = (v2)->y
#define glhckMinV3(v1, v2) \
   glhckMinV2(v1, v2);     \
   if ((v1)->z > (v2)->z) (v1)->z = (v2)->z

enum fontType {
   FONT_TTF,
   FONT_BMP,
};

struct quad {
#if GLHCK_TEXT_FLOAT_PRECISION /* floats ftw */
   glhckVector2f v1, v2;
   glhckVector2f t1, t2;
#else /* short version */
   glhckVector2s v1, v2;
   glhckVector2s t1, t2;
#endif
};

struct glyph
{
   chckPoolIndex textureId;
   chckPoolIndex next;
   float xadv, xoff, yoff;
   unsigned int code;
   int x1, y1, x2, y2;
   short size;
};

struct font
{
   struct stbtt_fontinfo font;
   chckPoolIndex textureId; /* only used on bitmap fonts */
   chckLut *lut;
   chckPool *glyphs;
   void *data;
   float ascender, descender, lineHeight;
   enum fontType type;
};

struct row {
   unsigned short x, y, h;
};

struct size {
   int width, height;
};

enum pool {
   $shader, // glhckHandle
   $fonts, // chckPool (font)
   $textures, // chckPool (glhckTextTexture)
   $cacheSize, // struct size
   $color, // glhckColor
   POOL_LAST
};

static size_t pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // shader
   sizeof(chckPool*), // fonts
   sizeof(chckPool*), // textures
   sizeof(struct size), // cache size
   sizeof(glhckColor), // color
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

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

static int textGeometryAllocateMore(struct glhckTextGeometry *geometry)
{
   int newCount = geometry->vertexCount + GLHCK_TEXT_VERT_COUNT;

#if GLHCK_TEXT_FLOAT_PRECISION
   glhckVertexData2f *newData;
   if (!(newData = realloc(geometry->vertexData, newCount * sizeof(glhckVertexData2f))))
      goto fail;
#else
   glhckVertexData2s *newData;
   if (!(newData = realloc(geometry->vertexData, newCount * sizeof(glhckVertexData2s))))
      goto fail;
#endif

   geometry->vertexData = newData;
   geometry->allocatedCount = newCount;
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief creates new texture to the cache */
static struct glhckTextTexture* textTextureNew(const glhckHandle handle, chckPoolIndex *outId, glhckTextureFormat format, glhckDataType type, const void *data)
{
   glhckHandle texture = 0;
   chckIterPool *rowsPool = NULL;
   CALL(0, "%s, %u, %u, %p", glhckHandleRepr(handle), format, type, data);

   if (!(texture = glhckTextureNew()))
      goto fail;

   const struct size *cache = get($cacheSize, handle);

   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, cache->width, cache->height, 0, 0, format, type, 0, data) != RETURN_OK)
      goto fail;

   /* set default params */
   glhckTextureParameter(texture, glhckTextureDefaultLinearParameters());

   if (!(rowsPool = chckIterPoolNew(32, 32, sizeof(struct glhckTextTexture))))
      goto fail;

   struct glhckTextTexture *textTexture;
   if (!(textTexture = chckPoolAdd(*(chckPool**)get($textures, handle), NULL, outId)))
      goto fail;

   /* set internal texture dimensions for mapping */
   textTexture->internalWidth = (float)1.0f / cache->width;
   textTexture->internalHeight = (float)1.0f / cache->height;

   textTexture->texture = texture;
   textTexture->rows = rowsPool;

   RET(0, "%p", textTexture);
   return textTexture;

fail:
   IFDO(glhckHandleRelease, texture);
   IFDO(chckIterPoolFree, rowsPool);
   RET(0, "%d", RETURN_FAIL);
   return NULL;
}

/* \brief free cache texture from text object
 * NOTE: all glyphs from fonts pointing to this cache texture are flushed after this! */
static void textTextureFree(const glhckHandle handle, chckPoolIndex textTextureId)
{
   CALL(1, "%s, %zu", glhckHandleRepr(handle), textTextureId);

   struct glhckTextTexture *t;
   if (!(t = chckPoolGet(*(chckPool**)get($textures, handle), textTextureId)))
      return;

   /* remove glyphs from fonts that contain this texture */
   struct font *f;
   for (chckPoolIndex iterFonts = 0; (f = chckPoolIter(*(chckPool**)get($fonts, handle), &iterFonts));) {
      struct glyph *g;
      for (chckPoolIndex iterGlyphs = 0; (g = chckPoolIter(f->glyphs, &iterGlyphs));) {
         if (g->textureId != textTextureId)
            continue;

         IFDO(chckPoolFree, f->glyphs);
         chckLutFlush(f->lut);
         break;
      }
   }

   IFDO(glhckHandleRelease, t->texture);
   IFDO(chckIterPoolFree, t->rows);
   IFDO(free, t->geometry.vertexData);
   chckPoolRemove(*(chckPool**)get($textures, handle), textTextureId);
}

/* \brief get texture where to cache the glyph, will allocate new cache page if glyph doesn't fit any existing pages. */
static chckPoolIndex getCacheTexture(const glhckHandle handle, int gw, int gh, struct row **outRow)
{
   chckPoolIndex iterCache = 0;
   struct row *row = NULL;
   int rh = (gh + 7) & ~7;

   if (outRow)
      *outRow = NULL;

   const struct size *cache = get($cacheSize, handle);

   /* glyph should not be larger than texture */
   if (gw >= cache->width || gh >= cache->height)
      return GLHCK_INVALID_TEXTURE;

   while (!row) {
      /* find next texture with space */
      struct glhckTextTexture *t;
      while ((t = chckPoolIter(*(chckPool**)get($textures, handle), &iterCache)) && t->bitmap);

      if (!t) {
         if (!(t = textTextureNew(handle, &iterCache, GLHCK_ALPHA, GLHCK_UNSIGNED_BYTE, NULL))) {
            return GLHCK_INVALID_TEXTURE;
         } else {
            iterCache++;
         }
      }

      /* search for fitting row */
      for (chckPoolIndex iter = 0; (row = chckIterPoolIter(t->rows, &iter));) {
         if (row->h == rh && row->x + gw + 1 <= cache->width)
            break;
      }

      /* we got row, break */
      if (row)
         break;

      /* if no row found:
       * - add new row
       * - try next texture
       * - create new texture */

      int py = 0;

      /* this texture is at least used */
      struct row *lr;
      if ((lr = chckIterPoolGetLast(t->rows))) {
         py = lr->y + lr->h + 1;

         /* not enough space */
         if (py + rh > cache->height)
            continue;
      }

      /* add new row */
      row = chckIterPoolAdd(t->rows, NULL, NULL);
      row->x = 0;
      row->y = py;
      row->h = rh;
   }

   if (outRow)
      *outRow = row;

   return (row ? iterCache - 1 : GLHCK_INVALID_TEXTURE);
}

/* \brief get glyph from font */
static struct glyph* getGlyph(const glhckHandle handle, struct font *font, unsigned int code, short size)
{
   struct glyph *glyph;

   /* find code and size */
   if (font->glyphs) {
      chckPoolIndex iter = *(chckPoolIndex*)chckLutGet(font->lut, code);
      while (iter != GLHCK_INVALID_GLYPH && (glyph = chckPoolGet(font->glyphs, iter))) {
         if (glyph->code == code && (glyph->size == size || font->type == FONT_BMP))
            return glyph;
         iter = glyph->next;
      }
   }

   /* user hasn't added the glyph */
   if (font->type == FONT_BMP)
      return NULL;

   /* create glyph pool */
   if (!font->glyphs && !(font->glyphs = chckPoolNew(32, 32, sizeof(struct glyph))))
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
   struct row *row;
   chckPoolIndex textTextureId;
   if ((textTextureId = getCacheTexture(handle, gw, gh, &row)) == GLHCK_INVALID_TEXTURE)
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
   struct glhckTextTexture *t = chckPoolGet(*(chckPool**)get($textures, handle), textTextureId);
   assert(!t->bitmap);
   if (t && (data = malloc(gw * gh))) {
      stbtt_MakeGlyphBitmap(&font->font, data, gw, gh, gw, scale, scale, gid);
      glhckTextureFill(t->texture, 0, glyph->x1, glyph->y1, 0, gw, gh, 0, GLHCK_ALPHA, GLHCK_UNSIGNED_BYTE, gw * gh, data);
      free(data);
   }

   return glyph;
}

/* \brief get quad for text drawing */
static int getQuad(const glhckHandle handle, const struct font *font, const struct glyph *glyph, short size, float *inOutX, float *inOutY, struct quad *outQuad)
{
   assert(font && glyph);
   assert(inOutX && inOutY && outQuad);

   float scale = (font->type == FONT_BMP ? (float)size/glyph->size : 1.0f);
   int rx = floorf(*inOutX + scale * glyph->xoff);
   int ry = floorf(*inOutY + scale * glyph->yoff);

   glhckVector2f v1, v2, t1, t2;

   v2.x = rx;
   v1.y = ry;
   v1.x = rx + scale * (glyph->x2 - glyph->x1);
   v2.y = ry + scale * (glyph->y2 - glyph->y1);

   struct glhckTextTexture *t;
   if ((t = chckPoolGet(*(chckPool**)get($textures, handle), glyph->textureId))) {
      t2.x = glyph->x1 * t->internalWidth;
      t1.y = glyph->y1 * t->internalHeight;
      t1.x = glyph->x2 * t->internalWidth;
      t2.y = glyph->y2 * t->internalHeight;
   }

#if GLHCK_TEXT_FLOAT_PRECISION /* floats ftw */
   memcpy(&outQuad->v1, &v1, sizeof(glhckVector2f));
   memcpy(&outQuad->v2, &v2, sizeof(glhckVector2f));
#else /* short precision version */
   outQuad->v1.x = v1.x;
   outQuad->v1.y = v1.y;
   outQuad->v2.x = v2.x;
   outQuad->v2.y = v2.y;
#endif

#if GLHCK_TEXT_FLOAT_PRECISION /* floats ftw */
   memcpy(&outQuad->t1, &t1, sizeof(glhckVector2f));
   memcpy(&outQuad->t2, &t2, sizeof(glhckVector2f));
#else /* short precision version */
   outQuad->t1.x = t1.x * GLHCK_TEXT_TEXTURE_RANGE;
   outQuad->t1.y = t1.y * GLHCK_TEXT_TEXTURE_RANGE;
   outQuad->t2.x = t2.x * GLHCK_TEXT_TEXTURE_RANGE;
   outQuad->t2.y = t2.y * GLHCK_TEXT_TEXTURE_RANGE;
#endif

   *inOutX += scale * glyph->xadv;
   return RETURN_OK;
}

/* \brief create new internal font */
static glhckFont fontNewInternal(const glhckHandle handle, const struct glhckBitmapFontInfo *font, int *outNativeSize)
{
   glhckHandle texture = 0;
   assert(font);

   if (outNativeSize)
      *outNativeSize = 0;

   if (!(texture = glhckTextureNew()))
      goto fail;

   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, font->width, font->height, 0, 0, font->format, font->type, 0, font->data) != RETURN_OK)
      goto fail;

   /* set default params */
   glhckTextureParameter(texture, glhckTextureDefaultSpriteParameters());

   /* create font from texture */
   glhckFont id;
   if (!(id = glhckTextFontNewFromTexture(handle, texture, font->ascent, font->descent, font->lineGap)))
      goto fail;

   /* fill glyphs to the monospaced bitmap font */
   {
      unsigned int r, c;
      for (r = 0; r < font->rows; ++r) {
         for (c = 0; c < font->columns; ++c) {
            char utf8buf[5] = { 0, 0, 0, 0 };
            encutf8(r * font->columns + c, utf8buf, sizeof(utf8buf));
            glhckTextGlyphNew(handle, id, utf8buf, font->fontSize, font->fontSize, c * font->fontSize, r * font->fontSize, font->fontSize / 2, font->fontSize, 0, 0, font->fontSize / 2);
         }
      }
   }

   if (outNativeSize)
      *outNativeSize = font->fontSize;

   return id;

fail:
   IFDO(glhckHandleRelease, texture);
   return 0;
}

struct glhckTextTexture* _glhckTextGetTextTexture(const glhckHandle handle, const chckPoolIndex index)
{
   return chckPoolGet(*(chckPool**)get($textures, handle), index);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   chckPool *textures = *(chckPool**)get($textures, handle);
   chckPool *fonts = *(chckPool**)get($fonts, handle);

   struct glhckTextTexture *t;
   for (chckPoolIndex iter = 0; (t = chckPoolIter(textures, &iter));) {
      IFDO(glhckHandleRelease, t->texture);
      IFDO(chckIterPoolFree, t->rows);
      IFDO(free, t->geometry.vertexData);
   }

   struct font *f;
   for (chckPoolIndex iter = 0; (f = chckPoolIter(fonts, &iter));) {
      IFDO(chckPoolFree, f->glyphs);
      IFDO(chckLutFree, f->lut);
      IFDO(free, f->data);
   }

   IFDO(chckPoolFree, textures);
   IFDO(chckPoolFree, fonts);

   glhckTextShader(handle, 0);
}

/***
 * public api
 ***/

/* \brief create new text stack */
GLHCKAPI glhckHandle glhckTextNew(int cacheWidth, int cacheHeight)
{
   CALL(0, "%d, %d", cacheWidth, cacheHeight);

   glhckHandle handle = 0;
   chckPool *fonts = NULL, *textures = NULL;

   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_TEXT, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   if (!(fonts = chckPoolNew(32, 0, sizeof(struct font))))
      goto fail;

   if (!(textures = chckPoolNew(32, 0, sizeof(struct glhckTextTexture))))
      goto fail;

   set($fonts, handle, &fonts);
   set($textures, handle, &textures);

   // _glhckNextPow2(cacheWidth, cacheHeight, 0, &object->cacheWidth, &object->cacheHeight, NULL, GLHCKRF()->texture.maxTextureSize);

   glhckTextColoru(handle, 255, 255, 255, 255);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   IFDO(chckPoolFree, fonts);
   IFDO(chckPoolFree, textures);
   RET(0, "%s", glhckHandleRepr(handle));
   return 0;
}


/* \brief free font from text */
GLHCKAPI void glhckTextFontFree(const glhckHandle handle, const glhckFont font)
{
   CALL(1, "%s, %u", glhckHandleRepr(handle), font);
   assert(handle > 0 && font > 0);

   struct font *f;
   chckPool *fonts = *(chckPool**)get($fonts, handle);
   if (!(f = chckPoolGet(fonts, font - 1)))
      return;

   struct glyph *g;
   for (chckPoolIndex iter = 0; (g = chckPoolIter(f->glyphs, &iter));)
      textTextureFree(handle, g->textureId);

   IFDO(chckPoolFree, f->glyphs);
   IFDO(chckLutFree, f->lut);
   chckPoolRemove(fonts, font);
}

/* \brief flush text glyph cache */
GLHCKAPI void glhckTextFlushCache(const glhckHandle handle)
{
   CALL(1, "%s", glhckHandleRepr(handle));

   /* free texture cache */
   struct glhckTextTexture *t;
   chckPool *textures = *(chckPool**)get($textures, handle);
   for (chckPoolIndex iter = 0; (t = chckPoolIter(textures, &iter));) {
      /* skip bitmap font textures */
      if (t->bitmap)
         continue;

      IFDO(chckIterPoolFree, t->rows);
      IFDO(glhckHandleRelease, t->texture);
      IFDO(free, t->geometry.vertexData);
      chckPoolRemove(textures, iter - 1);
   }

   /* free font cache */
   struct font *f;
   for (chckPoolIndex iter = 0; (f = chckPoolIter(*(chckPool**)get($fonts, handle), &iter));) {
      if (f->type == FONT_BMP)
         continue;

      IFDO(chckPoolFree, f->glyphs);
      chckLutFlush(f->lut);
   }
}

/* \brief get text metrics */
GLHCKAPI void glhckTextGetMetrics(const glhckHandle handle, const glhckFont font, float size, float *outAscender, float *outDescender, float *outLineHeight)
{
   CALL(1, "%s, %d, %f, %p, %p, %p", glhckHandleRepr(handle), font, size, outAscender, outDescender, outLineHeight);
   assert(handle > 0 && font > 0);

   if (outAscender) *outAscender = 0.0f;
   if (outDescender) *outDescender = 0.0f;
   if (outLineHeight) *outLineHeight = 0.0f;

   struct font *f;
   if (!(f = chckPoolGet(*(chckPool**)get($fonts, handle), font - 1)))
      return;

   if (outAscender) *outAscender = f->ascender * size;
   if (outDescender) *outDescender = f->descender * size;
   if (outLineHeight) *outLineHeight = f->lineHeight * size;
}

/* \brief get min max of text */
GLHCKAPI void glhckTextGetMinMax(const glhckHandle handle, const glhckFont font, float size, const char *s, kmVec2 *outMin, kmVec2 *outMax)
{
   CALL(1, "%s, %u, %f, %s, %p, %p", glhckHandleRepr(handle), font, size, s, outMin, outMax);
   assert(handle > 0 && font > 0 && s && outMin && outMax);

   if (outMin) memset(outMin, 0, sizeof(kmVec2));
   if (outMax) memset(outMax, 0, sizeof(kmVec2));

   struct font *f;
   if (!(f = chckPoolGet(*(chckPool**)get($fonts, handle), font - 1)))
      return;

   float x, y;
   unsigned int codepoint = 0, state = 0;
   for (x = 0, y = 0; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s))
         continue;

      struct glyph *g;
      if (!(g = getGlyph(handle, f, codepoint, size)))
         continue;

      struct quad q;
      if (getQuad(handle, f, g, size, &x, &y, &q) != RETURN_OK)
         continue;

      if (outMin) glhckMinV2(outMin, &q.v1);
      if (outMax) glhckMaxV2(outMax, &q.v1);
      if (outMin) glhckMinV2(outMin, &q.v2);
      if (outMax) glhckMaxV2(outMax, &q.v2);
   }
}

/* \brief set text color */
GLHCKAPI void glhckTextColor(const glhckHandle handle, const glhckColor color)
{
   set($color, handle, &color);
}

/* \brief set text color (with unsigned char) */
GLHCKAPI void glhckTextColoru(const glhckHandle handle, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a)
{
   glhckTextColor(handle, (a | (b << 8) | (g << 16) | (r << 24)));
}

/* \brief get text color */
GLHCKAPI glhckColor glhckTextGetColor(const glhckHandle handle)
{
   return *(glhckColor*)get($color, handle);
}

/* \brief new internal font */
GLHCKAPI glhckFont glhckTextFontNewKakwafont(const glhckHandle handle, int *outNativeSize)
{
   CALL(0, "%s, %p", glhckHandleRepr(handle), outNativeSize);
   glhckFont font = fontNewInternal(handle, &kakwafont, outNativeSize);
   RET(0, "%u", font);
   return font;
}

/* \brief new font from memory */
GLHCKAPI glhckFont glhckTextFontNewFromMemory(const glhckHandle handle, const void *data, const size_t size)
{
   struct font *font = NULL;
   CALL(0, "%s, %p, %zu", glhckHandleRepr(handle), data, size);
   assert(data);

   chckPoolIndex id;
   if (!(font = chckPoolAdd(*(chckPool**)get($fonts, handle), NULL, &id)))
      goto fail;

   if (!(font->lut = chckLutNew(GLHCK_FONT_LUT_SIZE, (int)GLHCK_INVALID_GLYPH, sizeof(chckPoolIndex))))
      goto fail;

   if (!(font->data = malloc(size)))
      goto fail;

   memcpy(font->data, data, size);

   if (!stbtt_InitFont(&font->font, font->data, 0))
      goto fail;

   int ascent, descent, lineGap;
   stbtt_GetFontVMetrics(&font->font, &ascent, &descent, &lineGap);

   int fh = fabs(ascent - descent);
   if (fh <= 0)
      fh = 1;

   font->ascender = (float)ascent/fh;
   font->descender = (float)descent/fh;
   font->lineHeight = (float)(fh + lineGap)/fh;
   font->type = FONT_TTF;

   RET(0, "%zu", id);
   return id - 1;

fail:
   if (font) {
      IFDO(free, font->data);
      chckPoolRemove(*(chckPool**)get($fonts, handle), id);
   }
   RET(0, "%u", 0);
   return 0;
}

/* \brief new truetype font */
GLHCKAPI glhckFont glhckTextFontNew(const glhckHandle handle, const char *file)
{
   FILE *f = NULL;
   unsigned char *data = NULL;
   CALL(0, "%s, %s", glhckHandleRepr(handle), file);
   assert(file);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 0, SEEK_END);
   size_t size = ftell(f);
   fseek(f, 0, SEEK_SET);

   if (!(data = malloc(size)))
      goto fail;

   if (fread(data, 1, size, f) != size)
      goto read_fail;

   NULLDO(fclose, f);

   glhckFont font;
   if (!(font = glhckTextFontNewFromMemory(handle, data, size)))
      goto fail;

   free(data);
   RET(0, "%d", font);
   return font;

read_fail:
   DEBUG(GLHCK_DBG_WARNING, "\1Failed to open font: \5%s", file);
fail:
   IFDO(fclose, f);
   IFDO(free, data);
   RET(0, "%d", 0);
   return 0;
}

/* \brief new bitmap font from texture */
GLHCKAPI glhckFont glhckTextFontNewFromTexture(const glhckHandle handle, const glhckHandle texture, int ascent, int descent, int lineGap)
{
   struct font *font;
   struct glhckTextTexture *textTexture = NULL;
   CALL(0, "%s, %s, %d, %d, %d", glhckHandleRepr(handle), glhckHandleRepr(texture), ascent, descent, lineGap);

   chckPoolIndex fId;
   if (!(font = chckPoolAdd(*(chckPool**)get($fonts, handle), NULL, &fId)))
      goto fail;

   if (!(font->lut = chckLutNew(GLHCK_FONT_LUT_SIZE, (int)GLHCK_INVALID_GLYPH, sizeof(chckPoolIndex))))
      goto fail;

   chckPoolIndex tId;
   if (!(textTexture = chckPoolAdd(*(chckPool**)get($textures, handle), NULL, &tId)))
      goto fail;

   /* reference texture */
   glhckHandleRef(texture);
   textTexture->texture = texture;

   /* make sure no glyphs are cached to this texture */
   textTexture->bitmap = 1;

   /* store internal dimensions for mapping */
   textTexture->internalWidth = (float)1.0f / glhckTextureGetWidth(texture);
   textTexture->internalHeight = (float)1.0f / glhckTextureGetHeight(texture);

   /* store normalized line height */
   int fh = fabs(ascent - descent);
   if (fh == 0)
      fh = 1;

   font->ascender = (float)ascent/fh;
   font->descender = (float)descent/fh;
   font->lineHeight = (float)(fh + lineGap)/fh;
   font->textureId = tId;
   font->type = FONT_BMP;

   RET(0, "%zu", fId + 1);
   return fId + 1;

fail:
   if (textTexture) {
      IFDO(glhckHandleRelease, textTexture->texture);
      chckPoolRemove(*(chckPool**)get($textures, handle), tId);
   }
   if (font) {
      chckPoolRemove(*(chckPool**)get($fonts, handle), fId);
   }
   RET(0, "%d", 0);
   return 0;
}

/* \brief new bitmap font */
GLHCKAPI glhckFont glhckTextFontNewFromImage(const glhckHandle handle, const char *file, int ascent, int descent, int lineGap)
{
   glhckHandle texture = 0;
   CALL(0, "%s, %s, %d, %d, %d", glhckHandleRepr(handle), file, ascent, descent, lineGap);
   assert(file);

   if (!(texture = glhckTextureNewFromFile(file, NULL, NULL)))
      goto fail;

   glhckTextureParameter(texture, glhckTextureDefaultSpriteParameters());

   glhckFont font;
   if ((font = glhckTextFontNewFromTexture(handle, texture, ascent, descent, lineGap)))
      goto fail;

   glhckHandleRelease(texture);
   RET(0, "%u", font);
   return font;

fail:
   IFDO(glhckHandleRelease, texture);
   RET(0, "%d", 0);
   return 0;
}

/* \brief add new glyph to bitmap font */
GLHCKAPI void glhckTextGlyphNew(const glhckHandle handle,
      glhckFont font, const char *s,
      short size, short base, int x, int y, int w, int h,
      float xoff, float yoff, float xadvance)
{
   CALL(2, "%s, %u, \"%s\", %d, %d, %d, %d, %d, %d, %f, %f, %f",
         glhckHandleRepr(handle), font, s, size, base, w, y, w, h, xoff, yoff, xadvance);
   assert(s);

   struct font *f;
   if (!(f = chckPoolGet(*(chckPool**)get($fonts, handle), font - 1)))
      return;

   assert(f->type == FONT_BMP);

   /* decode utf8 character */
   unsigned int codepoint = 0, state = 0;
   for (; *s; ++s) {
      if (!decutf8(&state, &codepoint, *(unsigned char*)s))
         break;
   }

   if (state != UTF8_ACCEPT)
      return;

   if (!f->glyphs && !(f->glyphs = chckPoolNew(32, 32, sizeof(struct glyph))))
      return;

   /* allocate space for new glyph */
   chckPoolIndex id;
   struct glyph *g;
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

/* \brief clear text from stash */
GLHCKAPI void glhckTextClear(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));

   struct glhckTextTexture *t;
   for (chckPoolIndex iter = 0; (t = chckPoolIter(*(chckPool**)get($textures, handle), &iter));)
      t->geometry.vertexCount = 0;
}

/* \brief draw text using font */
GLHCKAPI void glhckTextStash(const glhckHandle handle, const glhckFont font, float size, float x, float y, const char *s, float *outWidth)
{
   CALL(2, "%s, %u, %f, %f, %f, %s, %p", glhckHandleRepr(handle), font, size, x, y, s, outWidth);
   assert(handle > 0 && font > 0);

   if (outWidth)
      *outWidth = 0;

   struct font *f;
   if (!(f = chckPoolGet(*(chckPool**)get($fonts, handle), font - 1)))
      return;

   unsigned int codepoint = 0, state = 0;
   for (; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s))
         continue;

      struct glyph *g;
      if (!(g = getGlyph(handle, f, codepoint, size)))
         continue;

      struct glhckTextTexture *t;
      if (!(t = chckPoolGet(*(chckPool**)get($textures, handle), g->textureId)))
         continue;

      int vcount = t->geometry.vertexCount;
#if GLHCK_TRISTRIP
      int newVcount = (vcount ? vcount + 6 : 4);
#else
      int newVcount = vcount + 6;
#endif
      if (newVcount >= t->geometry.allocatedCount) {
         if (textGeometryAllocateMore(&t->geometry) != RETURN_OK) {
            DEBUG(GLHCK_DBG_WARNING, "TEXT :: [%s] out of memory!", glhckHandleRepr(handle));
            continue;
         }
      }

      /* should not ever fail */
      struct quad q;
      if (getQuad(handle, f, g, size, &x, &y, &q) != RETURN_OK)
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

   if (outWidth)
      *outWidth = x;
}

/* \brief set shader to text */
GLHCKAPI void glhckTextShader(const glhckHandle handle, const glhckHandle shader)
{
   setHandle($shader, handle, shader);
}

/* \brief get text's shader */
GLHCKAPI glhckHandle glhckTextGetShader(const glhckHandle handle)
{
   return *(glhckHandle*)get($shader, handle);
}

/* \brief create texture from text */
GLHCKAPI glhckHandle glhckTextRTT(const glhckHandle handle, const glhckFont font, float size, const char *s, const glhckTextureParameters *params)
{
   glhckHandle texture = 0, fbo = 0;
   CALL(1, "%s, %u, %f, %s, %p", glhckHandleRepr(handle), font, size, s, params);

   if (!(texture = glhckTextureNew()))
      goto fail;

   float linew, ascender, descender;
   glhckTextClear(handle);
   glhckTextGetMetrics(handle, font, 1.0f, &ascender, &descender, NULL);
   glhckTextStash(handle, font, size, 0, size + descender * size, s, &linew);

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
   glhckRenderClearColor(0);
   glhckRenderClear(GLHCK_COLOR_BUFFER_BIT);
   glhckRenderText(handle);
   glhckFramebufferEnd(fbo);

   // texture->importFlags |= GLHCK_TEXTURE_IMPORT_TEXT;
   glhckHandleRelease(fbo);
   glhckTextClear(handle);

   RET(1, "%s", glhckHandleRepr(texture));
   return texture;

fail:
   glhckTextClear(handle);
   IFDO(glhckHandleRelease, fbo);
   IFDO(glhckHandleRelease, texture);
   RET(1, "%s", glhckHandleRepr(0));
   return 0;
}

#if 0
/* \brief create plane object from text */
GLHCKAPI glhckHandle glhckTextPlane(const glhckHandle handle, const glhckFont font, float size, const char *s, const glhckTextureParameters *params)
{
   glhckHandle sprite = 0, texture = 0;
   CALL(1, "%s, %u, %f, %s, %p", glhckHandleRepr(handle), font, size, s, params);

   if (!(texture = glhckTextRTT(handle, font, size, s, params)))
      goto fail;

   if (!(sprite = glhckSpriteNew(texture, 0, 0)))
      goto fail;

   glhckHandleRelease(texture);
   RET(1, "%s", glhckHandleRepr(sprite));
   return sprite;

fail:
   IFDO(glhckHandleRelease, texture);
   IFDO(glhckHandleRelease, sprite);
   RET(1, "%s", glhckHandleRepr(0));
   return 0;
}
#endif

/* vim: set ts=8 sw=3 tw=0 :*/
