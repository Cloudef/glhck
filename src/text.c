#include "internal.h"
#include <limits.h> /* for SHRT_MAX */
#include <stdio.h>

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

#define GLHCK_FONT_TTF      1
#define GLHCK_FONT_TTF_MEM  2
#define GLHCK_FONT_BMP      3

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
   struct __GLHCKtextGlyph *gcache;
   void *data;
   float ascender, descender, lineh;
   unsigned int id, type, numGlyph;
   int lut[GLHCK_TEXT_HASH_SIZE];
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
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief resize row data */
static int _glhckTextTextureRowsAllocateMore(__GLHCKtextTexture *texture)
{
   int newCount;
   struct __GLHCKtextTextureRow *newRows;

   newCount = texture->numRows + GLHCK_TEXT_ROWS;
   if (!(newRows = _glhckRealloc(texture->rows, texture->allocatedRows, newCount, sizeof(__GLHCKtextTextureRow))))
      goto fail;

   texture->rows = newRows;
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief creates new texture to the cache */
static int _glhckTextNewTexture(glhckText *object, glhckTextureFormat format, glhckDataType type, const void *data)
{
   __GLHCKtextTexture *texture = NULL, *t;
   glhckTextureParameters nparams;
   CALL(0, "%p, %d, %u", object, format);
   assert(object);

   if (!(texture = _glhckCalloc(1, sizeof(__GLHCKtextTexture))))
      goto fail;

   if (!(texture->texture = glhckTextureNew(NULL, NULL, NULL)))
      goto fail;

   if (glhckTextureCreate(texture->texture, GLHCK_TEXTURE_2D, 0, object->tw, object->th,
            0, 0, format, type, object->tw * object->th, data) != RETURN_OK)
      goto fail;

   /* make sure mipmap is disabled from the parameters */
   memcpy(&nparams, glhckTextureDefaultParameters(), sizeof(glhckTextureParameters));
   nparams.mipmap = 0;
   /* FIXME: do a function that checks, if filter contains mipmaps, and then replace */
   nparams.minFilter = GLHCK_FILTER_NEAREST;
   glhckTextureParameter(texture->texture, &nparams);

   for (t = object->tcache; t && t->next; t = t->next);
   if (!t) object->tcache = texture;
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

/* \brief get glyph from font */
__GLHCKtextGlyph* _glhckTextGetGlyph(glhckText *object, __GLHCKtextFont *font,
      unsigned int code, short isize)
{
   unsigned int h;
   unsigned char *data;
   int i, x1, y1, x2, y2, gw, rh, gh, gid, advance, lsb;
   float scale;
   float size = (float)isize/10.0f;
   short py;
   __GLHCKtextTexture *texture;
   __GLHCKtextGlyph *glyph;
   __GLHCKtextTextureRow *br;

   /* find code and size */
   h = hashint(code) & (GLHCK_TEXT_HASH_SIZE-1);
   i = font->lut[h];
   while (font->gcache && i != -1) {
      if (font->gcache[i].code == code &&
         (font->type == GLHCK_FONT_BMP ||
          font->gcache[i].size == isize))
         return &font->gcache[i];
      i = font->gcache[i].next;
   }

   /* user hasn't added the glyph */
   if (font->type == GLHCK_FONT_BMP)
      return NULL;

   /* create glyph if ttf font */
   scale = stbtt_ScaleForPixelHeight(&font->font, size);
   gid   = stbtt_FindGlyphIndex(&font->font, code);
   stbtt_GetGlyphHMetrics(&font->font, gid, &advance, &lsb);
   stbtt_GetGlyphBitmapBox(&font->font, gid, scale, scale,
         &x1, &y1, &x2, &y2);
   gw = x2-x1;
   gh = y2-y1;

   /* glyph should not be larger than texture */
   if (gw >= object->tw || gh >= object->th)
      return NULL;

   br = NULL;
   rh = (gh+7) & ~7;
   texture = object->tcache;
   while (!br) {
      for (i = 0; i < texture->numRows; ++i) {
         if (texture->rows[i].h == rh &&
             texture->rows[i].x+gw+1 <= object->tw)
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
      if (texture->numRows) {
         py = texture->rows[texture->numRows-1].y +
              texture->rows[texture->numRows-1].h + 1;

         /* not enough space */
         if (py+rh > object->th) {
            /* go to next texture, if this was used */
            if (texture->next) {
               texture = texture->next;
               continue;
            }

            /* as last resort create new texture, if this was used */
            if (_glhckTextNewTexture(object, GLHCK_ALPHA, GLHCK_DATA_UNSIGNED_BYTE, NULL) != RETURN_OK)
               return NULL;

            /* cycle and hope for best */
            texture = texture->next;
            continue;
         }
      }

      /* add new row */
      if (texture->numRows >= texture->allocatedRows) {
         if (_glhckTextTextureRowsAllocateMore(texture) != RETURN_OK) {
            DEBUG(GLHCK_DBG_WARNING, "TEXT :: [%p] out of memory!", object);
            continue;
         }
      }
      br = &texture->rows[texture->numRows];
      br->x = 0; br->y = py; br->h = rh;
      texture->numRows++;
   }

   /* create new glyph */
   glyph = _glhckRealloc(font->gcache,
         font->numGlyph, font->numGlyph+1, sizeof(__GLHCKtextGlyph));
   if (!glyph) return NULL;
   font->gcache = glyph;

   /* init glyph */
   glyph = &font->gcache[font->numGlyph++];
   memset(glyph, 0, sizeof(__GLHCKtextGlyph));
   glyph->code = code;
   glyph->size = isize;
   glyph->texture = texture;
   glyph->x1 = br->x;
   glyph->y1 = br->y;
   glyph->x2 = glyph->x1+gw;
   glyph->y2 = glyph->y1+gh;
   glyph->xadv = scale * advance;
   glyph->xoff = (float)x1;
   glyph->yoff = (float)y1;
   glyph->next = 0;

   /* advance in row */
   br->x += gw+1;

   /* insert to hash lookup */
   glyph->next  = font->lut[h];
   font->lut[h] = font->numGlyph-1;

   /* this glyph is a space, don't render it */
   if (!gw || !gh)
      return glyph;

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
static int _getQuad(glhckText *object, __GLHCKtextFont *font, __GLHCKtextGlyph *glyph,
      short isize, float *x, float *y, __GLHCKtextQuad *q)
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

   t2.x = (glyph->x1) * object->itw;
   t1.y = (glyph->y1) * object->ith;
   t1.x = (glyph->x2) * object->itw;
   t2.y = (glyph->y2) * object->ith;

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

/***
 * public api
 ***/

/* \brief create new text stack */
GLHCKAPI glhckText* glhckTextNew(int cachew, int cacheh)
{
   glhckText *object;
   CALL(0, "%d, %d", cachew, cacheh);

   /* allocate text stack */
   if (!(object = _glhckCalloc(1, sizeof(glhckText))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   object->tw  = cachew;
   object->th  = cacheh;
   object->itw = (float)1.0f/cachew;
   object->ith = (float)1.0f/cacheh;

   /* create first texture */
   if (_glhckTextNewTexture(object, GLHCK_ALPHA, GLHCK_DATA_UNSIGNED_BYTE, NULL) != RETURN_OK)
      goto fail;

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
   for (t = object->tcache; t; t = tn) {
      tn = t->next;
      IFDO(glhckTextureFree, t->texture);
      IFDO(_glhckFree, t->geometry.vertexData);
      IFDO(_glhckFree, t->rows);
      _glhckFree(t);
   }

   /* free font cache */
   for (f = object->fcache; f; f = fn) {
      fn = f->next;
      IFDO(_glhckFree, f->gcache);
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

/* \brief get text metrics */
GLHCKAPI void glhckTextGetMetrics(glhckText *object, unsigned int font_id,
      float size, float *ascender, float *descender, float *lineh)
{
   __GLHCKtextFont *font;
   CALL(1, "%p, %d, %f, %f, %f, %f", object, font_id, size, ascender, descender, lineh);
   assert(object);

   /* search font */
   for (font = object->fcache; font && font->id != font_id; font = font->next);

   /* not found */
   if (!font || (font->type != GLHCK_FONT_BMP && !font->data))
      return;

   /* must not fail */
   if (ascender)  *ascender  = font->ascender*size;
   if (descender) *descender = font->descender*size;
   if (lineh)     *lineh     = font->lineh*size;
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

   /* search font */
   for (font = object->fcache; font && font->id != font_id; font = font->next);

   /* not found */
   if (!font || (font->type != GLHCK_FONT_BMP && !font->data))
      return;

   min->x = max->x = x = 0;
   min->y = max->y = y = 0;

   for (; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s)) continue;
      if (!(glyph = _glhckTextGetGlyph(object, font, codepoint, isize)))
         continue;
      if (_getQuad(object, font, glyph, isize, &x, &y, &q) != RETURN_OK)
         continue;

      glhckMinV2(min, &q.v1);
      glhckMaxV2(max, &q.v1);
      glhckMinV2(min, &q.v2);
      glhckMaxV2(max, &q.v2);
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

/* \brief new font from memory */
GLHCKAPI unsigned int glhckTextNewFontFromMemory(glhckText *object, const void *data, size_t size)
{
   unsigned int id;
   int ascent, descent, fh, line_gap;
   __GLHCKtextFont *font, *f;
   CALL(0, "%p, %p", object, data);
   assert(object && data);

   /* allocate font */
   if (!(font = _glhckCalloc(1, sizeof(__GLHCKtextFont))))
      goto fail;

   /* init */
   memset(font->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   for (id = 1, f = object->fcache; f; f = f->next) ++id;
   font->data = _glhckCopy(data, size);

   /* create truetype font */
   if (!stbtt_InitFont(&font->font, font->data, 0))
      goto fail;

   /* store normalized line height */
   stbtt_GetFontVMetrics(&font->font, &ascent, &descent, &line_gap);
   fh = ascent - descent;
   font->ascender  = (float)ascent/fh;
   font->descender = (float)descent/fh;
   font->lineh     = (float)(fh + line_gap)/fh;

   font->id       = id;
   font->type     = GLHCK_FONT_TTF_MEM;
   font->next     = object->fcache;
   object->fcache = font;

   RET(0, "%d", id);
   return id;

fail:
   IFDO(_glhckFree, font);
   RET(0, "%d", 0);
   return 0;
}

/* \brief new truetype font */
GLHCKAPI unsigned int glhckTextNewFont(glhckText *object, const char *file)
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
   fread(data, 1, size, f);
   NULLDO(fclose, f);

   /* read and add the new font to stash */
   if (!(id = glhckTextNewFontFromMemory(object, data, size)))
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

/* \brief new bitmap font */
GLHCKAPI unsigned int glhckTextNewFontFromBitmap(glhckText *object,
      const char *file, int ascent, int descent, int line_gap)
{
   int fh;
   unsigned int id;
   glhckTextureParameters nparams;
   __GLHCKtextFont *font, *f;
   __GLHCKtextTexture *texture = NULL, *t;
   CALL(0, "%p, %s, %d, %d, %d", object, file, ascent, descent, line_gap);
   assert(object && file);

    /* allocate font */
   if (!(font = _glhckCalloc(1, sizeof(__GLHCKtextFont))))
      goto fail;

   /* init */
   memset(font->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   for (id = 1, f = object->fcache; f; f = f->next) ++id;

   if (!(texture = _glhckCalloc(1, sizeof(__GLHCKtextTexture))))
      goto fail;

   /* load image */
   if (!(texture->texture = glhckTextureNew(file, NULL, NULL)))
      goto fail;

   /* make sure mipmap is disabled from the parameters */
   memcpy(&nparams, glhckTextureDefaultParameters(), sizeof(glhckTextureParameters));
   nparams.mipmap = 0;
   /* FIXME: do a function that checks, if filter contains mipmaps, and then replace */
   nparams.minFilter = GLHCK_FILTER_NEAREST;
   glhckTextureParameter(texture->texture, &nparams);

   for (t = object->tcache; t && t->next; t = t->next);
   if (!t) object->tcache = texture;
   else t->next = texture;

   /* store normalized line height */
   fh = ascent - descent;
   font->ascender  = (float)ascent/fh;
   font->descender = (float)descent/fh;
   font->lineh     = (float)(fh + line_gap)/fh;

   font->id       = id;
   font->type     = GLHCK_FONT_BMP;
   font->next     = object->fcache;
   object->fcache = font;

   RET(0, "%d", id);
   return id;

fail:
   if (texture) {
      IFDO(glhckTextureFree, texture->texture);
   }
   IFDO(_glhckFree, texture);
   IFDO(_glhckFree, font);
   RET(0, "%d", 0);
   return 0;
}

/* \brief add new glyph to bitmap font */
GLHCKAPI void glhckTextNewGlyph(glhckText *object,
      unsigned int font_id, const char *s,
      short size, short base, int x, int y, int w, int h,
      float xoff, float yoff, float xadvance)
{
   unsigned int codepoint, state = 0, hh;
   __GLHCKtextFont *font;
   __GLHCKtextGlyph *glyph;
   CALL(0, "%p, %s, %d, %d, %d, %d, %d, %d, %f, %f, %f",
         object, s, size, base, w, y, w, h, xoff, yoff, xadvance);
   assert(object && s);

   /* search font */
   for (font = object->fcache; font && font->id != font_id; font = font->next);

   /* not found */
   if (!font || font->type == GLHCK_FONT_BMP)
      return;

   /* decode utf8 character */
   for (; *s; ++s)
      if (!decutf8(&state, &codepoint, *(unsigned char*)s)) break;
   if (state != UTF8_ACCEPT) return;

   /* allocate space for new glyph */
   glyph = _glhckRealloc(font->gcache,
         font->numGlyph, font->numGlyph+1, sizeof(__GLHCKtextGlyph));
   if (!glyph) return;
   font->gcache = glyph;

   /* init glyph */
   glyph = &font->gcache[font->numGlyph++];
   memset(glyph, 0, sizeof(__GLHCKtextGlyph));
   glyph->code = codepoint;
   glyph->size = size;
   glyph->x1 = x;   glyph->y1 = y;
   glyph->x2 = x+w; glyph->y2 = y+h;
   glyph->xoff = xoff;
   glyph->yoff = yoff - base;
   glyph->xadv = xadvance;

   /* hash */
   hh = hashint(codepoint) & (GLHCK_TEXT_HASH_SIZE-1);
   glyph->next   = font->lut[hh];
   font->lut[hh] = font->numGlyph;
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
   for (texture = object->tcache; texture;
        texture = texture->next) {
      texture->geometry.vertexCount = 0;
   }
}

/* \brief draw text using font */
GLHCKAPI void glhckTextStash(glhckText *object, unsigned int font_id,
      float size, float x, float y, const char *s, float *dx)
{
   unsigned int i, codepoint, state = 0;
   short isize = (short)size*10.0f;
   int vcount;
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
   CALL(2, "%p, %d, %f, %f, %f, %s, %p", object, font_id, size, x, y, s, dx);
   assert(object && s);
   if (!object->tcache) return;

   /* search font */
   for (font = object->fcache; font && font->id != font_id; font = font->next);

   /* not found */
   if (!font || (font->type != GLHCK_FONT_BMP && !font->data))
      return;

   for (; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s)) continue;
      if (!(glyph = _glhckTextGetGlyph(object, font, codepoint, isize)))
         continue;

      vcount = (texture = glyph->texture)->geometry.vertexCount;
      if ((vcount?vcount+6:4) >= texture->geometry.allocatedCount) {
         if (_glhckTextGeometryAllocateMore(&texture->geometry) != RETURN_OK) {
            DEBUG(GLHCK_DBG_WARNING, "TEXT :: [%p] out of memory!", object);
            continue;
         }
      }

      /* should not ever fail */
      if (_getQuad(object, font, glyph, isize, &x, &y, &q)
            != RETURN_OK)
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

   if (dx) *dx = x + (glyph?glyph->xadv:0);
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
GLHCKAPI glhckTexture* glhckTextRTT(glhckText *object, unsigned int font_id,
      float size, const char *s, const glhckTextureParameters *params)
{
   glhckTexture *texture = NULL;
   glhckFramebuffer *fbo = NULL;
   glhckTextureParameters nparams;
   float linew;
   CALL(2, "%p, %u, %f, %s", object, font_id, size, s);

   if (!(texture = glhckTextureNew(NULL, NULL, params)))
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

   if (!(glhckFramebufferAttachTexture(fbo, texture, GLHCK_COLOR_ATTACHMENT0)))
      goto fail;

   glhckFramebufferRecti(fbo, 0, 0, linew, size);
   glhckFramebufferBegin(fbo);
   glhckRenderClear(GLHCK_COLOR_BUFFER);
   glhckTextRender(object);
   glhckFramebufferEnd(fbo);

   texture->importFlags |= GLHCK_TEXTURE_IMPORT_TEXT;
   glhckFramebufferFree(fbo);

   RET(2, "%p", texture);
   return texture;

fail:
   IFDO(glhckFramebufferFree, fbo);
   IFDO(glhckTextureFree, texture);
   RET(2, "%p", NULL);
   return NULL;
}

/* \brief create plane object from text */
GLHCKAPI glhckObject* glhckTextPlane(glhckText *object, unsigned int font_id,
      float size, const char *s, const glhckTextureParameters *params)
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
