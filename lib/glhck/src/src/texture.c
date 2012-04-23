#include "internal.h"
#include "import/import.h"
#include "../include/SOIL.h"  /* for image saving */
#include <assert.h>           /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXTURE

/* ---- TEXTURE CACHE ---- */

/* \brief check if texture is in cache, returns reference if found */
static _glhckTexture* _glhckTextureCacheCheck(const char *file)
{
   __GLHCKtextureCache *cache;
   CALL("%s", file);

   if (!file)
      goto nothing;

   for (cache = _GLHCKlibrary.texture.cache; cache; cache = cache->next) {
      if (!strcmp(cache->texture->file, file))
         return glhckTextureRef(cache->texture);
   }

nothing:
   RET("%p", NULL);
   return NULL;
}

/* \brief insert texture to cache */
static int _glhckTextureCacheInsert(_glhckTexture *texture)
{
   __GLHCKtextureCache *cache;
   CALL("%p", texture);
   assert(texture);

   if (!texture->file)
      goto fail;

   cache = _GLHCKlibrary.texture.cache;
   if (!cache) {
      cache = _GLHCKlibrary.texture.cache =
         _glhckMalloc(sizeof(__GLHCKtextureCache));
   } else {
      for (; cache && cache->next; cache = cache->next);
      cache = cache->next =
         _glhckMalloc(sizeof(__GLHCKtextureCache));
   }

   if (!cache)
      goto fail;

   /* init cache */
   memset(cache, 0, sizeof(__GLHCKtextureCache));
   cache->texture = texture;

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief remove texture from cache */
static int _glhckTextureCacheRemove(_glhckTexture *texture)
{
   __GLHCKtextureCache *cache, *found;
   CALL("%p", texture);
   assert(texture);

   if (!(cache = _GLHCKlibrary.texture.cache))
      goto _return;

   if (cache->texture == texture) {
      _GLHCKlibrary.texture.cache = cache->next;
      _glhckFree(cache);
   } else {
      for (; cache && cache->next &&
             cache->next->texture != texture;
             cache = cache->next);
      if ((found = cache->next)) {
         cache->next = found->next;
         _glhckFree(found);
      }
   }

_return:
   RET("%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief release texture cache */
void _glhckTextureCacheRelease(void)
{
   __GLHCKtextureCache *cache, *next;
   TRACE();

   if (!(cache = _GLHCKlibrary.texture.cache))
      return;

   for (; cache; cache = next) {
      next = cache->next;
      _glhckFree(cache);
   }

   _GLHCKlibrary.texture.cache = NULL;
}

/* \brief set texture data.
 * NOTE: internal function, so data isn't copied. */
void _glhckTextureSetData(_glhckTexture *texture, unsigned char *data)
{
   CALL("%p, %p", texture, data);
   assert(texture);

   if (texture->data) _glhckFree(texture->data);
   texture->data = data;
}

/* ---- PUBLIC API ---- */

/* \brief Allocate texture
 * Takes filename as argument, pass NULL to use user data */
GLHCKAPI _glhckTexture* glhckTextureNew(const char *file, unsigned int flags)
{
   _glhckTexture *texture;
   CALL("%s, %u", file, flags);

   /* check if texture is in cache */
   if ((texture = _glhckTextureCacheCheck(file)))
      goto success;

   /* allocate texture */
   if (!(texture = (_glhckTexture*)_glhckMalloc(sizeof(_glhckTexture))))
      goto fail;

   /* init texture */
   memset(texture, 0, sizeof(_glhckTexture));

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
      if (_glhckImportImage(texture, file) != RETURN_OK)
         goto fail;

      /* upload texture */
      if (_GLHCKlibrary.render.api.uploadTexture(texture, flags) != RETURN_OK)
         goto fail;

      /* we know the size of image data from SOIL,
       * add it to tracking.
       *
       * Maybe solve this different way later? */
#ifndef NDEBUG
      _glhckTrackFake(texture, sizeof(_glhckTexture) + texture->size);
#endif

      /* insert to cache */
      _glhckTextureCacheInsert(texture);
      DEBUG(GLHCK_DBG_CRAP, "NEW %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);
   }

   /* increase ref counter */
   texture->refCounter++;

success:
   RET("%p", texture);
   return texture;

fail:
   if (texture) glhckTextureFree(texture);
   RET("%p", NULL);
   return NULL;
}

/* \brief copy texture */
GLHCKAPI _glhckTexture* glhckTextureCopy(_glhckTexture *src)
{
   _glhckTexture *texture;
   CALL("%p", src);
   assert(src);

   /* allocate texture */
   if (!(texture = (_glhckTexture*)_glhckMalloc(sizeof(_glhckTexture))))
      goto fail;

   /* copy */
   _GLHCKlibrary.render.api.generateTextures(1, &texture->object);

   if (!(texture->file = _glhckStrdup(src->file)))
      goto fail;

   glhckTextureCreate(texture, src->data, src->width, src->height, src->channels, src->flags);
   DEBUG(GLHCK_DBG_CRAP, "COPY %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   /* set ref counter to 1 */
   texture->refCounter = 1;

   /* Return texture */
   RET("%p", texture);
   return texture;

fail:
   if (texture) glhckTextureFree(texture);
   RET("%p", NULL);
   return NULL;
}

/* \brief reference texture */
GLHCKAPI _glhckTexture* glhckTextureRef(_glhckTexture *texture)
{
   CALL("%p", texture);
   assert(texture);

   DEBUG(GLHCK_DBG_CRAP, "REFERENCE %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   /* increase ref counter */
   texture->refCounter++;

   /* return reference */
   RET("%p", texture);
   return texture;
}

/* \brief free texture */
GLHCKAPI short glhckTextureFree(_glhckTexture *texture)
{
   CALL("%p", texture);
   assert(texture);

   /* there is still references to this texture alive */
   if (--texture->refCounter != 0) goto success;

   DEBUG(GLHCK_DBG_CRAP, "FREE %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   /* remove from cache */
   _glhckTextureCacheRemove(texture);

   /* delete texture if there is one */
   if (texture->object)
      _GLHCKlibrary.render.api.deleteTextures(1, &texture->object);

   _glhckTextureSetData(texture, NULL);
   if (texture->file) _glhckFree(texture->file);

   /* free */
   _glhckFree(texture);
   texture = NULL;

success:
   RET("%d", texture?texture->refCounter:0);
   return texture?texture->refCounter:0;
}

/* \brief create texture manually. */
GLHCKAPI int glhckTextureCreate(_glhckTexture *texture, unsigned char *data,
      int width, int height, int channels, unsigned int flags)
{
   unsigned int object;
   CALL("%p, %u, %d, %d, %d, %u", texture, data,
         width, height, channels, flags);
   assert(texture);

   /* create texture */
   object = _GLHCKlibrary.render.api.createTexture(
         data, width, height, channels,
         texture->object?texture->object:0,
         flags);

   if (!object)
      goto fail;

   /* remove old texture data */
   _glhckTextureSetData(texture, NULL);

   texture->object   = object;
   texture->width    = width;
   texture->height   = height;
   texture->channels = channels;
   texture->size     = width * height * channels;

   if (data) {
      if (!(texture->data = _glhckCopy(data, texture->size)))
         goto fail;
   }

   DEBUG(GLHCK_DBG_CRAP, "NEW %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief save texture to file in TGA format */
GLHCKAPI int glhckTextureSave(_glhckTexture *texture, const char *path)
{
   CALL("%p, %s", texture, path);
   assert(texture);

   if (!SOIL_save_image
      (
          path,
          SOIL_SAVE_TYPE_TGA,
          texture->width, texture->height, texture->channels,
          texture->data
      ))
      goto fail;

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief bind texture */
GLHCKAPI void glhckTextureBind(_glhckTexture *texture)
{
   CALL("%p", texture);

   if (!texture && _GLHCKlibrary.texture.bind) {
      _GLHCKlibrary.render.api.bindTexture(0);
      _GLHCKlibrary.texture.bind = 0;
      return;
   }

   if (_GLHCKlibrary.texture.bind == texture->object)
      return;

   _GLHCKlibrary.render.api.bindTexture(texture->object);
   _GLHCKlibrary.texture.bind = texture->object;
}

/* \brief bind using ID */
GLHCKAPI void glhckBindTexture(unsigned int texture)
{
   CALL("%u", texture);

   if (_GLHCKlibrary.texture.bind == texture)
      return;

   _GLHCKlibrary.render.api.bindTexture(texture);
   _GLHCKlibrary.texture.bind = texture;
}
