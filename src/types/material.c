#include <glhck/glhck.h>

#include "system/tls.h"
#include "texture.h"
#include "handle.h"
#include "trace.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_MATERIAL

struct blend {
   glhckBlendingMode blenda, blendb;
};

enum pool {
   $textureScale, // kmVec2
   $textureOffset, // kmVec2
   $textureRotation, // kmScalar
   $shader, // glhckHandle
   $texture, // glhckHandle
   $ambient, // glhckColor
   $diffuse, // glhckColor
   $emissive, // glhckColor
   $specular, // glhckColor
   $shininess, // kmScalar
   $blend, // struct blend
   $flags, // unsigned int
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(kmVec2), // textureScale
   sizeof(kmVec2), // textureOffset
   sizeof(kmScalar), // textureRotation
   sizeof(glhckHandle), // shader
   sizeof(glhckHandle), // texture
   sizeof(glhckColor), // ambient
   sizeof(glhckColor), // diffuse
   sizeof(glhckColor), // emmisive
   sizeof(glhckColor), // specular
   sizeof(kmScalar), // shininess
   sizeof(struct blend), // blend
   sizeof(unsigned int), // flags
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckMaterialTexture(handle, 0);
   glhckMaterialShader(handle, 0);
}

/* \brief allocate new material object */
GLHCKAPI glhckHandle glhckMaterialNew(const glhckHandle texture)
{
   TRACE(0);

   glhckHandle handle = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_MATERIAL, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   glhckMaterialTextureScalef(handle, 1.0f, 1.0f);
   glhckMaterialAmibentu(handle, 1, 1, 1, 255);
   glhckMaterialDiffuseu(handle, 255, 255, 255, 255);
   glhckMaterialSpecularu(handle, 200, 200, 200, 255);
   glhckMaterialShininess(handle, 0.9f);
   glhckMaterialOptions(handle, GLHCK_MATERIAL_LIGHTING);
   glhckMaterialTexture(handle, texture);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(handle));
   return 0;
}

/* \brief set options assigned to material */
GLHCKAPI void glhckMaterialOptions(const glhckHandle handle, const unsigned int flags)
{
   set($flags, handle, &flags);
}

/* \brief get options assigned to material */
GLHCKAPI unsigned int glhckMaterialGetOptions(const glhckHandle handle)
{
   return *(unsigned int*)get($flags, handle);
}

/* \brief set shader to material */
GLHCKAPI void glhckMaterialShader(const glhckHandle handle, const glhckHandle shader)
{
   setHandle($shader, handle, shader);
}

/* \brief get assigned shader from material */
GLHCKAPI glhckHandle glhckMaterialGetShader(const glhckHandle handle)
{
   return *(glhckHandle*)get($shader, handle);
}

/* \brief set texture to material */
GLHCKAPI void glhckMaterialTexture(const glhckHandle handle, const glhckHandle texture)
{
   if (!setHandle($texture, handle, texture))
      return;

#if 0
   if (texture) {
      const unsigned int flags = _glhckTextureGetImportFlags(texture);
      if (flags & GLHCK_TEXTURE_IMPORT_TEXT) {
         glhckMaterialBlendFunc(handle, GLHCK_ONE, GLHCK_ONE_MINUS_SRC_ALPHA);
      } else if (flags & GLHCK_TEXTURE_IMPORT_ALPHA) {
         glhckMaterialBlendFunc(handle, GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);
      } else {
         glhckMaterialBlendFunc(handle, 0, 0);
      }
   }
#endif
}

/* \brief get assigned texture from material */
GLHCKAPI glhckHandle glhckMaterialGetTexture(const glhckHandle handle)
{
   return *(glhckHandle*)get($texture, handle);
}

/* \brief set texture scale to material */
GLHCKAPI void glhckMaterialTextureScale(const glhckHandle handle, const kmVec2 *scale)
{
   set($textureScale, handle, scale);
}

/* \brief set texture scale to material (with kmScalar) */
GLHCKAPI void glhckMaterialTextureScalef(const glhckHandle handle, const kmScalar x, const kmScalar y)
{
   const kmVec2 scale = { x, y };
   glhckMaterialTextureScale(handle, &scale);
}

/* \brief get texture scale from material */
GLHCKAPI const kmVec2* glhckMaterialGetTextureScale(const glhckHandle handle)
{
   return get($textureScale, handle);
}

/* \brief set texture offset to material */
GLHCKAPI void glhckMaterialTextureOffset(const glhckHandle handle, const kmVec2 *offset)
{
   set($textureOffset, handle, offset);
}

/* \brief set texture offset to material (with kmScalar) */
GLHCKAPI void glhckMaterialTextureOffsetf(const glhckHandle handle, const kmScalar x, const kmScalar y)
{
   const kmVec2 offset = { x, y };
   glhckMaterialTextureOffset(handle, &offset);
}

/* \brief get texture offset from material */
GLHCKAPI const kmVec2* glhckMaterialGetTextureOffset(const glhckHandle handle)
{
   return get($textureOffset, handle);
}

/* \brief set texture rotation on material */
GLHCKAPI void glhckMaterialTextureRotationf(const glhckHandle handle, const kmScalar degrees)
{
   set($textureRotation, handle, &degrees);
}

/* \brief get texture rotation on material */
GLHCKAPI kmScalar glhckMaterialGetTextureRotation(const glhckHandle handle)
{
   return *(kmScalar*)get($textureRotation, handle);
}

/* \brief transform texture using rect and rotation */
GLHCKAPI void glhckMaterialTextureTransform(const glhckHandle handle, const glhckRect *transformed, const short degrees)
{
   if (kmAlmostEqual(transformed->w, 0.f) || kmAlmostEqual(transformed->h, 0.f))
      return;

   glhckMaterialTextureOffsetf(handle, transformed->x, transformed->y);
   glhckMaterialTextureScalef(handle, transformed->w, transformed->h);
   glhckMaterialTextureRotationf(handle, degrees);
}

/* \brief set material's shininess */
GLHCKAPI void glhckMaterialShininess(const glhckHandle handle, const kmScalar shininess)
{
   set($shininess, handle, &shininess);
}

/* \brief get material's shininess */
GLHCKAPI kmScalar glhckMaterialGetShininess(const glhckHandle handle)
{
   return *(kmScalar*)get($shininess, handle);
}

/* \brief set material's blending modes */
GLHCKAPI void glhckMaterialBlendFunc(const glhckHandle handle, const glhckBlendingMode blenda, const glhckBlendingMode blendb)
{
   struct blend *modes = (struct blend*)get($blend, handle);
   modes->blenda = blenda;
   modes->blendb = blendb;
}

/* \brief get material's blending modes */
GLHCKAPI void glhckMaterialGetBlendFunc(const glhckHandle handle, glhckBlendingMode *outBlenda, glhckBlendingMode *outBlendb)
{
   if (outBlenda) *outBlenda = ((const struct blend*)get($blend, handle))->blenda;
   if (outBlendb) *outBlendb = ((const struct blend*)get($blend, handle))->blendb;
}

/* \brief set material's ambient color */
GLHCKAPI void glhckMaterialAmbient(const glhckHandle handle, const glhckColor ambient)
{
   set($ambient, handle, &ambient);
}

/* \brief set material's ambient color (with chars) */
GLHCKAPI void glhckMaterialAmibentu(const glhckHandle handle, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a)
{
   glhckMaterialAmbient(handle, (a | (b << 8) | (g << 16) | (r << 24)));
}

/* \brief get material's ambient color */
GLHCKAPI glhckColor glhckMaterialGetAmbient(const glhckHandle handle)
{
   return *(glhckColor*)get($ambient, handle);
}

GLHCKAPI void glhckMaterialDiffuse(const glhckHandle handle, const glhckColor diffuse)
{
   set($diffuse, handle, &diffuse);
}

GLHCKAPI void glhckMaterialDiffuseu(const glhckHandle handle, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a)
{
   glhckMaterialDiffuse(handle, (a | (b << 8) | (g << 16) | (r << 24)));
}

GLHCKAPI glhckColor glhckMaterialGetDiffuse(const glhckHandle handle)
{
   return *(glhckColor*)get($diffuse, handle);
}

GLHCKAPI void glhckMaterialEmissive(const glhckHandle handle, const glhckColor emissive)
{
   set($emissive, handle, &emissive);
}

GLHCKAPI void glhckMaterialEmissiveu(const glhckHandle handle, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a)
{
   glhckMaterialEmissive(handle, (a | (b << 8) | (g << 16) | (r << 24)));
}

GLHCKAPI glhckColor glhckMaterialGetEmissive(const glhckHandle handle)
{
   return *(glhckColor*)get($emissive, handle);
}

GLHCKAPI void glhckMaterialSpecular(const glhckHandle handle, const glhckColor specular)
{
   set($specular, handle, &specular);
}

GLHCKAPI void glhckMaterialSpecularu(const glhckHandle handle, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a)
{
   glhckMaterialSpecular(handle, (a | (b << 8) | (g << 16) | (r << 24)));
}

GLHCKAPI glhckColor glhckMaterialGetSpecular(const glhckHandle handle)
{
   return *(glhckColor*)get($specular, handle);
}

/* vim: set ts=8 sw=3 tw=0 :*/
