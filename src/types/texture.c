#include "texture.h"

#include <glhck/import.h>

#include <stdlib.h> /* for malloc */
#include <stdio.h>  /* for file io */
#include <assert.h> /* for assert */

#include "trace.h"
#include "handle.h"
#include "texhck.h"
#include "renderers/render.h"
#include "system/tls.h"
#include "pool/pool.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_TEXTURE

struct dimensions {
   int width, height, depth;
};

#define GLHCK_MAX_ACTIVE_TEXTURE 8

static _GLHCK_TLS struct {
   glhckHandle texture[GLHCK_MAX_ACTIVE_TEXTURE][GLHCK_TEXTURE_TYPE_LAST];
   unsigned char active;
} rstate;

enum pool {
   $params, // glhckTextureParameters
   $file, // string handle
   $internalScale, // kmVec3
   $object, // unsigend int
   $flags, // unsigned int
   $importFlags, // unsigned int
   $dimensions, // struct dimensions
   $internalDimensions, // struct dimensions
   $target, // glhckTextureTarget
   $border, // unsigned char
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckTextureParameters), // params
   sizeof(glhckHandle), // file
   sizeof(kmVec3), // internalScale
   sizeof(unsigned int), // object
   sizeof(unsigned int), // flags
   sizeof(unsigned int), // importFlags
   sizeof(struct dimensions), // dimensions
   sizeof(struct dimensions), // internalDimensions
   sizeof(glhckTextureTarget), // target
   sizeof(unsigned char), // border
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

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
   if (glhckRendererGetFeatures()->texture.hasNativeNpotSupport) {
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

unsigned int _glhckTextureGetObject(const glhckHandle handle)
{
   return *(unsigned int*)get($object, handle);
}

const kmVec3* _glhckTextureGetInternalScale(const glhckHandle handle)
{
   return get($internalScale, handle);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const struct dimensions *internal = get($internalDimensions, handle);
   DEBUG(GLHCK_DBG_CRAP, "FREE(%s) %dx%dx%d", glhckHandleRepr(handle), internal->width, internal->height, internal->depth);

#if 0
   /* unbind from active slot */
   for (unsigned int i = 0; i != GLHCK_MAX_ACTIVE_TEXTURE; ++i) {
      if (GLHCKRD()->texture[i][object->target] == object)
         glhckTextureUnbind(object->target);
   }
#endif

   /* delete texture if there is one */
   unsigned int *object = (unsigned int*)get($object, handle);
   if (*object)
      _glhckRenderGetAPI()->textureDelete(1, object);

   releaseHandle($file, handle);
}

/* ---- PUBLIC API ---- */

/* \brief allocate new texture object */
GLHCKAPI glhckHandle glhckTextureNew(void)
{
   glhckHandle handle = 0;

   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_TEXTURE, pools, pool_sizes, POOL_LAST, destructor, NULL)))
      goto fail;

   set($target, handle, (glhckTextureTarget[]){GLHCK_TEXTURE_2D});

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief new texture object from file */
GLHCKAPI glhckHandle glhckTextureNewFromFile(const char *file, const glhckPostProcessImageParameters *postParams, const glhckTextureParameters *params)
{
   glhckHandle handle = 0;
   CALL(0, "%s, %p, %p", file, postParams, params);
   assert(file);

#if 0
   /* check if texture is in cache */
   if ((object = glhckLibraryGetTexture(file)))
      goto success;
#endif

#if 0
   glhckImportImageStruct *import;
   if (!(import = glhckImportImageFile(file)))
      goto fail;
#endif

   if (!(handle = glhckTextureNew()))
      goto fail;

   setCStr($file, handle, file);
   glhckTextureParameter(handle, params);

success:
   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief apply texture parameters. */
GLHCKAPI void glhckTextureParameter(const glhckHandle handle, const glhckTextureParameters *params)
{
   CALL(2, "%s, %p", glhckHandleRepr(handle), params);

   /* copy texture parameters over */
   params = (params ? params : glhckTextureDefaultLinearParameters());
   set($params, handle, params);

   /* push texture parameters to renderer */
   const glhckTextureTarget target = glhckTextureGetTarget(handle);
   glhckHandle old = glhckTextureCurrentForTarget(target);
   glhckTextureBind(handle);

   _glhckRenderGetAPI()->textureParameter(target, params);

   if (params->mipmap)
      _glhckRenderGetAPI()->textureGenerateMipmap(target);

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
GLHCKAPI void glhckTextureGetInformation(const glhckHandle handle, glhckTextureTarget *target, int *width, int *height, int *depth, int *border, glhckTextureFormat *format, glhckDataType *type)
{
   CALL(0, "%s, %p, %p, %p, %p, %p, %p, %p", glhckHandleRepr(handle), target, width, height, depth, border, format, type);
   if (target) *target = *(glhckTextureTarget*)get($target, handle);
   if (width) *width = ((const struct dimensions*)get($dimensions, handle))->width;
   if (height) *height = ((const struct dimensions*)get($dimensions, handle))->height;
   if (depth) *depth = ((const struct dimensions*)get($dimensions, handle))->depth;
   if (border) *border = *(unsigned char*)get($border, handle);
}

GLHCKAPI int glhckTextureGetTarget(const glhckHandle handle)
{
   return *(glhckTextureTarget*)get($target, handle);
}

GLHCKAPI int glhckTextureGetWidth(const glhckHandle handle)
{
   return ((struct dimensions*)get($dimensions, handle))->width;
}

GLHCKAPI int glhckTextureGetHeight(const glhckHandle handle)
{
   return ((struct dimensions*)get($dimensions, handle))->height;
}

/* \brief create texture manually. */
GLHCKAPI int glhckTextureCreate(const glhckHandle handle, const glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, glhckDataType type, int size, const void *data)
{
   int twidth = width, theight = height, tdepth = depth;
   CALL(0, "%s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", glhckHandleRepr(handle), target, level, width, height, depth, border, format, type, size, data);
   assert(level >= 0);

   /* check the true data size, if not provided */
   if (!size)
      size = _glhckSizeForTexture(target, width, height, depth, format, type);

   /* create texture */
   unsigned int *object = (unsigned int*)get($object, handle);
   if (!*object)
      _glhckRenderGetAPI()->textureGenerate(1, object);

   if (!*object)
      goto fail;

   /* set texture type */
   set($target, handle, &target);

   /* scale to next pow2 */
   _glhckNextPow2(width, height, depth, &twidth, &theight, &tdepth, 0);

   glhckHandle old = glhckTextureCurrentForTarget(target);
   glhckTextureBind(handle);

   if (width == twidth && height == theight) {
      _glhckRenderGetAPI()->textureImage(target, level, twidth, theight, tdepth, border, format, type, size, data);
   } else {
      _glhckRenderGetAPI()->textureImage(target, level, twidth, theight, tdepth, border, format, type, 0, NULL);

      if (data)
         _glhckRenderGetAPI()->textureFill(target, level, 0, 0, 0, width, height, depth, format, type, size, data);
   }

   if (old)
      glhckTextureBind(old);

   /* set rest */
   struct dimensions *dimensions = (struct dimensions*)get($dimensions, handle);
   struct dimensions *internalDimensions = (struct dimensions*)get($internalDimensions, handle);
   dimensions->width = width;
   dimensions->height = height;
   dimensions->depth = depth;
   internalDimensions->width = twidth;
   internalDimensions->height = theight;
   internalDimensions->depth = tdepth;

   set($border, handle, &border);

   kmVec3 *internalScale = (kmVec3*)get($internalScale, handle);
   internalScale->x = (width ? (kmScalar)width/twidth : 1.0f);
   internalScale->y = (height ? (kmScalar)height/theight : 1.0f);
   internalScale->z = (depth ? (kmScalar)depth/tdepth : 1.0f);

   /* FIXME: downscale and give warning about it
    * for now just throw big fat error! */
   if (twidth  > glhckRendererGetFeatures()->texture.maxTextureSize ||
       theight > glhckRendererGetFeatures()->texture.maxTextureSize ||
       tdepth  > glhckRendererGetFeatures()->texture.maxTextureSize) {
      DEBUG(GLHCK_DBG_ERROR, "TEXTURE IS BIGGER THAN MAX TEXTURE SIZE (%d)", glhckRendererGetFeatures()->texture.maxTextureSize);
   }

   DEBUG(GLHCK_DBG_CRAP, "NEW [%u] (%s) %dx%dx%d %.2f MiB", *object, glhckHandleRepr(handle), twidth, theight, tdepth, (float)size/1048576);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief recreate texture with new data.
 * dimensions of texture must be the same. */
GLHCKAPI int glhckTextureRecreate(const glhckHandle handle, const glhckTextureFormat format, const glhckDataType type, int size, const void *data)
{
   const struct dimensions *dimensions = get($dimensions, handle);
   const glhckTextureTarget target = glhckTextureGetTarget(handle);

   if (!size)
      size = _glhckSizeForTexture(target, dimensions->width, dimensions->height, dimensions->depth, format, type);

   const struct dimensions *internalDimensions = get($internalDimensions, handle);
   return glhckTextureCreate(handle, target, 0, internalDimensions->width, internalDimensions->height, internalDimensions->depth, 0, format, type, size, data);
}

/* \brief fill subdata to texture */
GLHCKAPI void glhckTextureFill(const glhckHandle handle, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, glhckDataType type, int size, const void *data)
{
   CALL(2, "%s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", glhckHandleRepr(handle), level, x, y, z, width, height, depth, format, type, size, data);
   assert(level >= 0);

   const struct dimensions *dimensions = get($dimensions, handle);

   if (x + width > dimensions->width) {
      DEBUG(GLHCK_DBG_ERROR, "x + width > texture width!");
      return;
   }
   if (y + height > dimensions->height) {
      DEBUG(GLHCK_DBG_ERROR, "h + height > texture height!");
      return;
   }
   if (z + depth > dimensions->depth) {
      DEBUG(GLHCK_DBG_ERROR, "z + depth > texture depth!");
      return;
   }

   const glhckTextureTarget target = glhckTextureGetTarget(handle);
   const glhckHandle old = glhckTextureCurrentForTarget(target);
   glhckTextureBind(handle);

   _glhckRenderGetAPI()->textureFill(target, level, x, y, z, width, height, depth, format, type, size, data);

   if (old)
      glhckTextureBind(old);
}

/* \brief fill subdata to texture from source data */
GLHCKAPI void glhckTextureFillFrom(const glhckHandle handle, int level, int sx, int sy, int sz, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, glhckDataType type, int size, const void *data)
{
   CALL(2, "%s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", glhckHandleRepr(handle), level, sx, sy, sz, x, y, z, width, height, depth, format, type, size, data);

   const glhckTextureTarget target = glhckTextureGetTarget(handle);

   void *buffer;
   const int bufferSize = _glhckSizeForTexture(target, width, height, depth, format, type);
   if (!(buffer = calloc(1, bufferSize)))
      return;

   const int unitSize = _glhckUnitSizeForTexture(format, type);
   switch(target) {
      case GLHCK_TEXTURE_1D:
         memcpy(buffer, data + sx * unitSize, width * unitSize);
         break;
      case GLHCK_TEXTURE_2D:
      case GLHCK_TEXTURE_CUBE_MAP:
         {
            const struct dimensions *dimensions = get($dimensions, handle);
            for (int dy = 0; dy < height; ++dy) {
               memcpy(buffer + dy * width * unitSize,
                     data + ((sy + dy) * dimensions->width + sx) * unitSize,
                     width * unitSize);
            }
         }
         break;
      case GLHCK_TEXTURE_3D:
         {
            const struct dimensions *dimensions = get($dimensions, handle);
            for (int dz = 0; dz < depth; ++dz) {
               for (int dy = 0; dy < height; ++dy) {
                  memcpy(buffer + (dz * height + dy) * width * unitSize,
                        data + ((sz + dz) * dimensions->width * dimensions->height + (sy + dy) * dimensions->width + sx) * unitSize,
                        width * unitSize);
               }
            }
         }
         break;
      default:
         assert(0 && "Filling from target type has not been implemented");
         break;
   }

   glhckTextureFill(handle, level, x, y, z, width, height, depth, format, type,  size, buffer);
   free(buffer);
}

/* \brief save texture to file in TGA format */
GLHCKAPI int glhckTextureSave(const glhckHandle handle, const char *path)
{
   FILE *f = NULL;
   CALL(0, "%s, %s", glhckHandleRepr(handle), path);

   const struct dimensions *dimensions = get($dimensions, handle);
   DEBUG(GLHCK_DBG_CRAP, "\2Save \3%d\5x\3%d\5 [\4%s\5]", dimensions->width, dimensions->height, path);

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
GLHCKAPI glhckHandle glhckTextureCurrentForTarget(glhckTextureTarget target)
{
   CALL(2, "%d", target);
   RET(2, "%s", glhckHandleRepr(rstate.texture[rstate.active][target]));
   return rstate.texture[rstate.active][target];
}

/* \brief active texture index */
GLHCKAPI void glhckTextureActive(unsigned int index)
{
   assert(index < GLHCK_MAX_ACTIVE_TEXTURE && "Tried to active bigger texture index than GLhck supports");

   if (rstate.active == index)
      return;

   _glhckRenderGetAPI()->textureActive(index);
   rstate.active = index;
}

/* \brief bind texture */
GLHCKAPI void glhckTextureBind(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));

   const glhckTextureTarget target = glhckTextureGetTarget(handle);

   if (rstate.texture[rstate.active][target] == handle)
      return;

   const unsigned int object = _glhckTextureGetObject(handle);
   _glhckRenderGetAPI()->textureBind(target, object);
   rstate.texture[rstate.active][target] = object;
}

/* \brief unbind texture */
GLHCKAPI void glhckTextureUnbind(glhckTextureTarget target)
{
   CALL(2, "%d", target);

   if (!rstate.texture[rstate.active][target])
      return;

   _glhckRenderGetAPI()->textureBind(target, 0);
   rstate.texture[rstate.active][target] = 0;
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
   IFDO(free, compressed);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
