#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ATLAS

/* \brief get packed area for packed texture */
static _glhckAtlasArea* _glhckAtlasGetPackedArea(const glhckAtlas *object, glhckTexture *texture)
{
   CALL(2, "%p, %p", object, texture);
   assert(object && texture);

   /* find packed area */
   _glhckAtlasRect *r;
   for (chckPoolIndex iter = 0; (r = chckIterPoolIter(object->rects, &iter));) {
      if (r->texture == texture) {
         RET(2, "%p", &r->packed);
         return &r->packed;
      }
   }

   RET(2, "%p", NULL);
   return NULL;
}

/* \brief drop all rects and free reference to texture */
static void _glhckAtlasDropRects(glhckAtlas *object)
{
   CALL(0, "%p", object);
   assert(object);

   /* drop all rects */
   _glhckAtlasRect *r;
   for (chckPoolIndex iter = 0; (r = chckIterPoolIter(object->rects, &iter));)
      glhckTextureFree(r->texture);

   chckIterPoolFlush(object->rects);
}

/***
 * public api
 ***/

/* \brief create new atlas object */
GLHCKAPI glhckAtlas* glhckAtlasNew(void)
{
   glhckAtlas *object;
   TRACE(0);

   if (!(object = _glhckCalloc(1, sizeof(glhckAtlas))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldAdd(&GLHCKW()->atlases, object);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief free atlas */
GLHCKAPI unsigned int glhckAtlasFree(glhckAtlas *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this atlas alive */
   if (--object->refCounter != 0) goto success;

   /* free all references */
   _glhckAtlasDropRects(object);

   /* free rects */
   IFDO(chckIterPoolFree, object->rects);

   /* free atlas texture */
   IFDO(glhckTextureFree, object->texture);

   /* remove from world */
   _glhckWorldRemove(&GLHCKW()->atlases, object);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", (object ? object->refCounter : 0));
   return (object ? object->refCounter : 0);
}

/* \brief insert texture to atlas */
GLHCKAPI int glhckAtlasInsertTexture(glhckAtlas *object, glhckTexture *texture)
{
   CALL(0, "%p, %p", object, texture);
   assert(object && texture);

   /* find if the texture is already added */
   if (_glhckAtlasGetPackedArea(object, texture))
      goto success;

   if (!object->rects && !(object->rects = chckIterPoolNew(32, 1, sizeof(_glhckAtlasRect))))
      goto fail;

   _glhckAtlasRect *rect;
   if (!(rect = chckIterPoolAdd(object->rects, NULL, NULL)))
      goto fail;

   /* assign reference */
   rect->texture  = glhckTextureRef(texture);

success:
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief remove texture from atlas */
GLHCKAPI void glhckAtlasRemoveTexture(glhckAtlas *object, glhckTexture *texture)
{
   CALL(0, "%p, %p", object, texture);
   assert(object && texture);

   _glhckAtlasRect *r;
   for (chckPoolIndex iter = 0; (r = chckIterPoolIter(object->rects, &iter));) {
      if (r->texture == texture) {
         glhckTextureFree(r->texture);
         chckIterPoolRemove(object->rects, iter - 1);
         break;
      }
   }
}

/* \brief return combined texture */
GLHCKAPI glhckTexture* glhckAtlasGetTexture(glhckAtlas *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%p", object->texture);
   return object->texture;
}

/* \brief pack textures to atlas */
GLHCKAPI int glhckAtlasPack(glhckAtlas *object, glhckTextureFormat format, int powerOfTwo, int border, const glhckTextureParameters *params)
{
   _glhckTexturePacker *tp = NULL;
   glhckFramebuffer *fbo = NULL;
   glhckTexture *texture = NULL;
   glhckObject *plane = NULL;
   glhckMaterial *material = NULL;
   CALL(0, "%p, %d, %d", object, powerOfTwo, border);

   if (!object->rects)
      goto fail;

   /* only one texture silly */
   if (chckIterPoolCount(object->rects) == 1) {
      IFDO(glhckTextureFree, object->texture);
      _glhckAtlasRect *r = chckIterPoolGet(object->rects, 0);
      object->texture = glhckTextureRef(r->texture);
      goto success;
   }

   /* new texture packer */
   if (!(tp = _glhckTexturePackerNew()))
      goto fail;

   _glhckTexturePackerCount(tp, chckIterPoolCount(object->rects));

   /* add textures to packer */
   _glhckAtlasRect *r;
   for (chckPoolIndex iter = 0; (r = chckIterPoolIter(object->rects, &iter));)
      _glhckTexturePackerAdd(tp, r->texture->width, r->texture->height);

   /* pack textures */
   int width, height;
   _glhckTexturePackerPack(tp, &width, &height, powerOfTwo, border);

   /* downscale if over maximum texture size */
   int realWidth = width, realHeight = height;
   int maxTexSize = GLHCKRF()->texture.maxTextureSize;
   kmMat4 projection;
   if (width > maxTexSize) {
      height *= (float)maxTexSize/width;
      width = maxTexSize;
      DEBUG(GLHCK_DBG_WARNING, "Downscaling atlas texture to: %dx%d", width, height);
   } else if (height > maxTexSize) {
      width *= (float)maxTexSize/height;
      height = maxTexSize;
      DEBUG(GLHCK_DBG_WARNING, "Downscaling atlas texture to: %dx%d", width, height);
   }

   /* create stuff needed for rendering */
   if (!(texture = glhckTextureNew()))
      goto fail;
   if (!(glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, width, height, 0, 0, format, GLHCK_UNSIGNED_BYTE, 0, NULL)))
      goto fail;
   if (!(fbo = glhckFramebufferNew(GLHCK_FRAMEBUFFER)))
      goto fail;
   if (!glhckFramebufferAttachTexture(fbo, texture, GLHCK_COLOR_ATTACHMENT0))
      goto fail;
   if (!(plane = glhckPlaneNewEx(1, 1, GLHCK_IDX_AUTO, GLHCK_VTX_V2F)))
      goto fail;
   if (!(material = glhckMaterialNew(NULL)))
      goto fail;

   /* set material to object */
   glhckObjectMaterial(plane, material);

   /* create projection for drawing */
   kmMat4Translation(&projection, -1.0f, -1.0f, 0.0f);

   /* render atlas */
   glhckFramebufferRecti(fbo, 0, 0, width, height);
   glhckFramebufferBegin(fbo);
   glhckRenderFlip(0);
   glhckRenderPass(GLHCK_PASS_TEXTURE);
   glhckRenderProjectionOnly(&projection);
   glhckRenderClearColorb(0,0,0,0);
   glhckRenderClear(GLHCK_COLOR_BUFFER_BIT);
   for (chckPoolIndex iter = 0; (r = chckIterPoolIter(object->rects, &iter));) {
      r->packed.rotated = _glhckTexturePackerGetLocation(tp, r->index, &r->packed.x1, &r->packed.y1, &r->packed.x2, &r->packed.y2);

      /* rotate if need */
      if (r->packed.rotated) {
         glhckObjectRotatef(plane, 0, 0, 90);
      } else {
         glhckObjectRotatef(plane, 0, 0, 0);
      }

      /* position */
      glhckObjectScalef(plane, (kmScalar)r->packed.x2/realWidth, (kmScalar)r->packed.y2/realHeight, 1.0f);
      glhckObjectPositionf(plane,
            (kmScalar)(r->packed.x1 * 2 + r->packed.x2)/realWidth,
            (kmScalar)(r->packed.y1 * 2 + r->packed.y2)/realHeight,
            0.0f);

      /* transform rect to fit the real width */
      if (width != realWidth) {
         r->packed.x1 *= (float)width/realWidth;
         r->packed.x2 *= (float)width/realWidth;
      }

      /* transform rect to fit the real height */
      if (height != realHeight) {
         r->packed.y1 *= (float)height/realHeight;
         r->packed.y2 *= (float)height/realHeight;
      }

      /* draw texture */
      glhckMaterialTexture(material, r->texture);
      glhckObjectRender(plane);
   }
   glhckFramebufferEnd(fbo);

   /* reference rtt's texture */
   IFDO(glhckTextureFree, object->texture);
   glhckTextureParameter(texture, (params ? params : glhckTextureDefaultSpriteParameters()));

   if (_glhckHasAlpha(format))
      texture->importFlags |= GLHCK_TEXTURE_IMPORT_ALPHA;

   object->texture = texture;

   /* cleanup */
   glhckMaterialFree(material);
   glhckObjectFree(plane);
   glhckFramebufferFree(fbo);
   _glhckTexturePackerFree(tp);

success:
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(glhckMaterialFree, material);
   IFDO(glhckObjectFree, plane);
   IFDO(glhckFramebufferFree, fbo);
   IFDO(glhckTextureFree, texture);
   IFDO(_glhckTexturePackerFree, tp);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return pointer to texture by index */
GLHCKAPI glhckTexture* glhckAtlasGetTextureByIndex(const glhckAtlas *object, unsigned short index)
{
   CALL(1, "%p, %d", object, index);
   assert(object);

   if (index >= chckIterPoolCount(object->rects))
      return NULL;

   _glhckAtlasRect *rect = chckIterPoolGet(object->rects, index);

   RET(1, "%p", (rect ? rect->texture : NULL));
   return (rect ? rect->texture : NULL);
}

/* \brief return transformed coordinates of packed texture */
GLHCKAPI int glhckAtlasGetTransform(const glhckAtlas *object, glhckTexture *texture, glhckRect *out, short *degrees)
{
   CALL(2, "%p, %p, %p, %p", object, texture, out, degrees);
   assert(object && texture && out && degrees);

   if (!object->texture)
      goto fail;

   /* only one texture silly */
   if (chckIterPoolCount(object->rects) == 1) {
      *degrees = 0;
      out->x = out->y = 0.0f;
      out->w = out->h = 1.0f;
      goto success;
   }

   _glhckAtlasArea *packed;
   if (!(packed = _glhckAtlasGetPackedArea(object, texture)))
      goto fail;

   float atlasWidth = object->texture->width;
   float atlasHeight = object->texture->height;
   out->w = packed->x2 / atlasWidth;
   out->h = packed->y2 / atlasHeight;
   out->x = packed->x1 / atlasWidth;
   out->y = packed->y1 / atlasHeight;
   *degrees = (packed->rotated ? -90 : 0);

success:
   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return coordinates transformed with the packed texture's transform */
GLHCKAPI int glhckAtlasTransformCoordinates(const glhckAtlas *object, glhckTexture *texture, const kmVec2 *in, kmVec2 *out)
{
   CALL(2, "%p, %p, %p, %p", object, texture, in, out);

   /* only one texture */
   if (chckIterPoolCount(object->rects) == 1) {
      kmVec2Assign(out, in);
      goto success;
   }

   short degrees;
   glhckRect transformed;
   if (glhckAtlasGetTransform(object, texture, &transformed, &degrees) != RETURN_OK)
      goto fail;

   if (kmAlmostEqual(transformed.w, 0.f) || kmAlmostEqual(transformed.h, 0.f))
      goto fail;

   /* do we need to rotate? */
   if (degrees != 0) {
      static const kmVec2 center = { 0.5f, 0.5f };
      kmVec2RotateBy(out, in, degrees, &center);
   } else {
      kmVec2Assign(out, in);
   }

   /* out */
   out->x *= transformed.w;
   out->x += transformed.x;
   out->y *= transformed.h;
   out->y += transformed.y;

success:
   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   kmVec2Assign(out, in);
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
