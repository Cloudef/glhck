#include "internal.h"
#include "import/import.h"
#include <assert.h>           /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXTURE

/* TODO:
 * keep track of internal and out formats.
 * this will be useful in future. */

/* ---- TEXTURE CACHE ---- */

/* \brief check if texture is in cache, returns reference if found */
static _glhckTexture* _glhckTextureCacheCheck(const char *file)
{
   _glhckTexture *cache;
   CALL(1, "%s", file);

   if (!file)
      goto nothing;

   for (cache = _GLHCKlibrary.world.tlist; cache; cache = cache->next) {
      if (cache->file && !strcmp(cache->file, file))
         return glhckTextureRef(cache);
   }

nothing:
   RET(1, "%p", NULL);
   return NULL;
}

/* \brief set texture data.
 * NOTE: internal function, so data isn't copied. */
void _glhckTextureSetData(_glhckTexture *texture, unsigned char *data)
{
   CALL(1, "%p, %p", texture, data);
   assert(texture);

   IFDO(_glhckFree, texture->data);
   texture->data = data;
}

/* \brief returns number of channels needed for texture format */
inline unsigned int _glhckNumChannels(unsigned int format)
{
   return format==GLHCK_LUMINANCE_ALPHA?2:
          format==GLHCK_RGB?3:
          format==GLHCK_RGBA?4:1;
}

/* ---- PUBLIC API ---- */

/* \brief Allocate texture
 * Takes filename as argument, pass NULL to use user data */
GLHCKAPI _glhckTexture* glhckTextureNew(const char *file, unsigned int flags)
{
   _glhckTexture *texture;
   CALL(0, "%s, %u", file, flags);

   /* check if texture is in cache */
   if ((texture = _glhckTextureCacheCheck(file)))
      goto success;

   /* allocate texture */
   if (!(texture = (_glhckTexture*)_glhckMalloc(sizeof(_glhckTexture))))
      goto fail;

   /* init texture */
   memset(texture, 0, sizeof(_glhckTexture));
   texture->refCounter++;

   /* If file is passed, then try import it */
   if (file) {
      /* copy filename */
      if (!(texture->file = _glhckStrdup(file)))
         goto fail;

      /* default flags if not any specified */
      if ((flags & GLHCK_TEXTURE_DEFAULTS))
         flags += GLHCK_TEXTURE_POWER_OF_TWO        |
                  GLHCK_TEXTURE_MIPMAPS             |
                  GLHCK_TEXTURE_NTSC_SAFE_RGB       |
                  GLHCK_TEXTURE_INVERT_Y            |
                  GLHCK_TEXTURE_DXT;

      /* import image */
      if (_glhckImportImage(texture, file, flags) != RETURN_OK)
         goto fail;
   }

   /* insert to world */
   _glhckWorldInsert(tlist, texture, _glhckTexture*);

success:
   RET(0, "%p", texture);
   return texture;

fail:
   IFDO(glhckTextureFree, texture);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief copy texture */
GLHCKAPI _glhckTexture* glhckTextureCopy(_glhckTexture *src)
{
   _glhckTexture *texture;
   CALL(0, "%p", src);
   assert(src);

   /* allocate texture */
   if (!(texture = (_glhckTexture*)_glhckMalloc(sizeof(_glhckTexture))))
      goto fail;

   /* copy */
   _GLHCKlibrary.render.api.generateTextures(1, &texture->object);

   if (src->file)
      texture->file = _glhckStrdup(src->file);

   glhckTextureCreate(texture, src->data, src->width, src->height, src->format, src->flags);
   DEBUG(GLHCK_DBG_CRAP, "COPY %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   /* set ref counter to 1 */
   texture->refCounter = 1;

   /* insert to world */
   _glhckWorldInsert(tlist, texture, _glhckTexture*);

   /* Return texture */
   RET(0, "%p", texture);
   return texture;

fail:
   IFDO(glhckTextureFree, texture);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference texture */
GLHCKAPI _glhckTexture* glhckTextureRef(_glhckTexture *texture)
{
   CALL(3, "%p", texture);
   assert(texture);

   /* increase ref counter */
   texture->refCounter++;

   /* return reference */
   RET(3, "%p", texture);
   return texture;
}

/* \brief free texture */
GLHCKAPI short glhckTextureFree(_glhckTexture *texture)
{
   CALL(FREE_CALL_PRIO(texture), "%p", texture);
   assert(texture);

   /* not initialized */
   if (!_glhckInitialized) return 0;

   /* there is still references to this texture alive */
   if (--texture->refCounter != 0) goto success;

   DEBUG(GLHCK_DBG_CRAP, "FREE(%p) %dx%d %.2f MiB", texture,
         texture->width, texture->height, (float)texture->size / 1048576);

   /* delete texture if there is one */
   if (texture->object)
      _GLHCKlibrary.render.api.deleteTextures(1, &texture->object);

   /* free */
   IFDO(_glhckFree, texture->file);
   _glhckTextureSetData(texture, NULL);

   /* remove from world */
   _glhckWorldRemove(tlist, texture, _glhckTexture*);

   /* free */
   NULLDO(_glhckFree, texture);

success:
   RET(FREE_RET_PRIO(texture), "%d", texture?texture->refCounter:0);
   return texture?texture->refCounter:0;
}

/* \brief create texture manually. */
GLHCKAPI int glhckTextureCreate(_glhckTexture *texture, unsigned char *data,
      int width, int height, unsigned int format, unsigned int flags)
{
   unsigned int object;
   CALL(0, "%p, %u, %d, %d, %d, %u", texture, data,
         width, height, format, flags);
   assert(texture);

   /* create texture */
   object = _GLHCKlibrary.render.api.createTexture(
         data, width, height, format,
         texture->object?texture->object:0,
         flags);

   if (!object)
      goto fail;

   /* remove old texture data */
   _glhckTextureSetData(texture, NULL);

   texture->object   = object;
   texture->width    = width;
   texture->height   = height;
   texture->format   = format;
   texture->size     = width * height * _glhckNumChannels(format);

   if (data) {
      if (!(texture->data = _glhckCopy(data, texture->size)))
         goto fail;
   }

   DEBUG(GLHCK_DBG_CRAP, "NEW(%p) %dx%d %.2f MiB", texture,
         texture->width, texture->height, (float)texture->size / 1048576);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief save texture to file in TGA format */
GLHCKAPI int glhckTextureSave(_glhckTexture *texture, const char *path)
{
   CALL(0, "%p, %s", texture, path);
   assert(texture);

   /* not correct texture format for saving */
   if (texture->format != GLHCK_RGBA)
      goto format_fail;

   DEBUG(GLHCK_DBG_CRAP,
         "\2Save \3%d\5x\3%d \5[\4%s, \3%d\5]",
         texture->width, texture->height,
         texture->format==GLHCK_RGBA?"RGBA":
         texture->format==GLHCK_RGB?"RGB":
         texture->format==GLHCK_LUMINANCE_ALPHA?
         "LUMINANCE ALPHA":"LUMINANCE",
         _glhckNumChannels(texture->format));

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

format_fail:
   _glhckPuts("\1Err.. Sorry only RGBA texture save for now.");
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief bind texture */
GLHCKAPI void glhckTextureBind(_glhckTexture *texture)
{
   CALL(2, "%p", texture);

   if (!texture && _GLHCKlibrary.render.draw.texture) {
      _GLHCKlibrary.render.api.bindTexture(0);
      _GLHCKlibrary.render.draw.texture = 0;
      return;
   }

   if (_GLHCKlibrary.render.draw.texture == texture->object)
      return;

   _GLHCKlibrary.render.api.bindTexture(texture->object);
   _GLHCKlibrary.render.draw.texture = texture->object;
}

/* \brief bind using ID */
GLHCKAPI void glhckBindTexture(unsigned int texture)
{
   CALL(2, "%d", texture);

   if (_GLHCKlibrary.render.draw.texture == texture)
      return;

   _GLHCKlibrary.render.api.bindTexture(texture);
   _GLHCKlibrary.render.draw.texture = texture;
}

/* vim: set ts=8 sw=3 tw=0 :*/
