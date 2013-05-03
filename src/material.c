#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_MATERIAL

/* \brief allocate new material object */
GLHCKAPI glhckMaterial* glhckMaterialNew(glhckTexture *texture)
{
   glhckMaterial *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckMaterial))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* default material settings */
   glhckMaterialTextureScalef(object, 1, 1);
   glhckMaterialDiffuseb(object, 255, 255, 255, 255);
   glhckMaterialOptions(object, GLHCK_MATERIAL_LIGHTING);

   /* assign initial texture */
   if (texture) glhckMaterialTexture(object, texture);

   /* insert to world */
   _glhckWorldInsert(material, object, glhckMaterial*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference material object */
GLHCKAPI glhckMaterial* glhckMaterialRef(glhckMaterial *object)
{
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free material object */
GLHCKAPI unsigned int glhckMaterialFree(glhckMaterial *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free texture */
   glhckMaterialTexture(object, NULL);

   /* free shader */
   glhckMaterialShader(object, NULL);

   /* remove from world */
   _glhckWorldRemove(material, object, glhckMaterial*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set options assigned to material */
GLHCKAPI void glhckMaterialOptions(glhckMaterial *object, unsigned int flags)
{
   CALL(1, "%p, %u", object, flags);
   assert(object);
   object->flags = flags;
}

/* \brief get options assigned to material */
GLHCKAPI unsigned int glhckMaterialGetOptions(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%u", object->flags);
   return object->flags;
}

/* \brief set shader to material */
GLHCKAPI void glhckMaterialShader(glhckMaterial *object, glhckShader *shader)
{
   CALL(1, "%p, %p", object, shader);
   assert(object);
   IFDO(glhckShaderFree, object->shader);
   object->shader = (shader?glhckShaderRef(shader):NULL);
}

/* \brief get assigned shader from material */
GLHCKAPI glhckShader* glhckMaterialGetShader(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%p", object->shader);
   return object->shader;
}

/* \brief set texture to material */
GLHCKAPI void glhckMaterialTexture(glhckMaterial *object, glhckTexture *texture)
{
   CALL(1, "%p, %p", object, texture);
   assert(object);

   /* already applied */
   if (object->texture == texture)
      return;

   /* free old texture */
   IFDO(glhckTextureFree, object->texture);

   /* assign new texture */
   if (texture) {
      object->texture = glhckTextureRef(texture);
      if (texture->importFlags & GLHCK_TEXTURE_IMPORT_TEXT) {
         glhckMaterialBlendFunc(object, GLHCK_ONE, GLHCK_ONE_MINUS_SRC_ALPHA);
      } else if (texture->importFlags & GLHCK_TEXTURE_IMPORT_ALPHA) {
         glhckMaterialBlendFunc(object, GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);
      } else {
         glhckMaterialBlendFunc(object, 0, 0);
      }
   }
}

/* \brief get assigned texture from material */
GLHCKAPI glhckTexture* glhckMaterialGetTexture(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%p", object->texture);
   return object->texture;
}

/* \brief set texture scale to material */
GLHCKAPI void glhckMaterialTextureScale(glhckMaterial *object, const kmVec2 *scale)
{
   CALL(1, "%p, "VEC2S, object, VEC2(scale));
   assert(object && scale);
   memcpy(&object->textureScale, scale, sizeof(kmVec2));
}

/* \brief set texture scale to material (with kmScalar) */
GLHCKAPI void glhckMaterialTextureScalef(glhckMaterial *object, kmScalar x, kmScalar y)
{
   const kmVec2 scale = {x, y};
   glhckMaterialTextureScale(object, &scale);
}

/* \brief get texture scale from material */
GLHCKAPI const kmVec2* glhckMaterialGetTextureScale(glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, VEC2S, VEC2(&object->textureScale));
   return &object->textureScale;
}

/* \brief set texture offset to material */
GLHCKAPI void glhckMaterialTextureOffset(glhckMaterial *object, const kmVec2 *offset)
{
   CALL(1, "%p, "VEC2S, object, VEC2(offset));
   assert(object && offset);
   memcpy(&object->textureOffset, offset, sizeof(kmVec2));
}

/* \brief set texture offset to material (with kmScalar) */
GLHCKAPI void glhckMaterialTextureOffsetf(glhckMaterial *object, kmScalar x, kmScalar y)
{
   const kmVec2 offset = {x, y};
   glhckMaterialTextureOffset(object, &offset);
}

/* \brief get texture offset from material */
GLHCKAPI const kmVec2* glhckMaterialGetTextureOffset(glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, VEC2S, VEC2(&object->textureOffset));
   return &object->textureOffset;
}

/* \brief set material's shininess */
GLHCKAPI void glhckMaterialShininess(glhckMaterial *object, float shininess)
{
   CALL(1, "%p, %f", object, shininess);
   assert(object);
   object->shininess = shininess;
}

/* \brief get material's shininess */
GLHCKAPI float glhckMaterialGetShininess(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%f", object->shininess);
   return object->shininess;
}

/* \brief set material's blending modes */
GLHCKAPI void glhckMaterialBlendFunc(glhckMaterial *object, glhckBlendingMode blenda, glhckBlendingMode blendb)
{
   CALL(1, "%p, %u, %u", object, blenda, blendb);
   assert(object);
   object->blenda = blenda;
   object->blendb = blendb;
}

/* \brief get material's blending modes */
GLHCKAPI void glhckMaterialGetBlendFunc(const glhckMaterial *object, glhckBlendingMode *blenda, glhckBlendingMode *blendb)
{
   CALL(1, "%p, %p, %p", object, blenda, blendb);
   assert(object);
   if (blenda) *blenda = object->blenda;
   if (blendb) *blendb = object->blendb;
}

/* \brief set material's ambient color */
GLHCKAPI void glhckMaterialAmbient(glhckMaterial *object, const glhckColorb *ambient)
{
   CALL(1, "%p, "COLBS, object, COLB(ambient));
   assert(object && ambient);
   memcpy(&object->ambient, ambient, sizeof(glhckColorb));
}

/* \brief set material's ambient color (with chars) */
GLHCKAPI void glhckMaterialAmibentb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   const glhckColorb ambient = {r, g, b, a};
   glhckMaterialAmbient(object, &ambient);
}

/* \brief get material's ambient color */
GLHCKAPI const glhckColorb* glhckMaterialGetAmbient(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, COLBS, COLB(&object->ambient));
   return &object->ambient;
}

GLHCKAPI void glhckMaterialDiffuse(glhckMaterial *object, const glhckColorb *diffuse)
{
   CALL(1, "%p, "COLBS, object, COLB(diffuse));
   assert(object && diffuse);
   memcpy(&object->diffuse, diffuse, sizeof(glhckColorb));
}

GLHCKAPI void glhckMaterialDiffuseb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   const glhckColorb diffuse = {r, g, b, a};
   glhckMaterialDiffuse(object, &diffuse);
}

GLHCKAPI const glhckColorb* glhckMaterialGetDiffuse(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, COLBS, COLB(&object->diffuse));
   return &object->diffuse;
}

GLHCKAPI void glhckMaterialEmissive(glhckMaterial *object, const glhckColorb *emissive)
{
   CALL(1, "%p, "COLBS, object, COLB(emissive));
   assert(object && emissive);
   memcpy(&object->emissive, emissive, sizeof(glhckColorb));
}

GLHCKAPI void glhckMaterialEmissiveb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   const glhckColorb emissive = {r, g, b, a};
   glhckMaterialEmissive(object, &emissive);
}

GLHCKAPI const glhckColorb* glhckMaterialGetEmissive(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, COLBS, COLB(&object->emissive));
   return &object->emissive;
}

GLHCKAPI void glhckMaterialSpecular(glhckMaterial *object, const glhckColorb *specular)
{
   CALL(1, "%p, "COLBS, object, COLB(specular));
   assert(object && specular);
   memcpy(&object->specular, specular, sizeof(glhckColorb));
}

GLHCKAPI void glhckMaterialSpecularb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   const glhckColorb specular = {r, g, b, a};
   glhckMaterialSpecular(object, &specular);
}

GLHCKAPI const glhckColorb* glhckMaterialGetSpecular(const glhckMaterial *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, COLBS, COLB(&object->specular));
   return &object->specular;
}

/* vim: set ts=8 sw=3 tw=0 :*/
