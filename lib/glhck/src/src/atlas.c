#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ATLAS
#define VEC2(v)   v?v->x:-1, v?v->y:-1
#define VEC2S     "vec2[%f, %f]"

/* \brief get packed area for packed texture */
static _glhckAtlasArea* _glhckAtlasGetPackedArea(
      _glhckAtlas *atlas, _glhckTexture *texture)
{
   _glhckAtlasRect *r;
   CALL("%p, %p", atlas, texture);
   assert(atlas && texture);

   /* find packed area */
   for (r = atlas->rect; r; r = r->next)
      if (r->texture == texture) {
         RET("%p", &r->packed);
         return &r->packed;
      }

   RET("%p", NULL);
   return NULL;
}

/* \brief drop all rects and free reference to texture */
static void _glhckAtlasDropRects(_glhckAtlas *atlas)
{
   _glhckAtlasRect *r, *rn;
   CALL("%p", atlas);
   assert(atlas);

   /* drop all rects */
   for (r = atlas->rect; r; r = rn) {
      rn = r->next;
      glhckTextureFree(r->texture);
      _glhckFree(r);
   }
}

/* public api */

/* \brief create new atlas object */
GLHCKAPI glhckAtlas* glhckAtlasNew(void)
{
   _glhckAtlas *atlas;
   TRACE();

   if (!(atlas = _glhckMalloc(sizeof(_glhckAtlas))))
      goto fail;

   /* init altas */
   memset(atlas, 0, sizeof(_glhckAtlas));

   /* increase reference */
   atlas->refCounter++;

   RET("%p", atlas);
   return atlas;

fail:
   RET("%p", NULL);
   return NULL;
}

/* \brief free atlas */
GLHCKAPI short glhckAtlasFree(glhckAtlas *atlas)
{
   CALL("%p", atlas);
   assert(atlas);

   /* there is still references to this atlas alive */
   if (--atlas->refCounter != 0) goto success;

   /* free all references */
   _glhckAtlasDropRects(atlas);

   /* free atlas texture */
   if (atlas->texture)
      glhckTextureFree(atlas->texture);

   /* free */
   _glhckFree(atlas);
   atlas = NULL;

success:
   RET("%d", atlas?atlas->refCounter:0);
   return atlas?atlas->refCounter:0;
}

/* \brief insert texture to atlas */
GLHCKAPI int glhckAtlasInsertTexture(glhckAtlas *atlas, glhckTexture *texture)
{
   _glhckAtlasRect *rect;
   CALL("%p, %p", atlas, texture);
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
         _glhckMalloc(sizeof(_glhckAtlasRect));
   } else {
      for (; rect && rect->next; rect = rect->next);
      rect = rect->next =
         _glhckMalloc(sizeof(_glhckAtlasRect));
   }

   if (!rect)
      goto fail;

   /* init rect */
   memset(rect, 0, sizeof(_glhckAtlasRect));

   /* assign reference */
   rect->texture  = glhckTextureRef(texture);

success:
   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief remove texture from atlas */
GLHCKAPI int glhckAtlasRemoveTexture(glhckAtlas *atlas, glhckTexture *texture)
{
   _glhckAtlasRect *rect, *found;
   CALL("%p, %p", atlas, texture);
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
   RET("%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief return combined texture */
GLHCKAPI glhckTexture* glhckAtlasGetTexture(glhckAtlas *atlas)
{
   CALL("%p", atlas);
   assert(atlas);

   RET("%p", atlas->texture);
   return atlas->texture;
}

#include <GL/gl.h>
/* \brief pack textures to atlas */
GLHCKAPI int glhckAtlasPack(glhckAtlas *atlas, const int power_of_two, const int border)
{
   int width, height;
   unsigned short count;
   kmMat4 old_projection;
   _glhckTexturePacker *tp;
   _glhckAtlasRect *rect;
   _glhckRtt *rtt = NULL;
   _glhckObject *plane = NULL;
   kmMat4 ortho;
   CALL("%p, %d, %d", atlas, power_of_two, border);

   /* new texture packer */
   tp = _glhckTexturePackerNew();

   /* count textures */
   for (count = 0, rect = atlas->rect;
        rect; rect = rect->next) {
      rect->index = count++;
   }
   _glhckTexturePackerSetCount(tp, count);

   /* add textures to packer */
   for (rect = atlas->rect;
        rect; rect = rect->next)
      _glhckTexturePackerAdd(tp,
            rect->texture->width, rect->texture->height);

   /* pack textures */
   _glhckTexturePackerPack(tp, &width, &height, power_of_two, border);

   if (!(rtt = glhckRttNew(width, height, GLHCK_RTT_RGBA)))
      goto fail;

   if (!(plane = glhckPlaneNew(1)))
      goto fail;

   kmMat4OrthographicProjection(&ortho, 0, width, 0, height, -1.0f, 1.0f);
   kmMat4Translation(&ortho, -1, -1, 0);

   _GLHCKlibrary.render.api.resize(width, height);
   old_projection = _GLHCKlibrary.render.api.getProjection();
   _GLHCKlibrary.render.api.setProjection(&ortho);

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
      glhckTextureBind(rect->texture);
      glhckObjectDraw(plane);
   }
   glhckRttFillData(rtt);
   glhckRttEnd(rtt);

   /* restore old projection && size
    * TODO: get size from renderer */
   _GLHCKlibrary.render.api.setProjection(&old_projection);
   _GLHCKlibrary.render.api.resize(_GLHCKlibrary.render.width,
         _GLHCKlibrary.render.height);

   /* free plane */
   glhckObjectFree(plane);

   /* reference rtt's texture */
   if (atlas->texture) glhckTextureFree(atlas->texture);
   atlas->texture = glhckTextureRef(glhckRttGetTexture(rtt));
   glhckTextureSave(atlas->texture, "test.tga");

   /* free rtt */
   glhckRttFree(rtt);

   /* free the texture packer */
   _glhckTexturePackerFree(tp);

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   if (plane) glhckObjectFree(plane);
   if (rtt) glhckRttFree(rtt);
   if (tp) _glhckTexturePackerFree(tp);
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return pointer to texture by index */
GLHCKAPI glhckTexture* glhckAtlasGetTextureByIndex(glhckAtlas *atlas,
      unsigned short index)
{
   _glhckAtlasRect *rect;
   CALL("%p, %d", atlas, index);
   assert(atlas);

   for (rect = atlas->rect; rect &&
        rect->index != index; rect = rect->next);

   RET("%p", rect?rect->texture:NULL);
   return rect?rect->texture:NULL;
}

/* \brief return transformed coordinates of packed texture */
GLHCKAPI int glhckAtlasGetTransformed(glhckAtlas *atlas, glhckTexture *texture,
      const kmVec2 *in, kmVec2 *out)
{
   float atlasWidth, atlasHeight;
   float width, height, x, y;
   _glhckAtlasArea *packed;
   kmVec2 center = { 0.5f, 0.5f };

   CALL("%p, %p, "VEC2S", "VEC2S, atlas, texture, VEC2(in), VEC2(out));
   assert(atlas && texture && in && out);

   if (!atlas->texture)
      goto fail;

   if (!(packed = _glhckAtlasGetPackedArea(atlas, texture)))
      goto fail;

   atlasWidth  = atlas->texture->width;
   atlasHeight = atlas->texture->height;

   width    = packed->x2 / atlasWidth;
   height   = packed->y2 / atlasHeight;
   x        = packed->x1 / atlasWidth;
   y        = packed->y1 / atlasHeight;

   /* do we need to rotate? */
   if (packed->rotated)
      kmVec2RotateBy(out, in, -90, &center);
   else
      kmVec2Assign(out, in);

   /* out */
   out->x *= width;
   out->x += x;
   out->y *= height;
   out->y += y;

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
