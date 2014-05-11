#include "internal.h"
#include "import/import.h"
#include "texhck.h"
#include <stdlib.h> /* for malloc */
#include <stdio.h>  /* for file io */
#include <assert.h> /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXTURE

/* ---- TEXTURE CACHE ---- */

/* \brief check if texture is in cache, returns reference if found */
static glhckTexture* _glhckTextureCacheCheck(const char *file)
{
   CALL(1, "%s", file);

   if (!file)
      goto nothing;

   glhckTexture *cache;
   for (chckArrayIndex iter = 0; GLHCKW()->textures && (cache = chckArrayIter(GLHCKW()->textures, &iter));) {
      if (cache->file && !strcmp(cache->file, file))
         return glhckTextureRef(cache);
   }

nothing:
   RET(1, "%p", NULL);
   return NULL;
}

/* \brief guess unit size for texture */
int _glhckUnitSizeForTexture(glhckTextureFormat format, glhckDataType type)
{
   size_t b;

   switch (type) {
      case GLHCK_BYTE:
      case GLHCK_UNSIGNED_BYTE:
         b = sizeof(unsigned char); break;
      case GLHCK_SHORT:
      case GLHCK_UNSIGNED_SHORT:
         b = sizeof(unsigned short); break;
      case GLHCK_INT:
      case GLHCK_UNSIGNED_INT:
         b = sizeof(unsigned int); break;
      case GLHCK_FLOAT:
         b = sizeof(float); break;
      case GLHCK_COMPRESSED:
         assert(0 && "This should not never happen"); break;
         break;
      default:
         assert(0 && "Datatype size not known");
         break;
   }

   return _glhckNumChannels(format) * b;
}

/* \brief guess size for texture */
int _glhckSizeForTexture(glhckTextureTarget target, int width, int height, int depth, glhckTextureFormat format, glhckDataType type)
{
   int size = 0;
   int unitSize = _glhckUnitSizeForTexture(format, type);

   switch (target) {
      case GLHCK_TEXTURE_1D:
         size = width * unitSize;
         break;
      case GLHCK_TEXTURE_2D:
      case GLHCK_TEXTURE_CUBE_MAP:
         size = width * height * unitSize;
         break;
      case GLHCK_TEXTURE_3D:
         size = width * height * depth * unitSize;
         break;
      default:assert(0 && "Target's size calculation not implemented");break;
   }

   return size;
}

/* \brief returns 1, if format contains alpha */
int _glhckHasAlpha(glhckTextureFormat format)
{
   switch (format) {
      case GLHCK_RG:
      case GLHCK_RED:
      case GLHCK_RGB:
      case GLHCK_BGR:
      case GLHCK_LUMINANCE:
      case GLHCK_COMPRESSED_RGB_DXT1:
      case GLHCK_DEPTH_COMPONENT:
      case GLHCK_DEPTH_COMPONENT16:
      case GLHCK_DEPTH_COMPONENT24:
      case GLHCK_DEPTH_COMPONENT32:
      case GLHCK_DEPTH_STENCIL:
         return 0;
      case GLHCK_RGBA:
      case GLHCK_BGRA:
      case GLHCK_ALPHA:
      case GLHCK_LUMINANCE_ALPHA:
      case GLHCK_COMPRESSED_RGBA_DXT5:
         return 1;
      default:break;
   }
   assert(0 && "Alpha check not implemented for texture format");
   return 0;
}

/* \brief returns number of channels needed for texture format */
unsigned int _glhckNumChannels(glhckTextureFormat format)
{
   switch (format) {
      case GLHCK_RED:
      case GLHCK_ALPHA:
      case GLHCK_LUMINANCE:
      case GLHCK_DEPTH_COMPONENT:
      case GLHCK_DEPTH_COMPONENT16:
      case GLHCK_DEPTH_COMPONENT24:
      case GLHCK_DEPTH_COMPONENT32:
      case GLHCK_DEPTH_STENCIL:
         return 1;
      case GLHCK_RG:
      case GLHCK_LUMINANCE_ALPHA:
         return 2;
      case GLHCK_RGB:
      case GLHCK_BGR:
      case GLHCK_COMPRESSED_RGB_DXT1:
         return 3;
      case GLHCK_RGBA:
      case GLHCK_BGRA:
      case GLHCK_COMPRESSED_RGBA_DXT5:
         return 4;
      default:break;
   }
   assert(0 && "Num channels not implemented for texture format");
   return 0;
}

/* \brief returns if format is compressed format */
int _glhckIsCompressedFormat(glhckTextureFormat format)
{
   switch (format) {
      case GLHCK_COMPRESSED_RGB_DXT1:
      case GLHCK_COMPRESSED_RGBA_DXT5:
         return 1;
      default:break;
   }
   return 0;
}

/* \brief get next power of two size */
void _glhckNextPow2(int width, int height, int depth, int *outWidth, int *outHeight, int *outDepth, int limitToSize)
{
   int dimensions[3] = { width, height, depth };

   /* every implementation above GL 2.0 should support NPOT */
   if (GLHCKRF()->texture.hasNativeNpotSupport) {
      if (outWidth) *outWidth = dimensions[0];
      if (outHeight) *outHeight = dimensions[1];
      if (outDepth) *outDepth = dimensions[2];
      return;
   }

   /* scale dimensions to next pot size */
   for (int i = 0; i != 3; ++i) {
      if ((dimensions[i] != 1) && (dimensions[i] & (dimensions[i] - 1))) {
         int pot;
         for (pot = 1; pot < dimensions[i]; pot *= 2);
         dimensions[i] = pot;
      }
   }

   /* limit dimensions if requested */
   if (limitToSize) {
      for (int i = 0; i != 3; ++i) {
         if (dimensions[i] > limitToSize)
            dimensions[i] = limitToSize;
      }
   }

   if (outWidth) *outWidth = dimensions[0];
   if (outHeight) *outHeight = dimensions[1];
   if (outDepth) *outDepth = dimensions[2];
}

/* ---- PUBLIC API ---- */

/* \brief allocate new texture object */
GLHCKAPI glhckTexture* glhckTextureNew(void)
{
   glhckTexture *object;

   /* allocate texture */
   if (!(object = _glhckCalloc(1, sizeof(glhckTexture))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* default target type */
   object->target = GLHCK_TEXTURE_2D;

   /* insert to world */
   _glhckWorldAdd(&GLHCKW()->textures, object);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckTextureFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief new texture object from file */
GLHCKAPI glhckTexture* glhckTextureNewFromFile(const char *file, const glhckImportImageParameters *importParams, const glhckTextureParameters *params)
{
   glhckTexture *object;
   CALL(0, "%s, %p, %p", file, importParams, params);
   assert(file);

   /* check if texture is in cache */
   if ((object = _glhckTextureCacheCheck(file)))
      goto success;

   if (!(object = glhckTextureNew()))
      goto fail;

   /* copy filename */
   if (!(object->file = _glhckStrdup(file)))
      goto fail;

   /* import image */
   if (_glhckImportImage(object, file, importParams) != RETURN_OK)
      goto fail;

   /* apply texture parameters */
   glhckTextureParameter(object, params);

success:
   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckTextureFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference texture */
GLHCKAPI glhckTexture* glhckTextureRef(glhckTexture *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   /* return reference */
   RET(2, "%p", object);
   return object;
}

/* \brief free texture */
GLHCKAPI unsigned int glhckTextureFree(glhckTexture *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   DEBUG(GLHCK_DBG_CRAP, "FREE(%p) %dx%dx%d", object, object->internalWidth, object->internalHeight, object->internalDepth);

   /* unbind from active slot */
   for (unsigned int i = 0; i != GLHCK_MAX_ACTIVE_TEXTURE; ++i) {
      if (GLHCKRD()->texture[i][object->target] == object)
         glhckTextureUnbind(object->target);
   }

   /* delete texture if there is one */
   if (object->object)
      GLHCKRA()->textureDelete(1, &object->object);

   /* free */
   IFDO(_glhckFree, object->file);

   /* remove from world */
   _glhckWorldRemove(&GLHCKW()->textures, object);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", (object ? object->refCounter : 0));
   return (object ? object->refCounter : 0);
}

/* \brief apply texture parameters. */
GLHCKAPI void glhckTextureParameter(glhckTexture *object, const glhckTextureParameters *params)
{
   CALL(2, "%p, %p", object, params);
   assert(object);

   /* copy texture parameters over */
   memcpy(&object->params, (params ? params : glhckTextureDefaultParameters()), sizeof(glhckTextureParameters));
   params = &object->params;

   /* push texture parameters to renderer */
   glhckTexture *old = glhckTextureCurrentForTarget(object->target);
   glhckTextureBind(object);
   GLHCKRA()->textureParameter(object->target, params);

   if (params->mipmap)
      GLHCKRA()->textureGenerateMipmap(object->target);

   if (old)
      glhckTextureBind(old);
}

/* \brief return default texture parameters */
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultParameters(void)
{
   static glhckTextureParameters defaultParameters = {
      .maxAnisotropy = 16.0f,
      .minLod        = -1000.0f,
      .maxLod        = 1000.0f,
      .biasLod       = 0.0f,
      .baseLevel     = 0,
      .maxLevel      = 1000,
      .wrapS         = GLHCK_REPEAT,
      .wrapT         = GLHCK_REPEAT,
      .wrapR         = GLHCK_REPEAT,
      .minFilter     = GLHCK_NEAREST_MIPMAP_LINEAR,
      .magFilter     = GLHCK_LINEAR,
      .compareMode   = GLHCK_COMPARE_NONE,
      .compareFunc   = GLHCK_LEQUAL,
      .mipmap        = 1,
   };
   return &defaultParameters;
}

/* \brief return default texture parameters without mipmap and repeat */
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultLinearParameters(void)
{
   static glhckTextureParameters defaultParameters = {
      .maxAnisotropy = 16.0f,
      .minLod        = -1000.0f,
      .maxLod        = 1000.0f,
      .biasLod       = 0.0f,
      .baseLevel     = 0,
      .maxLevel      = 1000,
      .wrapS         = GLHCK_CLAMP_TO_EDGE,
      .wrapT         = GLHCK_CLAMP_TO_EDGE,
      .wrapR         = GLHCK_CLAMP_TO_EDGE,
      .minFilter     = GLHCK_LINEAR,
      .magFilter     = GLHCK_LINEAR,
      .compareMode   = GLHCK_COMPARE_NONE,
      .compareFunc   = GLHCK_LEQUAL,
      .mipmap        = 0,
   };
   return &defaultParameters;
}

/* \brief return default sprite parameters */
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultSpriteParameters(void)
{
   static glhckTextureParameters defaultParameters = {
      .maxAnisotropy = 16.0f,
      .minLod        = -1000.0f,
      .maxLod        = 1000.0f,
      .biasLod       = 0.0f,
      .baseLevel     = 0,
      .maxLevel      = 1000,
      .wrapS         = GLHCK_CLAMP_TO_EDGE,
      .wrapT         = GLHCK_CLAMP_TO_EDGE,
      .wrapR         = GLHCK_CLAMP_TO_EDGE,
      .minFilter     = GLHCK_NEAREST,
      .magFilter     = GLHCK_NEAREST,
      .compareMode   = GLHCK_COMPARE_NONE,
      .compareFunc   = GLHCK_LEQUAL,
      .mipmap        = 0,
   };
   return &defaultParameters;
}

/* \brief get texture's information */
GLHCKAPI void glhckTextureGetInformation(glhckTexture *object, glhckTextureTarget *target, int *width, int *height, int *depth, int *border, glhckTextureFormat *format, glhckDataType *type)
{
   CALL(0, "%p, %p, %p, %p, %p, %p, %p, %p", object, target, width, height, depth, border, format, type);
   assert(object);
   if (target) *target = object->target;
   if (width) *width = object->width;
   if (height) *height = object->height;
   if (depth) *depth = object->depth;
   if (border) *border = object->border;
}

/* \brief create texture manually. */
GLHCKAPI int glhckTextureCreate(glhckTexture *object, glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, glhckDataType type, int size, const void *data)
{
   int twidth = width, theight = height, tdepth = depth;
   CALL(0, "%p, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", object, target, level, width, height, depth, border, format, type, size, data);
   assert(object);
   assert(level >= 0);

   /* check the true data size, if not provided */
   if (!size)
      size = _glhckSizeForTexture(target, width, height, depth, format, type);

   /* create texture */
   if (!object->object)
      GLHCKRA()->textureGenerate(1, &object->object);

   if (!object->object)
      goto fail;

   /* set texture type */
   object->target = target;

   /* scale to next pow2 */
   _glhckNextPow2(width, height, depth, &twidth, &theight, &tdepth, 0);

   glhckTexture *old = glhckTextureCurrentForTarget(object->target);
   glhckTextureBind(object);

   if (width == twidth && height == theight) {
      GLHCKRA()->textureImage(target, level, twidth, theight, tdepth, border, format, type, size, data);
   } else {
      GLHCKRA()->textureImage(target, level, twidth, theight, tdepth, border, format, type, 0, NULL);

      if (data)
         GLHCKRA()->textureFill(target, level, 0, 0, 0, width, height, depth, format, type, size, data);
   }

   if (old)
      glhckTextureBind(old);

   /* set rest */
   object->target = target;
   object->width  = width;
   object->height = height;
   object->depth  = depth;
   object->internalWidth = twidth;
   object->internalHeight = theight;
   object->internalDepth = tdepth;
   object->border = border;
   object->internalScale.x = (width?(kmScalar)width/twidth:1.0f);
   object->internalScale.y = (height?(kmScalar)height/theight:1.0f);
   object->internalScale.z = (depth?(kmScalar)depth/tdepth:1.0f);

   /* make aprox texture sizes show up in memory graph, even if not allocated */
   _glhckTrackFake(object, sizeof(glhckTexture) + size);

   /* FIXME: downscale and give warning about it
    * for now just throw big fat error! */
   if (twidth  > GLHCKRF()->texture.maxTextureSize ||
       theight > GLHCKRF()->texture.maxTextureSize ||
       tdepth  > GLHCKRF()->texture.maxTextureSize) {
      DEBUG(GLHCK_DBG_ERROR, "TEXTURE IS BIGGER THAN MAX TEXTURE SIZE (%d)", GLHCKRF()->texture.maxTextureSize);
   }

   DEBUG(GLHCK_DBG_CRAP, "NEW(%p:%u) %dx%dx%d %.2f MiB", object, object->object, object->internalWidth, object->internalHeight, object->internalDepth, (float)size/1048576);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief recreate texture with new data.
 * dimensions of texture must be the same. */
GLHCKAPI int glhckTextureRecreate(glhckTexture *object, glhckTextureFormat format, glhckDataType type, int size, const void *data)
{
   /* check the true data size, if not provided */
   if (!size)
      size = _glhckSizeForTexture(object->target, object->width, object->height, object->depth, format, type);

   return glhckTextureCreate(object, object->target, 0, object->internalWidth, object->internalHeight, object->internalDepth, 0, format, type, size, data);
}

/* \brief fill subdata to texture */
GLHCKAPI void glhckTextureFill(glhckTexture *object, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, glhckDataType type, int size, const void *data)
{
   CALL(2, "%p, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", object, level, x, y, z, width, height, depth, format, type, size, data);
   assert(object);
   assert(level >= 0);

   if (x + width > object->width) {
      DEBUG(GLHCK_DBG_ERROR, "x + width > texture width!");
      return;
   }
   if (y + height > object->height) {
      DEBUG(GLHCK_DBG_ERROR, "h + height > texture height!");
      return;
   }
   if (z + depth > object->depth) {
      DEBUG(GLHCK_DBG_ERROR, "z + depth > texture depth!");
      return;
   }

   glhckTexture *old = glhckTextureCurrentForTarget(object->target);
   glhckTextureBind(object);
   GLHCKRA()->textureFill(object->target, level, x, y, z, width, height, depth, format, type, size, data);

   if (old)
      glhckTextureBind(old);
}

/* \brief fill subdata to texture from source data */
GLHCKAPI void glhckTextureFillFrom(glhckTexture *object, int level, int sx, int sy, int sz, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, glhckDataType type, int size, const void *data)
{
   void *buffer;
   CALL(2, "%p, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", object, level, sx, sy, sz, x, y, z, width, height, depth, format, type, size, data);

   int bufferSize = _glhckSizeForTexture(object->target, width, height, depth, format, type);
   if (!(buffer = calloc(1, bufferSize)))
      return;

   int unitSize = _glhckUnitSizeForTexture(format, type);
   switch(object->target) {
      case GLHCK_TEXTURE_1D:
         memcpy(buffer, data + sx * unitSize, width * unitSize);
         break;
      case GLHCK_TEXTURE_2D:
      case GLHCK_TEXTURE_CUBE_MAP:
         for(int dy = 0; dy < height; ++dy) {
            memcpy(buffer + dy * width * unitSize,
                  data + ((sy + dy) * object->width + sx) * unitSize,
                  width * unitSize);
         }
         break;
      case GLHCK_TEXTURE_3D:
         for(int dz = 0; dz < depth; ++dz) {
            for(int dy = 0; dy < height; ++dy) {
               memcpy(buffer + (dz * height + dy) * width * unitSize,
                     data + ((sz + dz) * object->width * object->height + (sy + dy) * object->width + sx) * unitSize,
                     width * unitSize);
            }
         }
         break;
      default:
         assert(0 && "Filling from target type has not been implemented");
         break;
   }

   glhckTextureFill(object, level, x, y, z, width, height, depth, format, type,  size, buffer);
   free(buffer);
}

/* \brief save texture to file in TGA format */
GLHCKAPI int glhckTextureSave(glhckTexture *object, const char *path)
{
   FILE *f = NULL;
   CALL(0, "%p, %s", object, path);
   assert(object);

   DEBUG(GLHCK_DBG_CRAP, "\2Save \3%d\5x\3%d\5 [\4%s\5]", object->width, object->height, path);

   /* TODO: Render to FBO to get the image
    * Or use glGetTexImage if it's available (not in GLES) */

#if 0
   /* open for read */
   if (!(f = fopen(path, "wb")))
      goto fail;

   /* dump raw data */
   fwrite(object->data, 1, object->size, f);
   NULLDO(fclose, f);
#endif

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

//fail:
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief get current active texture for type */
GLHCKAPI glhckTexture* glhckTextureCurrentForTarget(glhckTextureTarget target)
{
   CALL(2, "%d", target);
   RET(2, "%p", GLHCKRD()->texture[GLHCKRD()->activeTexture][target]);
   return GLHCKRD()->texture[GLHCKRD()->activeTexture][target];
}

/* \brief active texture index */
GLHCKAPI void glhckTextureActive(unsigned int index)
{
   assert(index < GLHCK_MAX_ACTIVE_TEXTURE && "Tried to active bigger texture index than GLhck supports");

   if (GLHCKRD()->activeTexture == index)
      return;

   GLHCKRA()->textureActive(index);
   GLHCKRD()->activeTexture = index;
}

/* \brief bind texture */
GLHCKAPI void glhckTextureBind(glhckTexture *object)
{
   CALL(2, "%p", object);
   assert(object);

   if (GLHCKRD()->texture[GLHCKRD()->activeTexture][object->target] == object)
      return;

   GLHCKRA()->textureBind(object->target, object->object);
   GLHCKRD()->texture[GLHCKRD()->activeTexture][object->target] = object;
}

/* \brief unbind texture */
GLHCKAPI void glhckTextureUnbind(glhckTextureTarget target)
{
   CALL(2, "%d", target);

   if (!GLHCKRD()->texture[GLHCKRD()->activeTexture][target])
      return;

   GLHCKRA()->textureBind(target, 0);
   GLHCKRD()->texture[GLHCKRD()->activeTexture][target] = NULL;
}

/* \brief compress image data, this is public function which return data you must free! */
GLHCKAPI void* glhckTextureCompress(glhckTextureCompression compression, int width, int height, glhckTextureFormat format, glhckDataType type, const void *data, int *size, glhckTextureFormat *outFormat, glhckDataType *outType)
{
   void *compressed = NULL;
   CALL(0, "%d, %d, %d, %d, %d, %p, %p, %p", compression, width, height, format, type, data, size, outFormat);
   assert(type == GLHCK_UNSIGNED_BYTE && "Only GLHCK_UNSIGNED_BYTE is supported atm");
   if (size) *size = 0;
   if (outFormat) *outFormat = 0;

   /* NULL data, just return */
   if (!data)
      goto fail;

   /* check if already compressed */
   if (_glhckIsCompressedFormat(format))
      goto already_compressed;

   /* DXT compression requested */
   if (compression == GLHCK_COMPRESSION_DXT) {
      if ((_glhckNumChannels(format) & 1) == 1) {
         /* RGB */
         if (!(compressed = malloc(texhckSizeForDXT1(width, height))))
            goto out_of_memory;

         texhckConvertToDXT1(compressed, data, width, height, _glhckNumChannels(format));
         if (size)      *size      = texhckSizeForDXT1(width, height);
         if (outFormat) *outFormat = GLHCK_COMPRESSED_RGB_DXT1;
         if (outType)   *outType   = GLHCK_UNSIGNED_BYTE;
         DEBUG(GLHCK_DBG_CRAP, "Image data converted to DXT1");
      } else {
         /* RGBA */
         if (!(compressed = malloc(texhckSizeForDXT5(width, height))))
            goto out_of_memory;

         texhckConvertToDXT5(compressed, data, width, height, _glhckNumChannels(format));
         if (size)      *size      = texhckSizeForDXT5(width, height);
         if (outFormat) *outFormat = GLHCK_COMPRESSED_RGBA_DXT5;
         if (outType)   *outType   = GLHCK_UNSIGNED_BYTE;
         DEBUG(GLHCK_DBG_CRAP, "Image data converted to DXT5");
      }
   } else {
      DEBUG(GLHCK_DBG_WARNING, "Compression type not implemented or supported for the texture format/datatype combination.");
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

/* vim: set ts=8 sw=3 tw=0 :*/
