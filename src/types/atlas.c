#include "atlas.h"
#include "atlas/atlas.h"

#include "system/tls.h"
#include "handle.h"
#include "trace.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ATLAS

struct area {
   unsigned int x1, y1, x2, y2;
   unsigned char rotated;
};

struct rect {
   struct area packed;
   glhckHandle texture;
};

enum pool {
   $rects, // list handle (struct rect)
   $texture, // glhckHandle
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // rects
   sizeof(glhckHandle), // texture
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

/* \brief get packed area for packed texture */
static const struct area* getPackedArea(const struct rect *rects, const size_t memb, const glhckHandle texture)
{
   CALL(2, "%p, %zu, %s", rects, memb, glhckHandleRepr(texture));

   for (size_t i = 0; i < memb; ++i) {
      if (rects[i].texture == texture) {
         RET(2, "%p", &rects[i].packed);
         return &rects[i].packed;
      }
   }

   RET(2, "%p", NULL);
   return NULL;
}

/* \brief drop all rects and free reference to texture */
static void dropRects(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   size_t memb;
   const struct rect *rects = getList($rects, handle, &memb);
   for (size_t i = 0; i < memb; ++i)
      glhckHandleRelease(rects[i].texture);

   releaseHandle($rects, handle);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   dropRects(handle);
   releaseHandle($texture, handle);
}

/***
 * public api
 ***/

/* \brief create new atlas atlas */
GLHCKAPI glhckHandle glhckAtlasNew(void)
{
   TRACE(0);

   glhckHandle handle;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_FRAMEBUFFER, pools, pool_sizes, POOL_LAST, destructor, NULL)))
      goto fail;

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

GLHCKAPI int glhckAtlasInsertTextures(const glhckHandle handle, const glhckHandle *textures, const size_t memb)
{
   CALL(0, "%s, %p, %zu", glhckHandleRepr(handle), textures, memb);

   glhckHandle list;
   if (!(list = glhckListNew(memb, sizeof(struct rect))))
      goto fail;

   for (size_t i = 0; i < memb; ++i) {
      struct rect *rect = glhckListFetch(list, i);
      glhckHandleRef(textures[i]);
      rect->texture = textures[i];
   }

   size_t num;
   const struct rect *old = getList($rects, handle, &num);
   for (size_t i = 0; i < num; ++i)
      glhckHandleRelease(old[i].texture);

   setHandle($rects, handle, list);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}


/* \brief return combined texture */
GLHCKAPI glhckHandle glhckAtlasGetTexture(const glhckHandle handle)
{
   return *(glhckHandle*)get($texture, handle);
}

/* \brief pack textures to atlas */
GLHCKAPI int glhckAtlasPack(const glhckHandle handle, const glhckTextureFormat format, const int powerOfTwo, const int border, const glhckTextureParameters *params)
{
   chckAtlas *atlas = NULL;
   glhckHandle fbo = 0, texture = 0, plane = 0, material = 0;
   CALL(0, "%s, %u, %d, %d, %p", glhckHandleRepr(handle), format, powerOfTwo, border, params);

   size_t memb;
   struct rect *rects = (struct rect*)getList($rects, handle, &memb);

   if (!rects || memb <= 0)
      goto fail;

   if (memb == 1) {
      setHandle($texture, handle, rects[0].texture);
      goto success;
   }

   if (!(atlas = chckAtlasNew()))
      goto fail;

   for (size_t i = 0; i < memb; ++i)
      chckAtlasPush(atlas, glhckTextureGetWidth(rects[i].texture), glhckTextureGetHeight(rects[i].texture));

   unsigned int width, height;
   chckAtlasPack(atlas, powerOfTwo, border, &width, &height);

   /* downscale if over maximum texture size */
   unsigned int realWidth = width, realHeight = height;
   unsigned int maxTexSize = glhckRendererGetFeatures()->texture.maxTextureSize;

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

   if (!(material = glhckMaterialNew(0)))
      goto fail;

   /* set material to atlas */
   glhckObjectMaterial(plane, material);

   /* create projection for drawing */
   kmMat4 projection;
   kmMat4Translation(&projection, -1.0f, -1.0f, 0.0f);

   /* render atlas */
   glhckFramebufferRecti(fbo, 0, 0, width, height);
   glhckFramebufferBegin(fbo);
   glhckRenderFlip(0);
   glhckRenderPass(GLHCK_PASS_TEXTURE);
   glhckRenderProjectionOnly(&projection);
   glhckRenderClearColoru(0, 0, 0, 0);
   glhckRenderClear(GLHCK_COLOR_BUFFER_BIT);

   for (size_t i = 0; i < memb; ++i) {
      rects[i].packed.rotated = chckAtlasGetTextureLocation(atlas, i,
            &rects[i].packed.x1, &rects[i].packed.y1, &rects[i].packed.x2, &rects[i].packed.y2);

      /* rotate if need */
      if (rects[i].packed.rotated) {
         glhckObjectRotatef(plane, 0, 0, 90);
      } else {
         glhckObjectRotatef(plane, 0, 0, 0);
      }

      /* position */
      glhckObjectScalef(plane, (kmScalar)rects[i].packed.x2/realWidth, (kmScalar)rects[i].packed.y2/realHeight, 1.0f);
      glhckObjectPositionf(plane,
            (kmScalar)(rects[i].packed.x1 * 2 + rects[i].packed.x2)/realWidth,
            (kmScalar)(rects[i].packed.y1 * 2 + rects[i].packed.y2)/realHeight,
            0.0f);

      /* transform rect to fit the real width */
      if (width != realWidth) {
         rects[i].packed.x1 *= (float)width/realWidth;
         rects[i].packed.x2 *= (float)width/realWidth;
      }

      /* transform rect to fit the real height */
      if (height != realHeight) {
         rects[i].packed.y1 *= (float)height/realHeight;
         rects[i].packed.y2 *= (float)height/realHeight;
      }

      /* draw texture */
      glhckMaterialTexture(material, rects[i].texture);
      glhckRenderObject(plane);
   }

   glhckFramebufferEnd(fbo);

   /* reference rtt's texture */
   glhckTextureParameter(texture, (params ? params : glhckTextureDefaultSpriteParameters()));
   setHandle($texture, handle, texture);

#if 0
   if (_glhckHasAlpha(format))
      texture->importFlags |= GLHCK_TEXTURE_IMPORT_ALPHA;
#endif

   /* cleanup */
   glhckHandleRelease(material);
   glhckHandleRelease(plane);
   glhckHandleRelease(fbo);
   chckAtlasFree(atlas);

success:
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(glhckHandleRelease, material);
   IFDO(glhckHandleRelease, plane);
   IFDO(glhckHandleRelease, fbo);
   IFDO(glhckHandleRelease, texture);
   IFDO(chckAtlasFree, atlas);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return pointer to texture by index */
GLHCKAPI glhckHandle glhckAtlasGetTextureByIndex(const glhckHandle handle, const unsigned int index)
{
   CALL(1, "%s, %d", glhckHandleRepr(handle), index);
   assert(handle > 0);

   size_t memb;
   const struct rect *rects = getList($rects, handle, &memb);

   if (index >= memb)
      return 0;

   RET(1, "%s", glhckHandleRepr(rects[index].texture));
   return rects[index].texture;
}

/* \brief return transformed coordinates of packed texture */
GLHCKAPI int glhckAtlasGetTransform(const glhckHandle handle, const glhckHandle texture, glhckRect *out, short *outDegrees)
{
   CALL(2, "%s, %s, %p, %p", glhckHandleRepr(handle), glhckHandleRepr(texture), out, outDegrees);
   assert(handle > 0 && texture > 0 && out && outDegrees);

   const glhckHandle atlasTexture = glhckAtlasGetTexture(handle);

   if (!atlasTexture)
      goto fail;

   size_t memb;
   const struct rect *rects = getList($rects, handle, &memb);

   if (memb <= 0)
      goto fail;

   if (memb == 1) {
      *outDegrees = 0;
      out->x = out->y = 0.0f;
      out->w = out->h = 1.0f;
      goto success;
   }

   const struct area *packed;
   if (!(packed = getPackedArea(rects, memb, texture)))
      goto fail;

   float atlasWidth = glhckTextureGetWidth(atlasTexture);
   float atlasHeight = glhckTextureGetHeight(atlasTexture);
   out->w = packed->x2 / atlasWidth;
   out->h = packed->y2 / atlasHeight;
   out->x = packed->x1 / atlasWidth;
   out->y = packed->y1 / atlasHeight;
   *outDegrees = (packed->rotated ? -90 : 0);

success:
   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return coordinates transformed with the packed texture's transform */
GLHCKAPI int glhckAtlasTransformCoordinates(const glhckHandle handle, const glhckHandle texture, const kmVec2 *in, kmVec2 *out)
{
   CALL(2, "%s, %s, %p, %p", glhckHandleRepr(handle), glhckHandleRepr(texture), in, out);

   size_t memb = getListCount($rects, handle);

   if (memb <= 0)
      goto fail;

   if (memb == 1) {
      kmVec2Assign(out, in);
      goto success;
   }

   short degrees;
   glhckRect transformed;
   if (glhckAtlasGetTransform(handle, texture, &transformed, &degrees) != RETURN_OK)
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
