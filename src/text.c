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

#define GLHCK_TEXT_HASH_SIZE  256

#define GLHCK_FONT_TTF      1
#define GLHCK_FONT_TTF_MEM  2
#define GLHCK_FONT_BMP      3

/* \brief quad helper struct */
typedef struct __GLHCKtextQuad {
   glhckVector2s v1, v2;
   glhckVector2s t1, t2;
} __GLHCKtextQuad;

/* \brief glyph object */
typedef struct __GLHCKtextGlyph
{
   unsigned int code;
   short size;
   int x1, y1, x2, y2;
   float xadv, xoff, yoff;
   struct __GLHCKtextTexture *texture;
   int next;
} __GLHCKtextGlyph;

/* \brief font object */
typedef struct __GLHCKtextFont
{
   int lut[GLHCK_TEXT_HASH_SIZE];
   unsigned int id, type, numGlyph;
   struct stbtt_fontinfo font;
   unsigned char *data;
   struct __GLHCKtextGlyph *gcache;
   float ascender, descender, lineh;
   struct __GLHCKtextFont *next;
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

/* \brief creates new texture to the cache */
static int _glhckTextNewTexture(_glhckText *text, unsigned int object, size_t size, unsigned int format)
{
   __GLHCKtextTexture *texture, *t;
   CALL(0, "%p, %d", text, object);
   assert(text);

   if (!(texture = _glhckMalloc(sizeof(__GLHCKtextTexture))))
      goto fail;
   memset(texture, 0, sizeof(__GLHCKtextTexture));
   memset(&texture->geometry, 0, sizeof(__GLHCKtextGeometry));

   /* assign */
   for (t = text->tcache; t && t->next; t = t->next);
   if (!t) goto fail;
   t->next = texture;

   texture->object = object;
   texture->size   = size;
   texture->format = format;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(_glhckFree, texture);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief get glyph from font */
__GLHCKtextGlyph* _glhckTextGetGlyph(_glhckText *text, __GLHCKtextFont *font,
      unsigned int code, short isize)
{
   unsigned int h, tex_object;
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
   if (gw >= text->tw || gh >= text->th)
      return NULL;

   br = NULL;
   rh = (gh+7) & ~7;
   texture = text->tcache;
   while (!br) {
      for (i = 0; i < texture->numRows; ++i) {
         if (texture->rows[i].h == rh &&
             texture->rows[i].x+gw+1 <= text->tw)
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
         if (py+rh > text->th) {
            /* go to next texture, if this was used */
            if (texture->next) {
               texture = texture->next;
               continue;
            }

            unsigned int format = GLHCK_ALPHA;
            size_t size         = text->tw * text->th;

            /* as last resort create new texture, if this was used */
            tex_object = _GLHCKlibrary.render.api.createTexture(NULL, size,
                  text->tw, text->th, format, 0);
            if (!tex_object)
               return NULL;

            if (_glhckTextNewTexture(text, tex_object, size, format) != RETURN_OK)
               return NULL;

            /* cycle and hope for best */
            texture = texture->next;
            continue;
         }
      }

      /* add new row */
      br = &texture->rows[texture->numRows];
      br->x = 0; br->y = py; br->h = rh;
      texture->numRows++;
   }

   /* create new glyph */
   font->gcache = _glhckRealloc(font->gcache,
         font->numGlyph, font->numGlyph+1, sizeof(__GLHCKtextGlyph));
   if (!font->gcache)
      return NULL;

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
      _GLHCKlibrary.render.api.fillTexture(texture->object, data, 0, glyph->x1, glyph->y1, gw, gh, texture->format);
      _glhckFree(data);
   }

   return glyph;
}

/* \brief get quad for text drawing */
static int _getQuad(_glhckText *text, __GLHCKtextFont *font, __GLHCKtextGlyph *glyph,
      short isize, float *x, float *y, __GLHCKtextQuad *q)
{
   glhckVector2f v1, v2, t1, t2;
   int rx, ry;
   float scale = 1.0f;

   if (font->type == GLHCK_FONT_BMP) scale = (float)isize/(glyph->size*10.0f);

   rx = floorf(*x + scale * glyph->xoff);
   ry = floorf(*y + scale * glyph->yoff);

   v2.x = rx;
   v1.y = ry;
   v1.x = rx + scale * (glyph->x2 - glyph->x1);
   v2.y = ry + scale * (glyph->y2 - glyph->y1);

   t2.x = (glyph->x1) * text->itw;
   t1.y = (glyph->y1) * text->ith;
   t1.x = (glyph->x2) * text->itw;
   t2.y = (glyph->y2) * text->ith;

#if GLHCK_PRECISION_VERTEX == GLHCK_FLOAT
   memcpy(&q->v1, &v1, sizeof(_glhckVertex2d));
   memcpy(&q->v2, &v2, sizeof(_glhckVertex2d));
#else
   glhckSetV2(&q->v1, &v1);
   glhckSetV2(&q->v2, &v2);
#endif

#if GLHCK_PRECISION_COORD == GLHCK_FLOAT
   memcpy(&q->t1, &t1, sizeof(_glhckCoord2d));
   memcpy(&q->t2, &t2, sizeof(_glhckCoord2d));
#else
   q->t1.x = t1.x * text->textureRange;
   q->t1.y = t1.y * text->textureRange;
   q->t2.x = t2.x * text->textureRange;
   q->t2.y = t2.y * text->textureRange;
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
   _glhckText *text;
   __GLHCKtextTexture *texture = NULL;
   CALL(0, "%d, %d", cachew, cacheh);

   /* allocate text stack */
   if (!(text = _glhckMalloc(sizeof(_glhckText))))
      goto fail;
   memset(text, 0, sizeof(_glhckText));

   /* allocate texture */
   if (!(texture = _glhckMalloc(sizeof(__GLHCKtextTexture))))
      goto fail;

   memset(texture, 0, sizeof(__GLHCKtextTexture));
   memset(&texture->geometry, 0, sizeof(__GLHCKtextGeometry));

   texture->format = GLHCK_ALPHA;
   texture->size   = cachew * cacheh;

   /* fill texture with emptiness */
   texture->object = _GLHCKlibrary.render.api.createTexture(NULL, texture->size,
         cachew, cacheh, texture->format, 0);
   if (!texture->object)
      goto fail;

   text->tw  = cachew;
   text->th  = cacheh;
   text->itw = (float)1.0f/cachew;
   text->ith = (float)1.0f/cacheh;
   text->tcache = texture;
   text->textureRange = SHRT_MAX;

   /* default color */
   glhckTextColor(text, 255, 255, 255, 255);

   /* insert to world */
   _glhckWorldInsert(tflist, text, _glhckText*);

   RET(0, "%p", text);
   return text;

fail:
   IFDO(_glhckFree, text);
   IFDO(_glhckFree, texture);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief free text stack */
GLHCKAPI short glhckTextFree(glhckText *text)
{
   __GLHCKtextFont *f, *fn;
   __GLHCKtextTexture *t, *tn;
   CALL(0, "%p", text);
   assert(text);

   /* not initialized */
   if (!_glhckInitialized) return 0;

   /* free texture cache */
   for (t = text->tcache; t; t = tn) {
      tn = t->next;
      if (t->object)
         _GLHCKlibrary.render.api.deleteTextures(1,
               &t->object);
      _glhckFree(t);
   }

   /* free font cache */
   for (f = text->fcache; f; f = fn) {
      fn = f->next;
      IFDO(_glhckFree, f->gcache);
      IFDO(_glhckFree, f->data);
      _glhckFree(f);
   }

   /* remove text */
   _glhckWorldRemove(tflist, text, _glhckText*);

   /* free this */
   _glhckFree(text);

   /* has no reference counting at least atm */
   return 0;
}

/* \brief get text metrics */
GLHCKAPI void glhckTextGetMetrics(glhckText *text, unsigned int font_id,
      float size, float *ascender, float *descender, float *lineh)
{
   __GLHCKtextFont *font;
   CALL(1, "%p, %d, %zu, %f, %f, %f", text, font_id, size,
         ascender, descender, lineh);
   assert(text);

   /* search font */
   for (font = text->fcache; font && font->id != font_id;
        font = font->next);

   /* not found */
   if (!font)
      return;

   /* must not fail */
   assert(font->type != GLHCK_FONT_BMP && !font->data);
   if (ascender)  *ascender  = font->ascender*size;
   if (descender) *descender = font->descender*size;
   if (lineh)     *lineh     = font->lineh*size;
}

/* \brief get min max of text */
GLHCKAPI void glhckTextGetMinMax(glhckText *text, int font_id, float size,
      const char *s, kmVec2 *min, kmVec2 *max)
{
   unsigned int codepoint, state = 0;
   short isize = (short)size*10.0f;
   __GLHCKtextFont *font;
   __GLHCKtextGlyph *glyph;
   __GLHCKtextQuad q;
   float x, y;

   CALL(1, "%p, %d, %zu, %s, %p, %p",
         text, font_id, size, s, min, max);
   assert(text && s && min && max);

   /* search font */
   for (font = text->fcache; font && font->id != font_id;
        font = font->next);

   /* not found */
   if (!font || (font->type != GLHCK_FONT_BMP && !font->data))
      return;

   min->x = max->x = x = 0;
   min->y = max->y = y = 0;

   for (; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s)) continue;
      if (!(glyph = _glhckTextGetGlyph(text, font, codepoint, isize)))
         continue;
      if (_getQuad(text, font, glyph, isize, &x, &y, &q) != RETURN_OK)
         continue;

      glhckMinV2(min, &q.v1);
      glhckMaxV2(max, &q.v1);
      glhckMinV2(min, &q.v2);
      glhckMaxV2(max, &q.v2);
   }
}

/* \brief set text color */
GLHCKAPI void glhckTextColor(glhckText *text,
      unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   CALL(1 ,"%p, %d, %d, %d", text, r, g, b, a);
   assert(text);

   text->color.r = r;
   text->color.g = g;
   text->color.b = b;
   text->color.a = a;
}

/* \brief get text color */
GLHCKAPI const glhckColorb* glhckTextGetColor(glhckText *text)
{
   CALL(1, "%p", text);
   assert(text);

   RET(1, "%p", &text->color);
   return &text->color;
}

/* \brief new font from memory */
GLHCKAPI unsigned int glhckTextNewFontFromMemory(glhckText *text, unsigned char *data)
{
   unsigned int id;
   int ascent, descent, fh, line_gap;
   __GLHCKtextFont *font, *f;
   CALL(0, "%p, %p", text, data);
   assert(text && data);

   /* allocate font */
   if (!(font = _glhckMalloc(sizeof(__GLHCKtextFont))))
      goto fail;

   /* init */
   memset(font, 0, sizeof(__GLHCKtextFont));
   memset(font->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   for (id = 1, f = text->fcache; f; f = f->next) ++id;
   font->data = data;

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
   font->next     = text->fcache;
   text->fcache   = font;

   RET(0, "%d", id);
   return id;

fail:
   IFDO(_glhckFree, font);
   RET(0, "%d", 0);
   return 0;
}

/* \brief new truetype font */
GLHCKAPI unsigned int glhckTextNewFont(glhckText *text, const char *file)
{
   FILE *f;
   size_t size;
   unsigned int id;
   unsigned char *data = NULL;
   CALL(0, "%p, %s", text, file);
   assert(text && file);

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
   if (!(id = glhckTextNewFontFromMemory(text, data)))
      goto fail;

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
GLHCKAPI unsigned int glhckTextNewFontFromBitmap(glhckText *text,
      const char *file, int ascent, int descent, int line_gap)
{
   int fh;
   _glhckTexture *temp = NULL;
   unsigned int id, texture;
   unsigned char *data = NULL;
   __GLHCKtextFont *font, *f;
   CALL(0, "%p, %s, %d, %d, %d",
         text, file, ascent, descent, line_gap);
   assert(text && file);

    /* allocate font */
   if (!(font = _glhckMalloc(sizeof(__GLHCKtextFont))))
      goto fail;

   /* init */
   memset(font, 0, sizeof(__GLHCKtextFont));
   memset(font->lut, -1, GLHCK_TEXT_HASH_SIZE * sizeof(int));
   for (id = 1, f = text->fcache; f; f = f->next) ++id;

   /* load image */
   if (!(temp = glhckTextureNew(file, GLHCK_TEXTURE_DEFAULTS)))
      goto fail;

   /* get data from texture */
   if (!(data = _glhckCopy(temp->data, temp->size)))
      goto fail;

   /* create texture */
   if (!(texture = _GLHCKlibrary.render.api.createTexture(
               data, temp->size, temp->width, temp->height, temp->format, 0)))
      goto fail;

   /* not needed anymore */
   NULLDO(glhckTextureFree, temp);

   if (_glhckTextNewTexture(text, texture, temp->size, temp->format) != RETURN_OK)
      goto fail;

   /* store normalized line height */
   fh = ascent - descent;
   font->ascender  = (float)ascent/fh;
   font->descender = (float)descent/fh;
   font->lineh     = (float)(fh + line_gap)/fh;

   font->id       = id;
   font->type     = GLHCK_FONT_BMP;
   font->next     = text->fcache;
   text->fcache   = font;

   RET(0, "%d", id);
   return id;

fail:
   IFDO(glhckTextureFree, temp);
   IFDO(_glhckFree, font);
   IFDO(free, data);
   RET(0, "%d", 0);
   return 0;
}

/* \brief add new glyph to bitmap font */
GLHCKAPI void glhckTextNewGlyph(glhckText *text,
      unsigned int font_id, const char *s,
      short size, short base, int x, int y, int w, int h,
      float xoff, float yoff, float xadvance)
{
   unsigned int codepoint, state = 0, hh;
   __GLHCKtextFont *font;
   __GLHCKtextGlyph *glyph;
   CALL(0, "%p, %s, %d, %d, %d, %d, %d, %d, %f, %f, %f",
         text, s, size, base, w, y, w, h, xoff, yoff, xadvance);
   assert(text && s);

   /* search font */
   for (font = text->fcache; font && font->id != font_id;
        font = font->next);

   /* not found */
   if (!font || font->type == GLHCK_FONT_BMP)
      return;

   /* decode utf8 character */
   for (; *s; ++s)
      if (!decutf8(&state, &codepoint, *(unsigned char*)s)) break;
   if (state != UTF8_ACCEPT) return;

   /* allocate space for new glyph */
   font->gcache = _glhckRealloc(font->gcache,
         font->numGlyph, font->numGlyph+1, sizeof(__GLHCKtextGlyph));
   if (!font->gcache) return;

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
GLHCKAPI void glhckTextRender(glhckText *text)
{
   __GLHCKtextTexture *texture;
   CALL(2, "%p", text); assert(text);
   _GLHCKlibrary.render.api.textDraw(text);

   /* reset vertex counts */
   for (texture = text->tcache; texture;
        texture = texture->next) {
      texture->geometry.vertexCount = 0;
   }
}

/* \brief draw text using font */
GLHCKAPI void glhckTextDraw(glhckText *text, unsigned int font_id,
      float size, float x, float y, const char *s, float *dx)
{
   unsigned int i, codepoint, state = 0;
   short isize = (short)size*10.0f;
   size_t vcount;
   glhckVertexData2s *v, *d;
   __GLHCKtextTexture *texture;
   __GLHCKtextGlyph *glyph;
   __GLHCKtextFont *font;
   __GLHCKtextQuad q;
   CALL(2, "%p, %d, %zu, %f, %f, %s, %p", text, font_id,
         size, x, y, s, dx);
   assert(text && s);
   if (!text->tcache) return;

   /* search font */
   for (font = text->fcache; font && font->id != font_id;
        font = font->next);

   /* not found */
   if (!font || (font->type != GLHCK_FONT_BMP && !font->data))
      return;

   for (; *s; ++s) {
      if (decutf8(&state, &codepoint, *(unsigned char*)s)) continue;
      if (!(glyph = _glhckTextGetGlyph(text, font, codepoint, isize)))
         continue;

      vcount = (texture = glyph->texture)->geometry.vertexCount;
      if ((vcount?vcount+6:4) >= GLHCK_TEXT_VERT_COUNT)
         continue;

      /* should not ever fail */
      if (_getQuad(text, font, glyph, isize, &x, &y, &q)
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

   if (dx) *dx = x;
}

/* vim: set ts=8 sw=3 tw=0 :*/
