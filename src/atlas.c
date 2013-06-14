#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ATLAS

/* \brief count textures */
static unsigned short _glhckAtlasNumTextures(const glhckAtlas *object)
{
   _glhckAtlasRect *r;
   unsigned short count = 0;
   for (r = object->rect; r; r = r->next) ++count;
   return count;
}

/* \brief get packed area for packed texture */
static _glhckAtlasArea* _glhckAtlasGetPackedArea(const glhckAtlas *object, glhckTexture *texture)
{
   _glhckAtlasRect *r;
   CALL(2, "%p, %p", object, texture);
   assert(object && texture);

   /* find packed area */
   for (r = object->rect; r; r = r->next)
      if (r->texture == texture) {
         RET(2, "%p", &r->packed);
         return &r->packed;
      }

   RET(2, "%p", NULL);
   return NULL;
}

/* \brief drop all rects and free reference to texture */
static void _glhckAtlasDropRects(glhckAtlas *object)
{
   _glhckAtlasRect *r, *rn;
   CALL(0, "%p", object);
   assert(object);

   /* drop all rects */
   for (r = object->rect; r; r = rn) {
      rn = r->next;
      glhckTextureFree(r->texture);
      _glhckFree(r);
   }
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
   _glhckWorldInsert(atlas, object, glhckAtlas*);

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

   /* free atlas texture */
   IFDO(glhckTextureFree, object->texture);

   /* remove from world */
   _glhckWorldRemove(atlas, object, glhckAtlas*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief insert texture to atlas */
GLHCKAPI int glhckAtlasInsertTexture(glhckAtlas *object, glhckTexture *texture)
{
   _glhckAtlasRect *rect;
   CALL(0, "%p, %p", object, texture);
   assert(object && texture);

   /* find if the texture is already added */
   for (rect = object->rect; rect &&
        rect->texture != texture; rect = rect->next);

   /* this texture is already inserted */
   if (rect)
      goto success;

   /* insert here */
   if (!(rect = object->rect)) {
      rect = object->rect = _glhckCalloc(1, sizeof(_glhckAtlasRect));
   } else {
      for (; rect && rect->next; rect = rect->next);
      rect = rect->next = _glhckCalloc(1, sizeof(_glhckAtlasRect));
   }

   if (!rect)
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
GLHCKAPI int glhckAtlasRemoveTexture(glhckAtlas *object, glhckTexture *texture)
{
   _glhckAtlasRect *rect, *found;
   CALL(0, "%p, %p", object, texture);
   assert(object && texture);

   if (!(rect = object->rect))
      goto _return;

   /* remove here */
   if (rect->texture == texture) {
      object->rect = rect->next;
      _glhckFree(rect);
   } else {
      for (; rect && rect->next &&
             rect->next->texture != texture;
             rect = rect->next);
      if ((found = rect->next)) {
         rect->next = found->next;
         _glhckFree(found);
      }
   }

_return:
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
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
   int width, height, maxTexSize;
   unsigned short count;
   _glhckTexturePacker *tp;
   _glhckAtlasRect *rect;
   glhckFramebuffer *fbo = NULL;
   glhckTexture *texture = NULL;
   glhckObject *plane = NULL;
   glhckMaterial *material = NULL;
   kmMat4 projection;
   CALL(0, "%p, %d, %d", object, powerOfTwo, border);

   /* count textures */
   for (count = 0, rect = object->rect;
        rect; rect = rect->next) {
      rect->index = count++;
   }

   /* only one texture silly */
   if (count==1) {
      IFDO(glhckTextureFree, object->texture);
      object->texture = glhckTextureRef(object->rect->texture);
      RET(0, "%d", RETURN_OK);
      return RETURN_OK;
   }

   /* new texture packer */
   if (!(tp = _glhckTexturePackerNew()))
      goto fail;

   _glhckTexturePackerCount(tp, count);

   /* add textures to packer */
   for (rect = object->rect;
        rect; rect = rect->next)
      _glhckTexturePackerAdd(tp,
            rect->texture->width, rect->texture->height);

   /* pack textures */
   _glhckTexturePackerPack(tp, &width, &height, powerOfTwo, border);

   /* downscale if over maximum texture size */

   /* FIXME: render properties! */
   maxTexSize = 2048;
int rwidth = width, rheight = height;
   if (width > maxTexSize) {
      height *= (float)maxTexSize/width;
      width   = maxTexSize;
      DEBUG(0, "Downscaling atlas texture to: %dx%d", width, height);
   } else if (height > maxTexSize) {
      width *= (float)maxTexSize/height;
      height = maxTexSize;
      DEBUG(0, "Downscaling atlas texture to: %dx%d", width, height);
   }

   /* create stuff needed for rendering */
   if (!(texture = glhckTextureNew()))
      goto fail;
   if (!(glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, width, height, 0, 0, format, GLHCK_DATA_UNSIGNED_BYTE, 0, NULL)))
      goto fail;
   if (!(fbo = glhckFramebufferNew(GLHCK_FRAMEBUFFER)))
      goto fail;
   if (!glhckFramebufferAttachTexture(fbo, texture, GLHCK_COLOR_ATTACHMENT0))
      goto fail;
   if (!(plane = glhckPlaneNewEx(1, 1, GLHCK_INDEX_NONE, GLHCK_VERTEX_V2F)))
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
   glhckRenderClear(GLHCK_COLOR_BUFFER);
   for (rect = object->rect; rect; rect = rect->next) {
      rect->packed.rotated = _glhckTexturePackerGetLocation(tp,
            rect->index, &rect->packed.x1, &rect->packed.y1,
            &rect->packed.x2, &rect->packed.y2);

      /* rotate if need */
      if (rect->packed.rotated)
         glhckObjectRotatef(plane, 0, 0, 90);
      else
         glhckObjectRotatef(plane, 0, 0, 0);

      /* position */
      glhckObjectScalef(plane, (kmScalar)rect->packed.x2/rwidth, (kmScalar)rect->packed.y2/rheight, 0);
      glhckObjectPositionf(plane,
            (kmScalar)(rect->packed.x1*2+rect->packed.x2)/rwidth,
            (kmScalar)(rect->packed.y1*2+rect->packed.y2)/rheight,
            0);

      /* transform rect to fit the real width */
      if (width != rwidth) {
         rect->packed.x1 *= (float)width/rwidth;
         rect->packed.x2 *= (float)width/rwidth;
      }

      /* transform rect to fit the real height */
      if (height != rheight) {
         rect->packed.y1 *= (float)height/rheight;
         rect->packed.y2 *= (float)height/rheight;
      }

      /* draw texture */
      glhckMaterialTexture(material, rect->texture);
      glhckObjectRender(plane);
   }
   glhckFramebufferEnd(fbo);

   /* reference rtt's texture */
   IFDO(glhckTextureFree, object->texture);
   glhckTextureParameter(texture, (params?params:glhckTextureDefaultSpriteParameters()));
   if (_glhckHasAlpha(format)) texture->importFlags |= GLHCK_TEXTURE_IMPORT_ALPHA;
   object->texture = texture;

   /* cleanup */
   glhckMaterialFree(material);
   glhckObjectFree(plane);
   glhckFramebufferFree(fbo);
   _glhckTexturePackerFree(tp);

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
   _glhckAtlasRect *rect;
   CALL(1, "%p, %d", object, index);
   assert(object);

   for (rect = object->rect; rect &&
        rect->index != index; rect = rect->next);

   RET(1, "%p", rect?rect->texture:NULL);
   return rect?rect->texture:NULL;
}

/* \brief return transformed coordinates of packed texture */
GLHCKAPI int glhckAtlasGetTransform(const glhckAtlas *object, glhckTexture *texture, glhckRect *out, short *degrees)
{
   float atlasWidth, atlasHeight;
   _glhckAtlasArea *packed;

   CALL(2, "%p, %p, %p, %p", object, texture, out, degrees);
   assert(object && texture && out && degrees);

   if (!object->texture)
      goto fail;

   /* only one texture silly */
   if (_glhckAtlasNumTextures(object)==1) {
      *degrees = 0;
      out->x = out->y = 0.0f;
      out->w = out->h = 1.0f;
      RET(2, "%d", RETURN_OK);
      return RETURN_OK;
   }

   if (!(packed = _glhckAtlasGetPackedArea(object, texture)))
      goto fail;

   atlasWidth  = object->texture->width;
   atlasHeight = object->texture->height;
   out->w = packed->x2 / atlasWidth;
   out->h = packed->y2 / atlasHeight;
   out->x = packed->x1 / atlasWidth;
   out->y = packed->y1 / atlasHeight;
   *degrees = packed->rotated?-90:0;

   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return coordinates transformed with the packed texture's transform */
GLHCKAPI int glhckAtlasTransformCoordinates(const glhckAtlas *object, glhckTexture *texture, const kmVec2 *in, kmVec2 *out)
{
   short degrees;
   glhckRect transformed;
   kmVec2 center = { 0.5f, 0.5f };
   CALL(2, "%p, %p, %p, %p", object, texture, in, out);

   /* only one texture */
   if (_glhckAtlasNumTextures(object) == 1) {
      kmVec2Assign(out, in);
      RET(2, "%d", RETURN_OK);
      return RETURN_OK;
   }

   if (glhckAtlasGetTransform(object, texture, &transformed, &degrees) != RETURN_OK)
      goto fail;

   if (kmAlmostEqual(transformed.w, 0.f) || kmAlmostEqual(transformed.h, 0.f))
      goto fail;

   /* do we need to rotate? */
   if (degrees != 0) kmVec2RotateBy(out, in, degrees, &center);
   else              kmVec2Assign(out, in);

   /* out */
   out->x *= transformed.w;
   out->x += transformed.x;
   out->y *= transformed.h;
   out->y += transformed.y;

   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   kmVec2Assign(out, in);
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
