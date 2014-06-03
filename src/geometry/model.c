#include <glhck/glhck.h>
#include <glhck/import.h>

#include <stdlib.h>

#include "trace.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new object from supported model files */
GLHCKAPI glhckHandle glhckModelNew(const char *file, const kmScalar size, const glhckPostProcessModelParameters *postParams)
{
   glhckIndexType itype = GLHCK_IDX_AUTO;
   glhckVertexType vtype = GLHCK_VTX_AUTO;
   // glhckGetGlobalPrecision(&itype, &vtype);
   return glhckModelNewEx(file, size, postParams, itype, vtype);
}

/* \brief create new object from supported model files you can specify the index and vertex precision here */
GLHCKAPI glhckHandle glhckModelNewEx(const char *file, const kmScalar size, const glhckPostProcessModelParameters *postParams, const glhckIndexType itype, const glhckVertexType vtype)
{
   CALL(0, "%s, %f, %p, %u, %u", file, size, postParams, itype, vtype);

   glhckHandle handle = 0, current = 0;
   glhckImportModelStruct *import = NULL;
   if (!(import = glhckImportModelFile(file)))
      goto fail;

   for (size_t i = 0; i < import->meshCount; ++i) {
      if (!(current = glhckObjectNew()))
         goto fail;

      glhckObjectInsertVertices(current, vtype, import->meshes[i].vertexData, import->meshes[i].vertexCount);
      glhckObjectInsertIndices(current, itype, import->meshes[i].indexData, import->meshes[i].indexCount);

      const glhckHandle geometry = glhckObjectGetGeometry(current);
      glhckGeometry *data = glhckGeometryGetStruct(geometry);
      data->type = import->meshes[i].geometryType;

      glhckObjectScalef(current, size, size, size);

      if (!handle) {
         handle = current;
         current = 0;
      } else {
         glhckObjectAddChild(handle, current);
      }
   }

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   IFDO(glhckHandleRelease, current);
   IFDO(free, import);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
