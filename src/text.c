#include "internal.h"
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

#define GLHCK_TEXT_HASH_SIZE 256
#define GLHCK_TEXT_ROWS 128
#define GLHCK_TEXT_VERT_COUNT (6*GLHCK_TEXT_ROWS)

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
   struct __GLHCKtextTexture *texture;
   float xadv, xoff, yoff;
   unsigned int code;
   int x1, y1, x2, y2;
   int next;
   short size;
} __GLHCKtextGlyph;

/* \brief font object */
typedef struct __GLHCKtextFont
{
   struct stbtt_fontinfo font;
   struct __GLHCKtextFont *next;
   struct __GLHCKtextGlyph *glyphCache;
   struct __GLHCKtextTexture *texture; /* only used on bitmap fonts */
   void *data;
   float ascender, descender, lineHeight;
   unsigned int id, glyphCount;
   int lut[GLHCK_TEXT_HASH_SIZE];
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
static unsigned int decutf8(unsigned int* state, unsigned int* codep, unsigned int byte)
{
   unsigned int type = utf8d[byte];
   *codep = (*state != UTF8_ACCEPT) ?
      (byte & 0x3fu) | (*codep << 6) :
      (0xff >> type) & (byte);
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
   int i, j;
   unsigned char *str = (unsigned char *)buffer;

   if ((i = j = encutf8len(ch)) == 1) {
      *str = ch;
      return 1;
   }

   if (bufferSize < i) return 0;
   for (; j > 1; j--) str[j-1] = 0x80 | (0x3F & (ch >> ((i - j) * 6)));
   *str = (~0) << (8 - i);
   *str |= (ch >> (i * 6 - 6));
   return i;
}

/* hasher */
static unsigned int hashint(unsigned int a)
{
   a += ~(a<<15);
   a ^=  (a>>10);
   a +=  (a<<3);
   a ^=  (a>>6);
   a += ~(a<<11);
   a ^=  (a>>16);
   return a;
}

/* \brief resize geometry data */
static int _glhckTextGeometryAllocateMore(__GLHCKtextGeometry *geometry)
{
   int newCount;
#if GLHCK_TEXT_FLOAT_PRECISION
   glhckVertexData2f *newData;
#else
   glhckVertexData2s *newData;
#endif

   newCount = geometry->vertexCount + GLHCK_TEXT_VERT_COUNT;
#if GLHCK_TEXT_FLOAT_PRECISION
   if (!(newData = _glhckRealloc(geometry->vertexData, geometry->allocatedCount, newCount, sizeof(glhckVertexData2f))))
      goto fail;
#else
   if (!(newData = _glhckRealloc(geometry->vertexData, geometry->allocatedCount, newCount, sizeof(glhckVertexData2s))))
      goto fail;
#endif

   geometry->vertexData = newData;
   geometry->allocatedCount = newCount;
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief resize row data */
static int _glhckTextTextureRowsAllocateMore(__GLHCKtextTexture *texture)
{
   int newCount;
   struct __GLHCKtextTextureRow *newRows;

   newCount = texture->rowsCount + GLHCK_TEXT_ROWS;
   if (!(newRows = _glhckRealloc(texture->rows, texture->allocatedCount, newCount, sizeof(__GLHCKtextTextureRow))))
      goto fail;

   texture->rows = newRows;
   texture->allocatedCount = newCount;
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief creates new texture to the cache */
static int _glhckTextTextureNew(glhckText *object, glhckTextureFormat format, glhckDataType type, const void *data)
{
   __GLHCKtextTexture *texture = NULL, *t;
   glhckTextureParameters nparams;
   CALL(0, "%p, %u, %u, %p", object, format, type, data);
   assert(object);

   if (!(texture = _glhckCalloc(1, sizeof(__GLHCKtextTexture))))
      goto fail;

   if (!(texture->texture = glhckTextureNew()))
      goto fail;

   if (glhckTextureCreate(texture->texture, GLHCK_TEXTURE_2D, 0, object->cacheWidth, object->cacheHeight,
            0, 0, format, type, 0, data) != RETURN_OK)
      goto fail;

   /* make sure mipmap is disabled from the parameters */
   memcpy(&nparams, glhckTextureDefaultParameters(), sizeof(glhckTextureParameters));
   nparams.mipmap = 0;
   /* FIXME: do a function that checks, if filter contains mipmaps, and then replace */
   nparams.minFilter = GLHCK_FILTER_NEAREST;
   nparams.wrapR = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapS = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapT = GLHCK_WRAP_CLAMP_TO_EDGE;
   glhckTextureParameter(texture->texture, &nparams);

   /* set internal texture dimensions for mapping */
   texture->internalWidth = (float)1.0f/object->cacheWidth;
   texture->internalHeight = (float)1.0f/object->cacheHeight;

   for (t = object->textureCache; t && t->next; t = t->next);
   if (!t) object->textureCache = texture;
   else t->next = texture;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   if (texture) {
      IFDO(glhckTextureFree, texture->texture);
   }
   IFDO(_glhckFree, texture);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief free cache texture from text object
 * NOTE: all glyphs from fonts pointing to this cache texture are flushed after this! */
static void _glhckTextTextureFree(glhckText *object, __GLHCKtextTexture *texture)
{
   unsigned int i;
   __GLHCKtextTexture *t, *tp;
   __GLHCKtextFont *f;
   CALL(1, "%p, %p", object, texture);
   assert(object && texture);

   /* search texture */
   for (t = object->textureCache, tp = NULL; t && t != texture; tp = t, t = t->next);
   if (!t) return;

   /* remove glyphs from fonts that contain this texture */
   for (f = object->fontCache; f; f = f->next) {
      for (i = 0; i != f->glyphCount; ++i) {
         if (f->glyphCache[i].texture != t) continue;
         IFDO(_glhckFree, f->glyphCache);
         f->glyphCount = 0;
         memset(f->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
         break;
      }
   }

   if (!tp) object->textureCache = t->next;
   else tp->next = t->next;
   IFDO(glhckTextureFree, t->texture);
   IFDO(_glhckFree, t->geometry.vertexData);
   IFDO(_glhckFree, t->rows);
   _glhckFree(t);
}

/* \brief get texture where to cache the glyph, will allocate new cache page if glyph doesn't fit any existing pages. */
static __GLHCKtextTexture* _glhckTextGetTextureCache(glhckText *object, int gw, int gh, __GLHCKtextTextureRow **row)
{
   short py;
   int i, rh;
   __GLHCKtextTexture *texture;
   __GLHCKtextTextureRow *br;

   /* glyph should not be larger than texture */
   if (gw >= object->cacheWidth || gh >= object->cacheHeight)
      return NULL;

   br = NULL;
   rh = (gh+7) & ~7;
   if (!(texture = object->textureCache)) {
      if (_glhckTextTextureNew(object, GLHCK_ALPHA, GLHCK_DATA_UNSIGNED_BYTE, NULL) != RETURN_OK)
         return NULL;
      texture = object->textureCache;
   }
   while (!br) {
      /* skip textures with INT_MAX rows (these are either really big text blobs or bitmap fonts) */
      while (texture && texture->rowsCount == INT_MAX) {
         if (!texture->next) {
            if (_glhckTextTextureNew(object, GLHCK_ALPHA, GLHCK_DATA_UNSIGNED_BYTE, NULL) != RETURN_OK)
               return NULL;
         }
         texture = texture->next;
      }

      for (i = 0; i < texture->rowsCount; ++i) {
         if (texture->rows[i].h == rh &&
             texture->rows[i].x+gw+1 <= object->cacheWidth)
            br = &texture->rows[i];
      }

      /* we got row, break */
      if (br) break;

      /* if no row found:
       * - add new row
       * - try next texture
       * - create new texture */

      /* reset py */
      py = 0;

      /* this texture is at least used */
      if (texture->rowsCount) {
         py = texture->rows[texture->rowsCount-1].y +
              texture->rows[texture->rowsCount-1].h + 1;

         /* not enough space */
         if (py+rh > object->cacheHeight) {
            /* go to next texture, if this was used */
            if (texture->next) {
               texture = texture->next;
               continue;
            }

            /* as last resort create new texture, if this was used */
            if (_glhckTextTextureNew(object, GLHCK_ALPHA, GLHCK_DATA_UNSIGNED_BYTE, NULL) != RETURN_OK)
               return NULL;

            /* cycle and hope for best */
            texture = texture->next;
            continue;
         }
      }

      /* add new row */
      if (texture->rowsCount >= texture->allocatedCount) {
         if (_glhckTextTextureRowsAllocateMore(texture) != RETURN_OK) {
            DEBUG(GLHCK_DBG_WARNING, "TEXT :: [%p] out of memory!", object);
            continue;
         }
      }
      br = &texture->rows[texture->rowsCount];
      br->x = 0; br->y = py; br->h = rh;
      texture->rowsCount++;
   }

   if (row) *row = br;
   return texture;
}

/* \brief get glyph from font */
__GLHCKtextGlyph* _glhckTextGetGlyph(glhckText *object, __GLHCKtextFont *font, unsigned int code, short isize)
{
   int i, x1, y1, x2, y2, gw, gh, gid, advance, lsb;
   unsigned int h;
   unsigned char *data;
   float scale;
   float size = (float)isize/10.0f;
   __GLHCKtextGlyph *glyph;
   __GLHCKtextTexture *texture;
   __GLHCKtextTextureRow *row;

   /* find code and size */
   h = hashint(code) & (GLHCK_TEXT_HASH_SIZE-1);
   i = font->lut[h];
   while (font->glyphCache && i != -1) {
      if (font->glyphCache[i].code == code &&
         (font->type == GLHCK_FONT_BMP ||
          font->glyphCache[i].size == isize))
         return &font->glyphCache[i];
      i = font->glyphCache[i].next;
   }

   /* user hasn't added the glyph */
   if (font->type == GLHCK_FONT_BMP)
      return NULL;

   /* create glyph if ttf font */
   scale = stbtt_ScaleForPixelHeight(&font->font, size);
   gid   = stbtt_FindGlyphIndex(&font->font, code);
   stbtt_GetGlyphHMetrics(&font->font, gid, &advance, &lsb);
   stbtt_GetGlyphBitmapBox(&font->font, gid, scale, scale, &x1, &y1, &x2, &y2);
   gw = x2-x1; gh = y2-y1;

   /* get cache texture where to store the glyph */
   if (!(texture = _glhckTextGetTextureCache(object, gw, gh, &row)))
      return NULL;

   /* create new glyph */
   glyph = _glhckRealloc(font->glyphCache, font->glyphCount, font->glyphCount+1, sizeof(__GLHCKtextGlyph));
   if (!glyph) return NULL;
   font->glyphCache = glyph;

   /* init glyph */
   glyph = &font->glyphCache[font->glyphCount++];
   memset(glyph, 0, sizeof(__GLHCKtextGlyph));
   glyph->code = code;
   glyph->size = isize;
   glyph->texture = texture;
   glyph->x1 = row->x;
   glyph->y1 = row->y;
   glyph->x2 = glyph->x1+gw;
   glyph->y2 = glyph->y1+gh;
   glyph->xadv = scale * advance;
   glyph->xoff = (float)x1;
   glyph->yoff = (float)y1;

   /* insert to hash lookup */
   glyph->next  = font->lut[h];
   font->lut[h] = font->glyphCount-1;

   /* this glyph is a space, don't render it */
   if (!gw || !gh)
      return glyph;

   /* advance in row */
   row->x += gw+1;

   /* rasterize */
   if ((data = _glhckMalloc(gw*gh))) {
      stbtt_MakeGlyphBitmap(&font->font, data, gw, gh, gw, scale, scale, gid);
      glhckTextureFill(texture->texture, 0, glyph->x1, glyph->y1, 0, gw, gh, 0,
            GLHCK_ALPHA, GLHCK_DATA_UNSIGNED_BYTE, gw*gh, data);
      _glhckFree(data);
   }

   return glyph;
}

/* \brief get quad for text drawing */
static int _getQuad(glhckText *object, __GLHCKtextFont *font, __GLHCKtextGlyph *glyph, short isize,
      float *x, float *y, __GLHCKtextQuad *q)
{
   glhckVector2f v1, v2, t1, t2;
   int rx, ry;
   float scale = 1.0f;
   assert(object && font && glyph);
   assert(x && y && q);

   if (font->type == GLHCK_FONT_BMP) scale = (float)isize/(glyph->size*10.0f);

   rx = floorf(*x + scale * glyph->xoff);
   ry = floorf(*y + scale * glyph->yoff);

   v2.x = rx;
   v1.y = ry;
   v1.x = rx + scale * (glyph->x2 - glyph->x1);
   v2.y = ry + scale * (glyph->y2 - glyph->y1);

   t2.x = glyph->x1 * glyph->texture->internalWidth;
   t1.y = glyph->y1 * glyph->texture->internalHeight;
   t1.x = glyph->x2 * glyph->texture->internalWidth;
   t2.y = glyph->y2 * glyph->texture->internalHeight;

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
static unsigned int glhckTextFontNewInternal(glhckText *object, const _glhckBitmapFontInfo *font, int *nativeSize)
{
   unsigned int id, c, r;
   glhckTexture *texture;
   glhckTextureParameters nparams;
   char utf8buf[5];
   assert(object && font);
   if (nativeSize) *nativeSize = 0;
   memset(utf8buf, 0, sizeof(utf8buf));

   /* create texture for bitmap font */
   if (!(texture = glhckTextureNew()))
      goto fail;

   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, font->width, font->height, 0, 0,
            font->format, font->type, 0, font->data) != RETURN_OK)
      goto fail;

   /* make sure mipmap is disabled from the parameters */
   memcpy(&nparams, glhckTextureDefaultParameters(), sizeof(glhckTextureParameters));
   nparams.mipmap = 0;
   /* FIXME: do a function that checks, if filter contains mipmaps, and then replace */
   nparams.minFilter = GLHCK_FILTER_NEAREST;
   nparams.wrapR = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapS = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapT = GLHCK_WRAP_CLAMP_TO_EDGE;
   glhckTextureParameter(texture, &nparams);

   /* create font from texture */
   if (!(id = glhckTextFontNewFromTexture(object, texture, font->ascent, font->descent, font->lineGap)))
      return 0;

   /* fill glyphs to the monospaced bitmap font */
   for (r = 0; r != font->rows; ++r) {
      for (c = 0; c != font->columns; ++c) {
         encutf8(r*font->columns+c, utf8buf, sizeof(utf8buf));
         glhckTextGlyphNew(object, id, utf8buf, font->fontSize, font->fontSize,
               c*font->fontSize, r*font->fontSize, font->fontSize/2, font->fontSize, 0, 0, font->fontSize/2);
      }
   }

   if (nativeSize) *nativeSize = font->fontSize;
   return id;

fail:
   IFDO(glhckTextureFree, texture);
   return 0;
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

   /* increase reference */
   object->refCounter++;

   object->cacheWidth = cacheWidth;
   object->cacheHeight = cacheHeight;
#if GLHCK_TEXT_FLOAT_PRECISION
   object->textureRange = 1;
#else
   object->textureRange = SHRT_MAX;
#endif

   /* default color */
   glhckTextColorb(object, 255, 255, 255, 255);

   /* insert to world */
   _glhckWorldInsert(text, object, glhckText*);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(_glhckFree, object);
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
   __GLHCKtextFont *f, *fn;
   __GLHCKtextTexture *t, *tn;
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free texture cache */
   for (t = object->textureCache; t; t = tn) {
      tn = t->next;
      IFDO(glhckTextureFree, t->texture);
      IFDO(_glhckFree, t->geometry.vertexData);
      IFDO(_glhckFree, t->rows);
      _glhckFree(t);
   }

   /* free font cache */
   for (f = object->fontCache; f; f = fn) {
      fn = f->next;
      IFDO(_glhckFree, f->glyphCache);
      IFDO(_glhckFree, f->data);
      _glhckFree(f);
   }

   /* free shader */
   glhckTextShader(object, NULL);

   /* remove text */
   _glhckWorldRemove(text, object, glhckText*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief free font from text */
GLHCKAPI void glhckTextFontFree(glhckText *object, unsigned int font_id)
{
   __GLHCKtextFont *f, *fp;
   __GLHCKtextGlyph *g;
   CALL(1, "%p, %u", object, font_id);
   assert(object);

   /* search font */
   for (f = object->fontCache, fp = NULL; f && f->id != font_id; fp = f, f = f->next);
   if (!f) return;

   /* free font */
   for (g = f->glyphCache; g; g = (g->next!=-1?&f->glyphCache[g->next]:NULL))
      _glhckTextTextureFree(object, g->texture);
   IFDO(_glhckFree, f->glyphCache);
   f->glyphCount = 0;
   memset(f->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   if (!fp) object->fontCache = f->next;
   else fp->next = f->next;
   _glhckFree(f);
}

/* \brief flush text glyph cache */
GLHCKAPI void glhckTextFlushCache(glhckText *object)
{
   __GLHCKtextTexture *t, *tn, *tp;
   __GLHCKtextFont *f;
   CALL(1, "%p", object);
   assert(object);

   /* free texture cache */
   for (t = object->textureCache, tp = NULL; t; t = tn) {
      tn = t->next;

      /* skip bitmap font textures */
      for (f = object->fontCache; f && f->texture != t; f = f->next);
      if (f) { tp = t; continue; }

      if (!object->textureCache) object->textureCache = tp;
      if (tp) tp->next = t->next;
      if (t == object->textureCache) t = NULL;
      IFDO(glhckTextureFree, t->texture);
      IFDO(_glhckFree, t->geometry.vertexData);
      IFDO(_glhckFree, t->rows);
      _glhckFree(t);
   }

   /* free font cache */
   for (f = object->fontCache; f; f = f->next) {
      if (f->type == GLHCK_FONT_BMP) continue;
      IFDO(_glhckFree, f->glyphCache);
      f->glyphCount = 0;
      memset(f->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   }
}

/* \brief get text metrics */
GLHCKAPI void glhckTextGetMetrics(glhckText *object, unsigned int font_id,
      float size, float *ascender, float *descender, float *lineHeight)
{
   __GLHCKtextFont *font;
   CALL(1, "%p, %d, %f, %f, %f, %f", object, font_id, size, ascender, descender, lineHeight);
   assert(object);
   if (ascender) *ascender = 0.0f;
   if (descender) *descender = 0.0f;
   if (lineHeight) *lineHeight = 0.0f;

   /* search font */
   for (font = object->fontCache; font && font->id != font_id; font = font->next);
   if (!font) return;

   /* must not fail */
   if (ascender)   *ascender   = font->ascender*size;
   if (descender)  *descender  = font->descender*size;
   if (lineHeight) *lineHeight = font->lineHeight*size;
}

/* \brief get min max of text */
GLHCKAPI void glhckTextGetMinMax(glhckText *object, unsigned int font_id, float size,
      const char *s, kmVec2 *min, kmVec2 *max)
{
   unsigned int codepoint, state = 0;
   short isize = (short)size*10.0f;
   __GLHCKtextFont *font;
   __GLHCKtextGlyph *glyph;
   __GLHCKtextQuad q;
   float x, y;

   CALL(1, "%p, %u, %f, %s, %p, %p", object, font_id, size, s, min, max);
   assert(object && s && min && max);
   if (min) memset(min, 0, sizeof(kmVec2));
   if (max) memset(max, 0, sizeof(kmVec2));

   /* search font */
   for (font = object->fontCache; font && font->id != font_id; font = font->next);
   if (!font) return;

   for (x = 0, y = 0; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s)) continue;
      if (!(glyph = _glhckTextGetGlyph(object, font, codepoint, isize)))
         continue;
      if (_getQuad(object, font, glyph, isize, &x, &y, &q) != RETURN_OK)
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
   const glhckColorb color = {r, g, b, a};
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
GLHCKAPI unsigned int glhckTextFontNewKakwafont(glhckText *object, int *nativeSize)
{
   unsigned int id;
   CALL(0, "%p", object);
   assert(object);
   id = glhckTextFontNewInternal(object, &kakwafont, nativeSize);
   RET(0, "%d", id);
   return id;
}

/* \brief new font from memory */
GLHCKAPI unsigned int glhckTextFontNewFromMemory(glhckText *object, const void *data, size_t size)
{
   unsigned int id;
   int ascent, descent, fh, lineGap;
   __GLHCKtextFont *font, *f;
   CALL(0, "%p, %p", object, data);
   assert(object && data);

   /* allocate font */
   if (!(font = _glhckCalloc(1, sizeof(__GLHCKtextFont))))
      goto fail;

   /* init */
   memset(font->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   for (id = 1, f = object->fontCache; f; f = f->next, ++id);

   /* copy the data */
   if (!(font->data = _glhckCopy(data, size)))
      goto fail;

   /* create truetype font */
   if (!stbtt_InitFont(&font->font, font->data, 0))
      goto fail;

   /* store normalized line height */
   stbtt_GetFontVMetrics(&font->font, &ascent, &descent, &lineGap);
   fh = ascent - descent;
   font->ascender    = (float)ascent/fh;
   font->descender   = (float)descent/fh;
   font->lineHeight  = (float)(fh + lineGap)/fh;
   font->id          = id;
   font->type        = GLHCK_FONT_TTF;
   font->next        = object->fontCache;
   object->fontCache = font;

   RET(0, "%d", id);
   return id;

fail:
   if (font) {
      IFDO(_glhckFree, font->data);
   }
   IFDO(_glhckFree, font);
   RET(0, "%d", 0);
   return 0;
}

/* \brief new truetype font */
GLHCKAPI unsigned int glhckTextFontNew(glhckText *object, const char *file)
{
   FILE *f;
   size_t size;
   unsigned int id;
   unsigned char *data = NULL;
   CALL(0, "%p, %s", object, file);
   assert(object && file);

   /* open font */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 0, SEEK_END);
   size = ftell(f);
   fseek(f, 0, SEEK_SET);
   if (!(data = _glhckMalloc(size)))
      goto fail;

   /* read data */
   if (fread(data, 1, size, f) != size)
      goto read_fail;
   NULLDO(fclose, f);

   /* read and add the new font to stash */
   if (!(id = glhckTextFontNewFromMemory(object, data, size)))
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
   return 0;
}

/* \brief new bitmap font from texture */
GLHCKAPI unsigned int glhckTextFontNewFromTexture(glhckText *object, glhckTexture *texture, int ascent, int descent, int lineGap)
{
   int fh;
   unsigned int id;
   __GLHCKtextFont *font, *f;
   __GLHCKtextTexture *textTexture, *t;
   CALL(0, "%p, %p, %d, %d, %d", object, texture, ascent, descent, lineGap);
   assert(object && texture);

   /* allocate font */
   if (!(font = _glhckCalloc(1, sizeof(__GLHCKtextFont))))
      goto fail;

   /* init */
   memset(font->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   for (id = 1, f = object->fontCache; f; f = f->next, ++id);

   /* allocate text texture */
   if (!(textTexture = _glhckCalloc(1, sizeof(__GLHCKtextTexture))))
      goto fail;

   /* reference texture */
   textTexture->texture = glhckTextureRef(texture);

   /* make sure no glyphs are cached to this texture */
   textTexture->rowsCount = INT_MAX;

   /* store internal dimensions for mapping */
   textTexture->internalWidth = 1.0f/texture->width;
   textTexture->internalHeight = 1.0f/texture->height;

   for (t = object->textureCache; t && t->next; t = t->next);
   if (!t) object->textureCache = textTexture;
   else t->next = textTexture;

   /* store normalized line height */
   fh = ascent - descent;
   if (fh == 0) fh = 1;
   font->ascender    = (float)ascent/fh;
   font->descender   = (float)descent/fh;
   font->lineHeight  = (float)(fh + lineGap)/fh;
   font->texture     = textTexture;
   font->id          = id;
   font->type        = GLHCK_FONT_BMP;
   font->next        = object->fontCache;
   object->fontCache = font;

   RET(0, "%d", id);
   return id;

fail:
   if (texture) {
      IFDO(glhckTextureFree, textTexture->texture);
   }
   IFDO(_glhckFree, texture);
   IFDO(_glhckFree, font);
   RET(0, "%d", 0);
   return 0;
}

/* \brief new bitmap font */
GLHCKAPI unsigned int glhckTextFontNewFromBitmap(glhckText *object, const char *file, int ascent, int descent, int lineGap)
{
   unsigned int id;
   glhckTextureParameters nparams;
   glhckTexture *texture;
   CALL(0, "%p, %s, %d, %d, %d", object, file, ascent, descent, lineGap);
   assert(object && file);

   /* load image */
   if (!(texture = glhckTextureNewFromFile(file, NULL, NULL)))
      goto fail;

   /* make sure mipmap is disabled from the parameters */
   memcpy(&nparams, glhckTextureDefaultParameters(), sizeof(glhckTextureParameters));
   nparams.mipmap = 0;
   /* FIXME: do a function that checks, if filter contains mipmaps, and then replace */
   nparams.minFilter = GLHCK_FILTER_NEAREST;
   nparams.wrapR = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapS = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapT = GLHCK_WRAP_CLAMP_TO_EDGE;
   glhckTextureParameter(texture, &nparams);

   if (!(id = glhckTextFontNewFromTexture(object, texture, ascent, descent, lineGap)))
      goto fail;

   glhckTextureFree(texture);
   RET(0, "%d", id);
   return id;

fail:
   IFDO(glhckTextureFree, texture);
   RET(0, "%d", 0);
   return 0;
}

/* \brief add new glyph to bitmap font */
GLHCKAPI void glhckTextGlyphNew(glhckText *object,
      unsigned int font_id, const char *s,
      short size, short base, int x, int y, int w, int h,
      float xoff, float yoff, float xadvance)
{
   unsigned int codepoint = 0, state = 0, hh;
   __GLHCKtextFont *font;
   __GLHCKtextGlyph *glyph;
   CALL(0, "%p, \"%s\", %d, %d, %d, %d, %d, %d, %f, %f, %f",
         object, s, size, base, w, y, w, h, xoff, yoff, xadvance);
   assert(object && s);

   /* search font */
   for (font = object->fontCache; font && font->id != font_id; font = font->next);
   if (!font || font->type != GLHCK_FONT_BMP)
      return;

   /* decode utf8 character */
   for (; *s; ++s)
      if (!decutf8(&state, &codepoint, *(unsigned char*)s)) break;
   if (state != UTF8_ACCEPT) return;

   /* allocate space for new glyph */
   glyph = _glhckRealloc(font->glyphCache, font->glyphCount, font->glyphCount+1, sizeof(__GLHCKtextGlyph));
   if (!glyph) return;
   font->glyphCache = glyph;

   /* init glyph */
   glyph = &font->glyphCache[font->glyphCount++];
   memset(glyph, 0, sizeof(__GLHCKtextGlyph));
   glyph->code = codepoint;
   glyph->texture = font->texture;
   glyph->size = size;
   glyph->x1 = x;   glyph->y1 = y;
   glyph->x2 = x+w; glyph->y2 = y+h;
   glyph->xoff = xoff;
   glyph->yoff = yoff - base;
   glyph->xadv = xadvance;

   /* insert to hash lookup */
   hh = hashint(codepoint) & (GLHCK_TEXT_HASH_SIZE-1);
   glyph->next   = font->lut[hh];
   font->lut[hh] = font->glyphCount-1;
}

/* \brief render all drawn text */
GLHCKAPI void glhckTextRender(glhckText *object)
{
   CALL(2, "%p", object); assert(object);
   GLHCKRA()->textRender(object);
}

/* \brief clear text from stash */
GLHCKAPI void glhckTextClear(glhckText *object)
{
   __GLHCKtextTexture *texture;
   for (texture = object->textureCache; texture; texture = texture->next) {
      texture->geometry.vertexCount = 0;
   }
}

/* \brief draw text using font */
GLHCKAPI void glhckTextStash(glhckText *object, unsigned int font_id, float size, float x, float y, const char *s, float *width)
{
   unsigned int i, codepoint, state = 0;
   short isize = (short)size*10.0f;
   int vcount, newVcount;
#if GLHCK_TEXT_FLOAT_PRECISION
   glhckVertexData2f *v;
#if GLHCK_TRISTRIP
   glhckVertexData2f *d;
#endif
#else
   glhckVertexData2s *v;
#if GLHCK_TRISTRIP
   glhckVertexData2s *d;
#endif
#endif
   __GLHCKtextTexture *texture;
   __GLHCKtextGlyph *glyph;
   __GLHCKtextFont *font;
   __GLHCKtextQuad q;
   CALL(2, "%p, %u, %f, %f, %f, %s, %p", object, font_id, size, x, y, s, width);
   assert(object && s);
   if (width) *width = 0;

   /* search font */
   for (font = object->fontCache; font && font->id != font_id; font = font->next);
   if (!font) return;

   for (; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s)) continue;
      if (!(glyph = _glhckTextGetGlyph(object, font, codepoint, isize)))
         continue;
      if (!(texture = glyph->texture))
         continue;

      vcount = texture->geometry.vertexCount;
#if GLHCK_TRISTRIP
      newVcount = (vcount?vcount+6:4)
#else
      newVcount = vcount+6;
#endif
      if (newVcount >= texture->geometry.allocatedCount) {
         if (_glhckTextGeometryAllocateMore(&texture->geometry) != RETURN_OK) {
            DEBUG(GLHCK_DBG_WARNING, "TEXT :: [%p] out of memory!", object);
            continue;
         }
      }

      /* should not ever fail */
      if (_getQuad(object, font, glyph, isize, &x, &y, &q) != RETURN_OK)
         continue;

      /* insert geometry data */
      v = &texture->geometry.vertexData[vcount];

      i = 0;
#if GLHCK_TRISTRIP
      /* degenerate */
      if (vcount) {
         d = &texture->geometry.vertexData[vcount-1];
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

      texture->geometry.vertexCount += i;
   }

   if (width) *width = x + (glyph?glyph->xadv:0);
}

/* \brief set shader to text */
GLHCKAPI void glhckTextShader(glhckText *object, glhckShader *shader)
{
   CALL(1, "%p, %p", object, shader);
   assert(object);
   if (object->shader == shader) return;
   if (object->shader) glhckShaderFree(object->shader);
   object->shader = (shader?glhckShaderRef(shader):NULL);
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
GLHCKAPI glhckTexture* glhckTextRTT(glhckText *object, unsigned int font_id, float size,
      const char *s, const glhckTextureParameters *params)
{
   glhckColorb oldClear;
   glhckTexture *texture = NULL;
   glhckFramebuffer *fbo = NULL;
   glhckTextureParameters nparams;
   float linew;
   CALL(1, "%p, %u, %f, %s", object, font_id, size, s);

   if (!(texture = glhckTextureNew()))
      goto fail;

   glhckTextStash(object, font_id, size, 0, size*0.82f, s, &linew);

   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, linew, size, 0, 0,
            GLHCK_RGBA, GLHCK_DATA_UNSIGNED_BYTE, 0, NULL) != RETURN_OK)
      goto fail;

   /* make sure mipmap is disabled from the parameters */
   memcpy(&nparams, (params?params:glhckTextureDefaultParameters()), sizeof(glhckTextureParameters));
   nparams.mipmap = 0;
   /* FIXME: do a function that checks, if filter contains mipmaps, and then replace */
   nparams.minFilter = GLHCK_FILTER_NEAREST;
   nparams.wrapR = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapS = GLHCK_WRAP_CLAMP_TO_EDGE;
   nparams.wrapT = GLHCK_WRAP_CLAMP_TO_EDGE;
   glhckTextureParameter(texture, &nparams);

   if (!(fbo = glhckFramebufferNew(GLHCK_FRAMEBUFFER)))
      goto fail;

   if (glhckFramebufferAttachTexture(fbo, texture, GLHCK_COLOR_ATTACHMENT0) != RETURN_OK)
      goto fail;

   /* set clear color */
   memcpy(&oldClear, glhckRenderGetClearColor(), sizeof(glhckColorb));
   glhckRenderClearColorb(0,0,0,0);

   glhckFramebufferRecti(fbo, 0, 0, linew, size);
   glhckFramebufferBegin(fbo);
   glhckRenderClear(GLHCK_COLOR_BUFFER);
   glhckTextRender(object);
   glhckFramebufferEnd(fbo);

   /* restore clear color */
   glhckRenderClearColor(&oldClear);

   texture->importFlags |= GLHCK_TEXTURE_IMPORT_TEXT;
   glhckFramebufferFree(fbo);

   RET(1, "%p", texture);
   return texture;

fail:
   IFDO(glhckFramebufferFree, fbo);
   IFDO(glhckTextureFree, texture);
   RET(1, "%p", NULL);
   return NULL;
}

/* \brief create plane object from text */
GLHCKAPI glhckObject* glhckTextPlane(glhckText *object, unsigned int font_id, float size,
      const char *s, const glhckTextureParameters *params)
{
   glhckTexture *texture;
   glhckObject *sprite = NULL;

   if (!(texture = glhckTextRTT(object, font_id, size, s, params)))
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
