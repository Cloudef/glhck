#include "../internal.h"
#include "import.h"
#include "openctm.h"
#include <stdio.h>

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

#ifdef NDEBUG
#  define CTM_CALL(x) x
#else
#  define CTM_CALL(ctx, x) x; CTM_ERROR(ctx, __LINE__, __func__, __STRING(x));
static inline void CTM_ERROR(CTMcontext context, unsigned int line, const char *func, const char *ctmfunc)
{
   CTMenum error;
   if ((error = ctmGetError(context)) != CTM_NONE)
      DEBUG(GLHCK_DBG_ERROR, "OCTM @%d:%-20s %-20s >> %s",
            line, func, ctmfunc, ctmErrorString(error));
}
#endif

/* read callback */
static CTMuint readOCTMFile(void *buf, CTMuint toRead, void *f)
{
   return ((CTMuint)fread(buf, 1, (size_t)toRead, (FILE *)f));
}

/* \brief check if file is a OpenCTM file */
int _glhckFormatOpenCTM(const char *file)
{
   FILE *f;
   CTMcontext context = NULL;
   CALL(0, "%s", file);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* create import OpenCTM context */
   if (!(context = ctmNewContext(CTM_IMPORT)))
      goto fail;

   /* load OpenCTM file */
   ctmLoadCustom(context, (CTMreadfn)readOCTMFile, f);

   /* close file */
   NULLDO(fclose, f);

   if (ctmGetError(context) != CTM_NONE)
      goto fail;

   NULLDO(ctmFreeContext, context);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(fclose, f);
   IFDO(ctmFreeContext, context);
   return RETURN_FAIL;
}

/* \brief import OpenCTM file */
int _glhckImportOpenCTM(_glhckObject *object, const char *file, int animated,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   FILE *f;
   size_t num_indices;
   unsigned int i, ix, *strip_indices = NULL;
   CTMcontext context = NULL;
   CTMuint num_vertices, num_triangles, num_uvs, num_attribs;
   const CTMuint *indices;
   const CTMfloat *vertices = NULL, *normals = NULL, *coords = NULL, *colors = NULL;
   const char *attribName, *comment, *textureFilename;
   char *texturePath;
   glhckImportVertexData *vertexData = NULL;
   _glhckTexture *texture;
   unsigned int geometryType = GLHCK_TRIANGLE_STRIP;
   CALL(0, "%p, %s, %d", object, file, animated);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* create import OpenCTM context */
   if (!(context = ctmNewContext(CTM_IMPORT)))
      goto fail;

   /* load OpenCTM file */
   CTM_CALL(context, ctmLoadCustom(context, (CTMreadfn)readOCTMFile, f));

   /* close file */
   NULLDO(fclose, f);

   /* read geometry data */
   vertices      = CTM_CALL(context, ctmGetFloatArray(context, CTM_VERTICES));
   indices       = CTM_CALL(context, ctmGetIntegerArray(context, CTM_INDICES));
   num_vertices  = CTM_CALL(context, ctmGetInteger(context, CTM_VERTEX_COUNT));
   num_triangles = CTM_CALL(context, ctmGetInteger(context, CTM_TRIANGLE_COUNT));
   num_uvs       = CTM_CALL(context, ctmGetInteger(context, CTM_UV_MAP_COUNT));
   num_attribs   = CTM_CALL(context, ctmGetInteger(context, CTM_ATTRIB_MAP_COUNT));

   /* indices count now */
   num_triangles *= 3;

   if (ctmGetInteger(context, CTM_HAS_NORMALS) == CTM_TRUE) {
      normals = CTM_CALL(context, ctmGetFloatArray(context, CTM_NORMALS));
   }

   if (num_uvs) {
      textureFilename = CTM_CALL(context, ctmGetUVMapString(context, CTM_UV_MAP_1, CTM_FILE_NAME));
      if (!textureFilename) {
         textureFilename = CTM_CALL(context, ctmGetUVMapString(context, CTM_UV_MAP_1, CTM_NAME));
      }

      if ((texturePath = _glhckImportTexturePath(textureFilename, file))) {
         if ((texture = glhckTextureNew(texturePath, GLHCK_TEXTURE_DEFAULTS))) {
            coords = CTM_CALL(context, ctmGetFloatArray(context, CTM_UV_MAP_1));
            glhckObjectSetTexture(object, texture);
            glhckTextureFree(texture);
         }
         free(texturePath);
      }
   }

   for (i = 0; i != num_attribs; ++i) {
      attribName = ctmGetAttribMapString(context, CTM_ATTRIB_MAP_1 + i, CTM_NAME);
      if (!strcmp(attribName, "Color")) {
         colors = CTM_CALL(context, ctmGetFloatArray(context, CTM_ATTRIB_MAP_1 + i));
      }
   }

   /* output comment to stdout */
   comment = CTM_CALL(context, ctmGetString(context, CTM_FILE_COMMENT));
   if (comment) DEBUG(GLHCK_DBG_CRAP, "%s", comment);

   /* process vertex data to import format */
   if (!(vertexData = _glhckMalloc(num_vertices * sizeof(glhckImportVertexData))))
      goto fail;

   /* init */
   memset(vertexData, 0, num_vertices * sizeof(glhckImportVertexData));

   for (i = 0; i != num_triangles; ++i) {
      ix = indices[i];

      vertexData[ix].vertex.x = vertices[ix*3+0];
      vertexData[ix].vertex.y = vertices[ix*3+1];
      vertexData[ix].vertex.z = vertices[ix*3+2];

      if (normals) {
         vertexData[ix].normal.x = normals[ix*3+0];
         vertexData[ix].normal.y = normals[ix*3+1];
         vertexData[ix].normal.z = normals[ix*3+2];
      }

      if (coords) {
         vertexData[ix].coord.x = coords[ix*2+0];
         vertexData[ix].coord.y = coords[ix*2+1];
      }

      if (colors) {
         vertexData[ix].color.r = colors[ix*4+0]*255.0f;
         vertexData[ix].color.g = colors[ix*4+1]*255.0f;
         vertexData[ix].color.b = colors[ix*4+2]*255.0f;
         vertexData[ix].color.a = colors[ix*4+3]*255.0f;
      }
   }

   /* triangle strip geometry */
   if (!(strip_indices = _glhckTriStrip(indices, num_triangles, &num_indices))) {
      /* failed, use non stripped geometry */
      geometryType = GLHCK_TRIANGLES;
      num_indices  = num_triangles;
   }

   /* this object has colors */
   if (colors) object->material.flags |= GLHCK_MATERIAL_COLOR;

   /* set geometry */
   glhckObjectInsertIndices(object, num_indices, itype, strip_indices?strip_indices:indices);
   glhckObjectInsertVertices(object, num_vertices, vtype, vertexData);
   object->geometry->type = geometryType;

   /* finish */
   IFDO(_glhckFree, strip_indices);
   NULLDO(_glhckFree, vertexData);
   NULLDO(ctmFreeContext, context);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
fail:
   IFDO(_glhckFree, strip_indices);
   IFDO(_glhckFree, vertexData);
   IFDO(fclose, f);
   IFDO(ctmFreeContext, context);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
