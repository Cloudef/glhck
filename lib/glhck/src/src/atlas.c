#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ATLAS
#define VEC2(v)   v?v->x:-1, v?v->y:-1
#define VEC2S     "vec2[%f, %f, %f]"

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
   unsigned short count;
   CALL("%p, %p", atlas, texture);
   assert(atlas && texture);

   /* find if the texture is already added */
   for (rect = atlas->rect; rect &&
        rect->texture != texture; rect = rect->next);

   /* this texture is already inserted */
   if (rect)
      goto success;

   /* insert here */
   count = 0;
   if (!(rect = atlas->rect)) {
      rect = atlas->rect =
         _glhckMalloc(sizeof(_glhckAtlasRect));
   } else {
      for (; rect && rect->next; rect = rect->next) ++count;
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

/* \brief pack textures to atlas */
GLHCKAPI int glhckAtlasPack(glhckAtlas *atlas, const int power_of_two, const int border)
{
   int width, height;
   unsigned short count;
   _glhckTexturePacker *tp;
   _glhckAtlasRect *rect;
   _glhckRtt *rtt = NULL;
   CALL("%p, %d, %d", atlas, power_of_two, border);

   /* new texture packer */
   tp = _glhckTexturePackerNew();

   /* add && set indexes to textures &&
    * count them at same time */
   for (count = 0, rect = atlas->rect;
        rect; rect = rect->next) {
      rect->index = count++;
      _glhckTexturePackerAdd(tp,
            rect->texture->width, rect->texture->height);
   }

   /* pack textures */
   _glhckTexturePackerPack(tp, &width, &height, power_of_two, border);

   if (!(rtt = glhckRttNew(width, height, GLHCK_RTT_RGBA)))
      goto fail;

   glhckRttBegin(rtt);
   for (rect = atlas->rect; rect; rect = rect->next) {
      rect->packed.rotated = _glhckTexturePackerGetLocation(tp,
            rect->index, &rect->packed.x1, &rect->packed.y1,
            &rect->packed.y1, &rect->packed.y2);

      /* we have all the info, render texture here */
   }
   glhckRttEnd(rtt);

   /* free rtt */
   glhckRttFree(rtt);

   /* free the texture packer */
   _glhckTexturePackerFree(tp);

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   if (tp) _glhckTexturePackerFree(tp);
   if (rtt) glhckRttFree(rtt);
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief retuirn transformed coordinates of packed texture */
GLHCKAPI int glchkAtlasGetTransformed(glhckAtlas *atlas, glhckTexture *texture,
      const kmVec2 *in, kmVec2 *out)
{
   float atlasWidth, atlasHeight;
   float width, height, x, y;
   _glhckAtlasArea *packed;
   kmVec2 center = { 0.5f, 0.5f };

   CALL("%p, %p, "VEC2S", "VEC2S, atlas, texture, VEC2(in), VEC2(out));
   assert(atlas && texture && in && out);

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
