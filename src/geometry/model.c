#include "../internal.h"
#include "../import/import.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new object from supported model files */
GLHCKAPI glhckObject* glhckModelNew(const char *file, kmScalar size, unsigned int importFlags)
{
   glhckGeometryIndexType itype;
   glhckGeometryVertexType vtype;
   glhckGetGlobalPrecision(&itype, &vtype);
   return glhckModelNewEx(file, size, importFlags, itype, vtype);
}

/* \brief create new object from supported model files
 * you can specify the index and vertex precision here */
GLHCKAPI glhckObject* glhckModelNewEx(const char *file, kmScalar size, unsigned int importFlags, glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   _glhckObject *object;
   CALL(0, "%s, %f, %u, %d, %d", file, size, importFlags, itype, vtype);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* import model */
   if (_glhckImportModel(object, file, importFlags, itype, vtype) != RETURN_OK)
      goto fail;

   /* scale the cube */
   glhckObjectScalef(object, size, size, size);

   /* set object's filename */
   _glhckObjectSetFile(object, file);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
