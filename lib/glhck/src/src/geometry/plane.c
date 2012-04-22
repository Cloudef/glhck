#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* shared vertices */
static const glhckImportVertexData vertices[] = {
   {
      {  1,  1, 0 },      /* vertices */
      {  0,  0, 0 },      /* normals */
      {  1,  1 },         /* uv coord */
      { 0, 0, 0, 0 }      /* color */
   },{
      { -1,  1, 0 },
      {  0,  0, 0 },
      {  0,  1 },
      { 0, 0, 0, 0 }
   },{
      {  1, -1, 0 },
      {  0,  0, 0 },
      {  1,  0 },
      { 0, 0, 0, 0 }
   },{
      { -1, -1, 0 },
      {  0,  0, 0 },
      {  0,  0 },
      { 0, 0, 0, 0 }
   }
};

/* \brief create new plane object */
GLHCKAPI _glhckObject* glhckPlaneNew(size_t size)
{
   _glhckObject *object;
   CALL("%d", size);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertexData(object,
            LENGTH(vertices), &vertices[0]) != RETURN_OK)
      goto fail;

   /* assigning indices would be waste
    * on the plane geometry */

   /* scale the cube */
   glhckObjectScalef(object, size, size, size);

   RET("%p", object);
   return object;

fail:
   if (object) glhckObjectFree(object);
   RET("%p", NULL);
   return NULL;
}

/* \brief create new sprite */
GLHCKAPI _glhckObject* glhckSpriteNew(const char *file, size_t size)
{
   CALL("%s, %zu", file, size);

   /* TODO: create plane and texture it
    * needs material system */

fail:
   RET("%p", NULL);
   return NULL;
}
