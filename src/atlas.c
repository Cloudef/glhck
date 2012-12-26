#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ATLAS

/* \brief count textures */
static unsigned short _glhckAtlasNumTextures(const _glhckAtlas *atlas)
{
   _glhckAtlasRect *r;
   unsigned short count = 0;
   for (r = atlas->rect; r; r = r->next) ++count;
   return count;
}

/* \brief get packed area for packed texture */
static _glhckAtlasArea* _glhckAtlasGetPackedArea(
      const _glhckAtlas *atlas, _glhckTexture *texture)
{
   _glhckAtlasRect *r;
   CALL(2, "%p, %p", atlas, texture);
   assert(atlas && texture);

   /* find packed area */
   for (r = atlas->rect; r; r = r->next)
      if (r->texture == texture) {
         RET(2, "%p", &r->packed);
         return &r->packed;
      }

   RET(2, "%p", NULL);
   return NULL;
}

/* \brief drop all rects and free reference to texture */
static void _glhckAtlasDropRects(_glhckAtlas *atlas)
{
   _glhckAtlasRect *r, *rn;
   CALL(0, "%p", atlas);
   assert(atlas);

   /* drop all rects */
   for (r = atlas->rect; r; r = rn) {
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
   _glhckAtlas *atlas;
   TRACE(0);

   if (!(atlas = _glhckCalloc(1, sizeof(_glhckAtlas))))
      goto fail;

   /* increase reference */
   atlas->refCounter++;

   /* insert to world */
   _glhckWorldInsert(alist, atlas, _glhckAtlas*);

   RET(0, "%p", atlas);
   return atlas;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief free atlas */
GLHCKAPI size_t glhckAtlasFree(glhckAtlas *atlas)
{
   CALL(FREE_CALL_PRIO(atlas), "%p", atlas);
   assert(atlas);

   /* not initialized */
   if (!_glhckInitialized) return 0;

   /* there is still references to this atlas alive */
   if (--atlas->refCounter != 0) goto success;

   /* free all references */
   _glhckAtlasDropRects(atlas);

   /* free atlas texture */
   IFDO(glhckTextureFree, atlas->texture);

   /* remove from world */
   _glhckWorldRemove(alist, atlas, _glhckAtlas*);

   /* free */
   NULLDO(_glhckFree, atlas);

success:
   RET(FREE_RET_PRIO(atlas), "%d", atlas?atlas->refCounter:0);
   return atlas?atlas->refCounter:0;
}

/* \brief insert texture to atlas */
GLHCKAPI int glhckAtlasInsertTexture(glhckAtlas *atlas, glhckTexture *texture)
{
   _glhckAtlasRect *rect;
   CALL(0, "%p, %p", atlas, texture);
   assert(atlas && texture);

   /* find if the texture is already added */
   for (rect = atlas->rect; rect &&
        rect->texture != texture; rect = rect->next);

   /* this texture is already inserted */
   if (rect)
      goto success;

   /* insert here */
   if (!(rect = atlas->rect)) {
      rect = atlas->rect =
         _glhckCalloc(1, sizeof(_glhckAtlasRect));
   } else {
      for (; rect && rect->next; rect = rect->next);
      rect = rect->next =
         _glhckCalloc(1, sizeof(_glhckAtlasRect));
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
GLHCKAPI int glhckAtlasRemoveTexture(glhckAtlas *atlas, glhckTexture *texture)
{
   _glhckAtlasRect *rect, *found;
   CALL(0, "%p, %p", atlas, texture);
   assert(atlas && texture);

   if (!(rect = atlas->rect))
      goto _return;

   /* remove here */
   if (rect->texture == texture) {
      atlas->rect = rect->next;
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
GLHCKAPI glhckTexture* glhckAtlasGetTexture(glhckAtlas *atlas)
{
   CALL(1, "%p", atlas);
   assert(atlas);

   RET(1, "%p", atlas->texture);
   return atlas->texture;
}

/* \brief pack textures to atlas */
GLHCKAPI int glhckAtlasPack(glhckAtlas *atlas, const int power_of_two, const int border)
{
   int width, height, maxTexSize;
   unsigned short count;
   glhckColorb old_clear;
   kmMat4 old_projection;
   _glhckTexturePacker *tp;
   _glhckAtlasRect *rect;
   _glhckRtt *rtt = NULL;
   _glhckObject *plane = NULL;
   kmMat4 ortho;
   CALL(0, "%p, %d, %d", atlas, power_of_two, border);

   /* count textures */
   for (count = 0, rect = atlas->rect;
        rect; rect = rect->next) {
      rect->index = count++;
   }

   /* only one texture silly */
   if (count==1) {
      IFDO(glhckTextureFree, atlas->texture);
      atlas->texture = glhckTextureRef(atlas->rect->texture);
      RET(0, "%d", RETURN_OK);
      return RETURN_OK;
   }

   /* new texture packer */
   if (!(tp = _glhckTexturePackerNew()))
      goto fail;

   _glhckTexturePackerSetCount(tp, count);

   /* add textures to packer */
   for (rect = atlas->rect;
        rect; rect = rect->next)
      _glhckTexturePackerAdd(tp,
            rect->texture->width, rect->texture->height);

   /* pack textures */
   _glhckTexturePackerPack(tp, &width, &height, power_of_two, border);

   /* downscale if over maximum texture size */
   glhckRenderGetIntegerv(GLHCK_MAX_TEXTURE_SIZE, &maxTexSize);
   if (width > maxTexSize) {
      height *= (float)maxTexSize/width;
      width   = maxTexSize;
      DEBUG(0, "Downscaling atlas texture to: %dx%d", width, height);
   } else if (height > maxTexSize) {
      width *= (float)maxTexSize/height;
      height = maxTexSize;
      DEBUG(0, "Downscaling atlas texture to: %dx%d", width, height);
   }

   if (!(rtt = glhckRttNew(width, height, GLHCK_RTT_RGBA, GLHCK_TEXTURE_DEFAULTS)))
      goto fail;

   if (!(plane = glhckPlaneNewEx(1, 1, GLHCK_INDEX_NONE, GLHCK_VERTEX_V2F)))
      goto fail;

   /* create projection for drawing */
   kmMat4OrthographicProjection(&ortho, 0, width, 0, height, -1.0f, 1.0f);
   kmMat4Translation(&ortho, -1, -1, 0);

   memcpy(&old_projection, _GLHCKlibrary.render.api.getProjection(), sizeof(kmMat4));
   _GLHCKlibrary.render.api.setProjection(&ortho);

   /* set clear color */
   memcpy(&old_clear, &_GLHCKlibrary.render.draw.clearColor, sizeof(glhckColorb));
   _GLHCKlibrary.render.api.setClearColor(0,0,0,0);

   glhckRttBegin(rtt);
   _GLHCKlibrary.render.api.clear();
   for (rect = atlas->rect; rect; rect = rect->next) {
      rect->packed.rotated = _glhckTexturePackerGetLocation(tp,
            rect->index, &rect->packed.x1, &rect->packed.y1,
            &rect->packed.x2, &rect->packed.y2);

      /* rotate if need */
      if (rect->packed.rotated)
         glhckObjectRotatef(plane, 0, 0, 90);
      else
         glhckObjectRotatef(plane, 0, 0, 0);

      /* position */
      glhckObjectScalef(plane, (float)rect->packed.x2/width, (float)rect->packed.y2/height, 0);
      glhckObjectPositionf(plane,
            (float)(rect->packed.x1*2+rect->packed.x2)/width,
            (float)(rect->packed.y1*2+rect->packed.y2)/height,
            0);

      /* draw texture */
      glhckObjectTexture(plane, rect->texture);
      glhckObjectRender(plane);
   }
   glhckRttFillData(rtt);
   glhckRttEnd(rtt);

   /* restore old state */
   _GLHCKlibrary.render.api.setProjection(&old_projection);
   _GLHCKlibrary.render.api.setClearColor(
         old_clear.r, old_clear.g, old_clear.b, old_clear.a);

   /* free plane */
   glhckObjectFree(plane);

   /* reference rtt's texture */
   IFDO(glhckTextureFree, atlas->texture);
   atlas->texture = glhckTextureRef(glhckRttGetTexture(rtt));
   glhckTextureSave(atlas->texture, "test.tga");

   /* free rtt */
   glhckRttFree(rtt);

   /* free the texture packer */
   _glhckTexturePackerFree(tp);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(glhckObjectFree, plane);
   IFDO(glhckRttFree, rtt);
   IFDO(_glhckTexturePackerFree, tp);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return pointer to texture by index */
GLHCKAPI glhckTexture* glhckAtlasGetTextureByIndex(const glhckAtlas *atlas,
      unsigned short index)
{
   _glhckAtlasRect *rect;
   CALL(1, "%p, %d", atlas, index);
   assert(atlas);

   for (rect = atlas->rect; rect &&
        rect->index != index; rect = rect->next);

   RET(1, "%p", rect?rect->texture:NULL);
   return rect?rect->texture:NULL;
}

/* \brief return transformed coordinates of packed texture */
GLHCKAPI int glhckAtlasGetTransform(const glhckAtlas *atlas, glhckTexture *texture,
      glhckRect *out, short *degrees)
{
   float atlasWidth, atlasHeight;
   _glhckAtlasArea *packed;

   CALL(2, "%p, %p, %p, %p", atlas, texture, out, degrees);
   assert(atlas && texture && out && degrees);

   if (!atlas->texture)
      goto fail;

   /* only one texture silly */
   if (_glhckAtlasNumTextures(atlas)==1) {
      *degrees = 0;
      out->x = out->y = 0.0f;
      out->w = out->h = 1.0f;
      RET(2, "%d", RETURN_OK);
      return RETURN_OK;
   }

   if (!(packed = _glhckAtlasGetPackedArea(atlas, texture)))
      goto fail;

   atlasWidth  = atlas->texture->width;
   atlasHeight = atlas->texture->height;

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
GLHCKAPI int glhckAtlasTransformCoordinates(const glhckAtlas *atlas, glhckTexture *texture,
      const kmVec2 *in, kmVec2 *out)
{
   short degrees;
   glhckRect transformed;
   kmVec2 center = { 0.5f, 0.5f };
   CALL(2, "%p, %p, %p, %p", atlas, texture, in, out);

   /* only one texture */
   if (_glhckAtlasNumTextures(atlas)==1) {
      kmVec2Assign(out, in);
      RET(2, "%d", RETURN_OK);
      return RETURN_OK;
   }

   if (glhckAtlasGetTransform(atlas, texture, &transformed, &degrees)
         != RETURN_OK)
      goto fail;

   if (transformed.w == 0.f || transformed.h == 0.f)
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
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
