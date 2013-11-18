#include "../internal.h"
#include "../import/import.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new object from supported model files */
GLHCKAPI glhckObject* glhckModelNew(const char *file, kmScalar size, const glhckImportModelParameters *importParams)
{
   glhckGeometryIndexType itype;
   glhckGeometryVertexType vtype;
   glhckGetGlobalPrecision(&itype, &vtype);
   return glhckModelNewEx(file, size, importParams, itype, vtype);
}

/* \brief create new object from supported model files
 * you can specify the index and vertex precision here */
GLHCKAPI glhckObject* glhckModelNewEx(const char *file, kmScalar size, const glhckImportModelParameters *importParams, glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   glhckObject *object;
   CALL(0, "%s, %f, %p, %d, %d", file, size, importParams, itype, vtype);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* import model */
   if (_glhckImportModel(object, file, importParams, itype, vtype) != RETURN_OK)
      goto fail;

   /* scale the cube */
   glhckObjectScalef(object, size, size, size);

   /* set object's filename */
   _glhckObjectFile(object, file);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
