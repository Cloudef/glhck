#include "../internal.h"
#include "../import/import.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new cube object */
GLHCKAPI _glhckObject* glhckModelNew(const char *file, kmScalar size)
{
   _glhckObject *object;
   CALL(0, "%s, %f", file, size);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* import model */
   _glhckImportModel(object, file, 0);

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
