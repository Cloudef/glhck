#include "internal.h"
#include "import/import.h"
#include "../include/SOIL.h"  /* for image saving */
#include <assert.h>           /* for assert */

/* ---- TEXTURE CACHE ---- */

/* check if texture is in chache
 * returns reference if found */
static _glhckTexture* _glhckTextureCacheCheck(const char *file)
{
   __GLHCKtextureCache *cache;
   CALL("%s", file);

   if (!file)
      goto nothing;

   for(cache = _GLHCKlibrary.texture.cache; cache; cache = cache->next) {
      if (!strcmp(cache->texture->file, file))
         return glhckTextureRef(cache->texture);
   }

nothing:
   RET("%p", NULL);
   return NULL;
}

/* insert texture to cache */
static int _glhckTextureCacheInsert(_glhckTexture *texture)
{
   __GLHCKtextureCache *cache;
   assert(texture);
   CALL("%p", texture);

   if (!texture->file)
      goto fail;

   cache = _GLHCKlibrary.texture.cache;
   if (!cache) {
      cache = _GLHCKlibrary.texture.cache =
         _glhckMalloc(sizeof(__GLHCKtextureCache));
   } else {
      for (; cache && cache->next; cache->next);
      cache = cache->next =
         _glhckMalloc(sizeof(__GLHCKtextureCache));
   }

   if (!cache)
      goto fail;

   /* init */
   cache->next = NULL;

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* remove texture from cache */
static int _glhckTextureCacheRemove(_glhckTexture *texture)
{
   __GLHCKtextureCache *cache, *found;
   assert(texture);
   CALL("%p", texture);

   if (!(cache = _GLHCKlibrary.texture.cache))
      goto _return;

   if (cache->texture == texture) {
      _GLHCKlibrary.texture.cache = cache->next;
      _glhckFree(cache);
   } else {
      for (; cache && cache->next &&
             cache->next->texture != texture;
             cache = cache->next);
      found = cache->next;
      cache->next = found->next;
      _glhckFree(found);
   }

_return:
   RET("%d", RETURN_OK);
   return RETURN_OK;
}

/* release texture cache */
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

/* ---- PUBLIC API ---- */

/* Allocate texture
 * Takes filename as argument, pass NULL to use user data */
GLHCKAPI _glhckTexture* glhckTextureNew(const char *file, unsigned int flags)
{
   _glhckTexture *texture;
   CALL("%s, %u", file, flags);

   /* check if texture is in cache */
   if ((texture = _glhckTextureCacheCheck(file))) {
      RET("%p", texture);
      return texture;
   }

   /* allocate texture */
   if (!(texture = (_glhckTexture*)_glhckMalloc(sizeof(_glhckTexture))))
      goto fail;

   /* init texture textureect */
   texture->object = 0;
   texture->file   = NULL;
   texture->data   = NULL;

   /* If file is passed, then try import it */
   if (file) {
      /* copy filename */
      if (!(texture->file = strdup(file)))
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

      /* insert to cache */
      _glhckTextureCacheInsert(texture);
      DEBUG(GLHCK_DBG_CRAP, "NEW %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);
   }

   /* increase ref counter */
   texture->refCounter++;

   RET("%p", texture);
   return texture;

fail:
   if (texture) glhckTextureFree(texture);
   RET("%p", NULL);
   return NULL;
}

/* Copy texture */
GLHCKAPI _glhckTexture* glhckTextureCopy(_glhckTexture *src)
{
   _glhckTexture *texture;
   assert(src);
   CALL("%p", src);

   /* allocate texture */
   if (!(texture = (_glhckTexture*)_glhckMalloc(sizeof(_glhckTexture))))
      goto fail;

   /* copy */
   _GLHCKlibrary.render.api.generateTextures(1, &texture->object);

   if (!(texture->file = strdup(src->file)))
      goto fail;

   glhckTextureCreate(texture, src->data, src->width, src->height, src->channels, src->flags);
   if (!(texture->data = _glhckCopy(src->data, src->size)))
      goto fail;

   DEBUG(GLHCK_DBG_CRAP, "COPY %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   /* increase ref counter */
   texture->refCounter++;

   /* Return texture */
   RET("%p", texture);
   return texture;

fail:
   if (texture) glhckTextureFree(texture);
   RET("%p", NULL);
   return NULL;
}

/* Reference texture */
GLHCKAPI _glhckTexture* glhckTextureRef(_glhckTexture *texture)
{
   assert(texture);
   CALL("%p", texture);

   DEBUG(GLHCK_DBG_CRAP, "REFERENCE %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   /* increase ref counter */
   texture->refCounter++;

   /* return reference */
   RET("%p", texture);
   return texture;
}

/* Free texture */
GLHCKAPI int glhckTextureFree(_glhckTexture *texture)
{
   assert(texture);
   CALL("%p", texture);

   /* there is still references to this textureect alive */
   if (--texture->refCounter != 0) { RET("%d", texture->refCounter); return texture->refCounter; }

   DEBUG(GLHCK_DBG_CRAP, "FREE %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   /* remove from cache */
   _glhckTextureCacheRemove(texture);

   /* delete texture if there is one */
   if (texture->object)
      _GLHCKlibrary.render.api.deleteTextures(1, &texture->object);
   if (texture->data)     _glhckFree(texture->data);
   if (texture->file)     _glhckFree(texture->file);

   /* free */
   _glhckFree(texture);

   RET("%d", RETURN_OK);
   return RETURN_OK;
}

/* Create GL Texture manually.
 * Flags are SOIL flags */
GLHCKAPI int glhckTextureCreate(_glhckTexture *texture, unsigned char *data,
      int width, int height, int channels, unsigned int flags)
{
   unsigned int object;
   assert(texture);
   CALL("%p, %u, %d, %d, %d, %u", texture, data,
         width, height, channels, flags);

   /* create texture */
   object = _GLHCKlibrary.render.api.createTexture(
         data, width, height, channels,
         texture->object?texture->object:0,
         flags);

   if (!object)
      goto fail;

   if (texture->data) _glhckFree(texture->data);
   texture->object   = object;
   texture->width    = width;
   texture->height   = height;
   texture->channels = channels;
   texture->data     = data;
   texture->size     = width * height * channels;

   DEBUG(GLHCK_DBG_CRAP, "NEW %dx%d %.2f MiB", texture->width, texture->height, (float)texture->size / 1048576);

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* Save texture to file in TGA format */
GLHCKAPI int glhckTextureSave(_glhckTexture *texture, const char *path)
{
   assert(texture);
   CALL("%p, %s", texture, path);

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

/* bind texture */
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

/* bind using ID */
GLHCKAPI void glhckBindTexture(unsigned int texture)
{
   CALL("%u", texture);

   if (_GLHCKlibrary.texture.bind == texture)
      return;

   _GLHCKlibrary.render.api.bindTexture(texture);
   _GLHCKlibrary.texture.bind = texture;
}
