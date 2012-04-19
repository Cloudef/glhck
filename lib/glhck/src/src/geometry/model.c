#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new cube object */
GLHCKAPI _glhckObject* glhckModelNew(const char *path, size_t size)
{
   _glhckObject *object;
   CALL("%s, %d", path, size);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* import model */
   _glhckImportModel(object, path, 0);

   /* scale the cube */
   glhckObjectScalef(object, size, size, size);

   RET("%p", object);
   return object;

fail:
   if (object) glhckObjectFree(object);
   RET("%p", NULL);
   return NULL;
}
