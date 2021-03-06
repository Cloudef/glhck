#include "../internal.h"
#include "import.h"
#include <stdio.h>
#include <mmd.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* FIXME: add generic loading
 * currently all MMD loading is GLHCK_MODEL_JOIN */

/* \brief check if file is a MikuMikuDance PMD file */
int _glhckFormatPMD(const char *file)
{
   FILE *f;
   mmd_data *mmd = NULL;
   CALL(0, "%s", file);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   if (!(mmd = mmd_new(f)))
      goto fail;

   if (mmd_read_header(mmd) != 0)
      goto fail;

   /* close file */
   NULLDO(mmd_free, mmd);
   NULLDO(fclose, f);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(mmd_free, mmd);
   IFDO(fclose, f);
   return RETURN_FAIL;
}

/* \brief import MikuMikuDance PMD file */
int _glhckImportPMD(_glhckObject *object, const char *file, const glhckImportModelParameters *params,
      unsigned char itype, unsigned char vtype)
{
   FILE *f;
   char *texturePath;
   mmd_data *mmd = NULL;
   glhckAtlas *atlas = NULL;
   glhckMaterial *material = NULL;
   glhckTexture *texture = NULL, **textureList = NULL;
   glhckImportVertexData *vertexData = NULL;
   glhckImportIndexData *indices = NULL, *stripIndices = NULL;
   unsigned int geometryType = GLHCK_TRIANGLE_STRIP;
   unsigned int i, i2, ix, start, numFaces, numIndices = 0;
   CALL(0, "%p, %s, %p", object, file, params);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   if (!(mmd = mmd_new(f)))
      goto mmd_no_memory;

   if (mmd_read_header(mmd) != 0)
      goto mmd_import_fail;

   if (mmd_read_vertex_data(mmd) != 0)
      goto mmd_import_fail;

   if (mmd_read_index_data(mmd) != 0)
      goto mmd_import_fail;

   if (mmd_read_material_data(mmd) != 0)
      goto mmd_import_fail;

   /* close file */
   NULLDO(fclose, f);

   if (mmd->header.name) {
      DEBUG(GLHCK_DBG_CRAP, "%s\n", mmd->header.name);
   }
   if (mmd->header.comment) printf("%s\n\n", mmd->header.comment);

   if (!(vertexData = _glhckCalloc(mmd->num_vertices, sizeof(glhckImportVertexData))))
      goto mmd_no_memory;

   if (!(indices = _glhckMalloc(mmd->num_indices * sizeof(unsigned int))))
      goto mmd_no_memory;

   if (!(textureList = _glhckCalloc(mmd->num_materials, sizeof(_glhckTexture*))))
      goto mmd_no_memory;

   if (!(atlas = glhckAtlasNew()))
      goto mmd_no_memory;

   /* add all textures to atlas packer */
   for (i = 0; i < mmd->num_materials; ++i) {
      if (!mmd->materials[i].texture) continue;

      if (!(texturePath = _glhckImportTexturePath(mmd->materials[i].texture, file)))
         continue;

      if ((texture = glhckTextureNewFromFile(texturePath, NULL, NULL))) {
         glhckAtlasInsertTexture(atlas, texture);
         glhckTextureFree(texture);
         textureList[i] = texture;
      }

      _glhckFree(texturePath);
   }

   for (i = 0; i < mmd->num_materials && !textureList[i]; ++i);
   if (i >= mmd->num_materials) {
      /* no textures found */
      NULLDO(glhckAtlasFree, atlas);
      NULLDO(_glhckFree, textureList);
   } else {
      /* pack textures */
      if (glhckAtlasPack(atlas, GLHCK_RGBA, 1, 0, glhckTextureDefaultParameters()) != RETURN_OK)
         goto fail;
   }

   /* assign data */
   for (i = 0, start = 0; i < mmd->num_materials; ++i, start += numFaces) {
      numFaces = mmd->materials[i].face;
      for (i2 = start; i2 < start + numFaces; ++i2) {
         ix = mmd->indices[i2];

         /* vertices */
         vertexData[ix].vertex.x = mmd->vertices[ix*3+0];
         vertexData[ix].vertex.y = mmd->vertices[ix*3+1];
         vertexData[ix].vertex.z = mmd->vertices[ix*3+2];

         /* normals */
         vertexData[ix].normal.x = mmd->normals[ix*3+0];
         vertexData[ix].normal.y = mmd->normals[ix*3+1];
         vertexData[ix].normal.z = mmd->normals[ix*3+2];

         /* texture coords */
         vertexData[ix].coord.x = mmd->coords[ix*2+0];
         vertexData[ix].coord.y = mmd->coords[ix*2+1] * -1;

         /* fix coords */
         if (vertexData[ix].coord.x < 0.0f)
            vertexData[ix].coord.x += 1.0f;
         if (vertexData[ix].coord.y < 0.0f)
            vertexData[ix].coord.y += 1.0f;

         /* if there is packed texture */
         if (atlas && textureList[i]) {
            kmVec2 coord;
            coord.x = vertexData[ix].coord.x;
            coord.y = vertexData[ix].coord.y;
            glhckAtlasTransformCoordinates(atlas, textureList[i], &coord, &coord);
            vertexData[ix].coord.x = coord.x;
            vertexData[ix].coord.y = coord.y;
         }

         indices[i2] = ix;
      }
   }

   if (atlas) {
      if (!(material = glhckMaterialNew(glhckAtlasGetTexture(atlas))))
         goto mmd_no_memory;

      glhckObjectMaterial(object, material);
      NULLDO(glhckMaterialFree, material);
      NULLDO(glhckAtlasFree, atlas);
      NULLDO(_glhckFree, textureList);
   }

   /* triangle strip geometry */
   if (!(stripIndices = _glhckTriStrip(indices, mmd->num_indices, &numIndices))) {
      /* failed, use non stripped geometry */
      geometryType   = GLHCK_TRIANGLES;
      numIndices    = mmd->num_indices;
      stripIndices  = indices;
   } else NULLDO(_glhckFree, indices);

   /* set geometry */
   glhckObjectInsertIndices(object, itype, stripIndices, numIndices);
   glhckObjectInsertVertices(object, vtype, vertexData, mmd->num_vertices);
   object->geometry->type = geometryType;

   /* finish */
   NULLDO(_glhckFree, vertexData);
   NULLDO(_glhckFree, stripIndices);
   NULLDO(mmd_free, mmd);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
mmd_import_fail:
   DEBUG(GLHCK_DBG_ERROR, "MMD importing failed.");
   goto fail;
mmd_no_memory:
   DEBUG(GLHCK_DBG_ERROR, "MMD not enough memory.");
fail:
   IFDO(fclose, f);
   IFDO(glhckAtlasFree, atlas);
   IFDO(mmd_free, mmd);
   IFDO(_glhckFree, textureList);
   IFDO(_glhckFree, vertexData);
   IFDO(_glhckFree, indices);
   IFDO(_glhckFree, stripIndices);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
