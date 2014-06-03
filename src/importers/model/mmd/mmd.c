#include "importers/importer.h"

#include <stdio.h>
#include <stdlib.h>

#include "trace.h"
#include "pool/pool.h"
#include "mmd/mmd.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* FIXME: need to change libmmd to support memory loading */

/* \brief check if file is a MikuMikuDance PMD file */
static int check(chckBuffer *buffer)
{
   FILE *f = NULL;
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
static int import(chckBuffer *buffer, glhckImportModelStruct *import)
{
   FILE *f = NULL;
   mmd_data *mmd = NULL;
   glhckImportVertexData *vertexData = NULL;
   glhckImportIndexData *indexData = NULL;
   glhckImportModelMaterial *materials = NULL;
   CALL(0, "%p, %p", buffer, import);

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

   if (mmd->header.name)
      DEBUG(GLHCK_DBG_CRAP, "%s\n", mmd->header.name);

   if (mmd->header.comment) printf("%s\n\n", mmd->header.comment);

   if (!(vertexData = calloc(mmd->num_vertices, sizeof(glhckImportVertexData))))
      goto mmd_no_memory;

   if (!(indexData = calloc(mmd->num_indices, sizeof(glhckImportIndexData))))
      goto mmd_no_memory;

   if (!(materials = calloc(mmd->num_materials, sizeof(glhckImportModelMaterial))))
      goto mmd_no_memory;

   /* add all textures to atlas packer */
   for (unsigned int i = 0; i < mmd->num_materials; ++i) {
      if (!mmd->materials[i].texture)
         continue;

      /* FIXME: read other stuff too */
      materials[i].diffuseTexture = mmd->materials[i].texture;
   }

   mmd_free(mmd);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

mmd_import_fail:
   DEBUG(GLHCK_DBG_ERROR, "MMD importing failed.");
   goto fail;
mmd_no_memory:
   DEBUG(GLHCK_DBG_ERROR, "MMD not enough memory.");
fail:
   IFDO(mmd_free, mmd);
   IFDO(free, materials);
   IFDO(free, vertexData);
   IFDO(free, indexData);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   info->check = check;
   info->modelImport = import;

   RET(0, "%s", "glhck-importer-mmd");
   return "glhck-importer-mmd";
}

/* vim: set ts=8 sw=3 tw=0 :*/
