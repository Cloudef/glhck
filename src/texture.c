#include "internal.h"
#include "import/import.h"
#include "imghck.h"
#include <stdio.h>            /* for file io */
#include <assert.h>           /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXTURE

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

/* \brief assign texture to draw list */
inline void _glhckTextureInsertToQueue(_glhckTexture *texture)
{
   __GLHCKtextureQueue *textures;
   unsigned int i;

   textures = &GLHCKRD()->textures;

   /* check duplicate */
   for (i = 0; i != textures->count; ++i)
      if (textures->queue[i] == texture) return;

   /* need alloc dynamically more? */
   if (textures->allocated <= textures->count+1) {
      textures->queue = _glhckRealloc(textures->queue,
            textures->allocated,
            textures->allocated + GLHCK_QUEUE_ALLOC_STEP,
            sizeof(_glhckTexture*));

      /* epic fail here */
      if (!textures->queue) return;
      textures->allocated += GLHCK_QUEUE_ALLOC_STEP;
   }

   /* assign the texture to list */
   textures->queue[textures->count] = glhckTextureRef(texture);
   textures->count++;
}

/* \brief set texture data.
 * NOTE: internal function, so data isn't copied. */
static void _glhckTextureSetData(_glhckTexture *texture, void *data)
{
   CALL(1, "%p, %p", texture, data);
   assert(texture);

   IFDO(_glhckFree, texture->data);
   texture->data = data;
}

/* \brief returns number of channels needed for texture format */
inline unsigned int _glhckNumChannels(glhckTextureFormat format)
{
   return format==GLHCK_LUMINANCE_ALPHA?2:
          format==GLHCK_RGB?3:
          format==GLHCK_RGBA?4:1;
}

/* \brief returns if format is compressed format */
inline int _glhckIsCompressedFormat(glhckTextureFormat format)
{
   return (format==GLHCK_COMPRESSED_RGB_DXT1 ||
           format==GLHCK_COMPRESSED_RGBA_DXT5);
}

/* \brief assign default flags */
static void _glhckTextureDefaultFlags(unsigned int *flags)
{
   *flags |= GLHCK_TEXTURE_DXT;
}

/* \brief compress image data internally */
static void* _glhckTextureCompress(
      const void *data, int width, int height,
      glhckTextureFormat format, unsigned int flags,
      glhckTextureFormat *outFormat)
{
   unsigned char *compressed = NULL;
   CALL(0, "%p, %u, %u, %u, %u, %p",
         data, width, height, format, flags, outFormat);
   assert(outFormat);

   /* NULL data, just return */
   if (!data) goto fail;

   /* check if already compressed */
   if (_glhckIsCompressedFormat(format))
      goto already_compressed;

   /* get default flags */
   if (flags & GLHCK_TEXTURE_DEFAULTS)
      _glhckTextureDefaultFlags(&flags);

   /* check that flags have compression */
   if (!(flags & GLHCK_TEXTURE_DXT))
      goto fail;

   /* DXT compression requested */
   if (flags & GLHCK_TEXTURE_DXT) {
      if ((_glhckNumChannels(format) & 1) == 1) {
         /* RGB */
         if (!(compressed = _glhckMalloc(imghckSizeForDXT1(width, height))))
            goto out_of_memory;

         imghckConvertToDXT1(compressed, data, width, height,
               _glhckNumChannels(format));
         *outFormat = GLHCK_COMPRESSED_RGB_DXT1;
         DEBUG(GLHCK_DBG_CRAP, "Image data converted to DXT1");
      } else {
         /* RGBA */
         if (!(compressed = _glhckMalloc(imghckSizeForDXT5(width, height))))
            goto out_of_memory;

         imghckConvertToDXT5(compressed, data, width, height,
               _glhckNumChannels(format));
         *outFormat = GLHCK_COMPRESSED_RGBA_DXT5;
         DEBUG(GLHCK_DBG_CRAP, "Image data converted to DXT5");
      }
   }

   RET(0, "%p", compressed);
   return compressed;

already_compressed:
   DEBUG(GLHCK_DBG_WARNING, "Image data is already compressed");
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory");
fail:
   IFDO(_glhckFree, compressed);
   RET(0, "%p", NULL);
   return NULL;
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
   if (!(texture = (_glhckTexture*)_glhckCalloc(1, sizeof(_glhckTexture))))
      goto fail;

   texture->refCounter++;

   /* If file is passed, then try import it */
   if (file) {
      /* copy filename */
      if (!(texture->file = _glhckStrdup(file)))
         goto fail;

      /* default flags if not any specified */
      if (flags & GLHCK_TEXTURE_DEFAULTS)
         _glhckTextureDefaultFlags(&flags);

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
   GLHCKRA()->textureGenerate(1, &texture->object);

   if (src->file)
      texture->file = _glhckStrdup(src->file);

   glhckTextureCreate(texture, src->data, src->width, src->height, src->format, src->outFormat, src->flags);
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
GLHCKAPI size_t glhckTextureFree(_glhckTexture *texture)
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
      GLHCKRA()->textureDelete(1, &texture->object);

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
GLHCKAPI int glhckTextureCreate(_glhckTexture *texture, const void *data,
      int width, int height, glhckTextureFormat format, glhckTextureFormat outFormat, unsigned int flags)
{
   size_t size;
   unsigned int object;
   unsigned char *compressed = NULL;
   CALL(0, "%p, %p, %d, %d, %u, %u, %u", texture, data,
         width, height, format, outFormat, flags);
   assert(texture);

   /* get default flags */
   if (flags & GLHCK_TEXTURE_DEFAULTS)
      _glhckTextureDefaultFlags(&flags);

   /* will do compression if requested */
   if ((compressed = _glhckTextureCompress(data,
               width, height, outFormat, flags, &outFormat))) {
      data = compressed;
   }

   /* check the true data size */
   if (outFormat == GLHCK_COMPRESSED_RGB_DXT1)
      size = imghckSizeForDXT1(width, height);
   else if (outFormat == GLHCK_COMPRESSED_RGBA_DXT5)
      size = imghckSizeForDXT5(width, height);
   else
      size = width * height * _glhckNumChannels(format);

   /* create texture */
   object = GLHCKRA()->textureCreate(
         data, size, width, height, outFormat,
         texture->object?texture->object:0, flags);

   if (!object)
      goto fail;

   /* remove old texture data */
   _glhckTextureSetData(texture, NULL);

   /* if size is zero, calculate the size here */
   texture->object    = object;
   texture->width     = width;
   texture->height    = height;
   texture->format    = format;
   texture->outFormat = outFormat;
   texture->size      = size;
   texture->flags     = flags;

   /* copy data */
   if (data) {
      if (!(texture->data = _glhckCopy(data, texture->size)))
         goto fail;
   }

   DEBUG(GLHCK_DBG_CRAP, "NEW(%p) %dx%d %.2f MiB", texture,
         texture->width, texture->height, (float)texture->size / 1048576);

   IFDO(_glhckFree, compressed);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(_glhckFree, compressed);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief recreate texture with new data.
 * dimensions of texture must be the same. */
GLHCKAPI int glhckTextureRecreate(glhckTexture *texture, const void *data, glhckTextureFormat format)
{
   return glhckTextureCreate(texture, data,
         texture->width, texture->height,
         format, format, texture->flags);
}

/* \brief save texture to file in TGA format */
GLHCKAPI int glhckTextureSave(_glhckTexture *texture, const char *path)
{
   FILE *f = NULL;
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

#if 0
   /* open for read */
   if (!(f = fopen(path, "wb")))
      goto fail;

   /* dump raw data */
   fwrite(texture->data, 1, texture->size, f);
   NULLDO(fclose, f);
#endif

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

format_fail:
   _glhckPuts("\1Err.. Sorry only RGBA texture save for now.");
//fail:
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief bind texture */
GLHCKAPI void glhckTextureBind(_glhckTexture *texture)
{
   CALL(2, "%p", texture);
   if (GLHCKRD()->texture == texture) return;
   GLHCKRA()->textureBind(texture?texture->object:0);
   GLHCKRD()->texture = texture;
}

/* vim: set ts=8 sw=3 tw=0 :*/
