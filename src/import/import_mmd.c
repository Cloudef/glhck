#include "../internal.h"
#include "import.h"
#include <stdio.h>
#include "mmd.h"

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* \brief check if file is a MikuMikuDance PMD file */
int _glhckFormatPMD(const char *file)
{
   FILE *f;
   mmd_header header;
   CALL(0, "%s", file);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   if (mmd_read_header(f, &header) != 0)
      goto fail;

   /* close file */
   NULLDO(fclose, f);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(fclose, f);
   return RETURN_FAIL;
}

/* \brief import MikuMikuDance PMD file */
int _glhckImportPMD(_glhckObject *object, const char *file, int animated,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   FILE *f;
   char *texturePath;
   size_t i, i2, ix, start, num_faces, num_indices;
   mmd_header header;
   mmd_data *mmd = NULL;
   _glhckAtlas *atlas = NULL;
   _glhckTexture *texture = NULL, **textureList = NULL;
   glhckImportVertexData *vertexData = NULL;
   unsigned int *indices = NULL, *strip_indices = NULL;
   CALL(0, "%p, %s, %d", object, file, animated);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   if (mmd_read_header(f, &header) != 0)
      goto mmd_import_fail;

   if (!(mmd = mmd_new()))
      goto mmd_no_memory;

   if (mmd_read_vertex_data(f, mmd) != 0)
      goto mmd_import_fail;

   if (mmd_read_index_data(f, mmd) != 0)
      goto mmd_import_fail;

   if (mmd_read_material_data(f, mmd) != 0)
      goto mmd_import_fail;

   /* close file */
   NULLDO(fclose, f);

   DEBUG(GLHCK_DBG_CRAP, "V: %d", mmd->num_vertices);
   DEBUG(GLHCK_DBG_CRAP, "I: %d", mmd->num_indices);

   if (!(vertexData = _glhckMalloc(mmd->num_vertices * sizeof(glhckImportVertexData))))
      goto mmd_no_memory;

   if (!(indices = _glhckMalloc(mmd->num_indices * sizeof(unsigned int))))
      goto mmd_no_memory;

   if (!(textureList = _glhckMalloc(mmd->num_materials * sizeof(_glhckTexture*))))
      goto mmd_no_memory;

   if (!(atlas = glhckAtlasNew()))
      goto mmd_no_memory;

   /* init texture list */
   memset(textureList, 0, mmd->num_materials * sizeof(_glhckTexture*));

   /* add all textures to atlas packer */
   for (i = 0; i != mmd->num_materials; ++i) {
      if (!(texturePath = _glhckImportTexturePath(mmd->texture[i].file, file)))
         continue;

      if ((texture = glhckTextureNew(texturePath, 0))) {
         glhckAtlasInsertTexture(atlas, texture);
         glhckTextureFree(texture);
         textureList[i] = texture;
      }

      free(texturePath);
   }

   /* pack textures */
   if (glhckAtlasPack(atlas, 1, 0) != RETURN_OK)
      goto fail;

   /* init */
   memset(vertexData, 0, mmd->num_vertices * sizeof(glhckImportVertexData));

   /* assign data */
   for (i = 0, start = 0; i != mmd->num_materials; ++i) {
      num_faces = mmd->face[i];
      for (i2 = start; i2 != start + num_faces; ++i2) {
         ix = mmd->indices[i2];

         /* vertices */
         vertexData[ix].vertex.x = mmd->vertices[ix * 3 + 0];
         vertexData[ix].vertex.y = mmd->vertices[ix * 3 + 1];
         vertexData[ix].vertex.z = mmd->vertices[ix * 3 + 2];

         /* normals */
         vertexData[ix].normal.x = mmd->normals[ix * 3 + 0];
         vertexData[ix].normal.y = mmd->normals[ix * 3 + 1];
         vertexData[ix].normal.z = mmd->normals[ix * 3 + 2];

         /* texture coords */
         vertexData[ix].coord.x = mmd->coords[ix * 2 + 0];
         vertexData[ix].coord.y = mmd->coords[ix * 2 + 1];

         /* fix coords */
         if (vertexData[ix].coord.x < 0.0f)
            vertexData[ix].coord.x += 1;
         if (vertexData[ix].coord.y < 0.0f)
            vertexData[ix].coord.y += 1;

         /* if there is packed texture */
         if (textureList[i]) {
            kmVec2 coord;
            coord.x = vertexData[ix].coord.x;
            coord.y = vertexData[ix].coord.y;
            glhckAtlasTransformCoordinates(atlas, textureList[i],
                  &coord, &coord);
            vertexData[ix].coord.x = coord.x;
            vertexData[ix].coord.y = coord.y;
         }

         indices[i2] = ix;
      }
      start += num_faces;
   }

   /* assign texture ot object */
   glhckObjectSetTexture(object, glhckAtlasGetTexture(atlas));

   /* we don't need atlas packer anymore */
   NULLDO(glhckAtlasFree, atlas);
   NULLDO(_glhckFree, textureList);

   /* triangle strip geometry */
   if (!(strip_indices = _glhckTriStrip(indices, mmd->num_indices, &num_indices))) {
      /* failed, use non stripped geometry */
      object->geometry->type  = GLHCK_TRIANGLES;
      num_indices             = mmd->num_indices;
      strip_indices           = indices;
   } else NULLDO(_glhckFree, indices);

   /* set geometry */
   glhckObjectInsertIndices(object, num_indices, itype, strip_indices);
   glhckObjectInsertVertices(object, mmd->num_vertices, vtype, vertexData);

   /* finish */
   NULLDO(_glhckFree, vertexData);
   NULLDO(_glhckFree, strip_indices);
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
   IFDO(_glhckFree, strip_indices);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
