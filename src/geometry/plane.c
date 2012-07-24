#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* \brief create new plane object */
GLHCKAPI _glhckObject* glhckPlaneNew(kmScalar size)
{
   _glhckObject *object;
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

   CALL(0, "%d", size);

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

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief create new sprite */
GLHCKAPI _glhckObject* glhckSpriteNew(const char *file, kmScalar size,
      unsigned int flags)
{
   _glhckObject *object;
   _glhckTexture *texture;
   kmScalar w, h;
   CALL(0, "%s, %zu", file, size);

   /* load texture */
   if (!(texture = glhckTextureNew(file, flags)))
      goto fail;

   w = 2.0f-(1.0f/(kmScalar)texture->width);
   h = 2.0f-(1.0f/(kmScalar)texture->height);
   const glhckImportVertexData vertices[] = {
      {
         {  w,  h, 0 },
         {  0,  0, 0 },      /* normals */
         {  1,  1 },         /* uv coord */
         { 0, 0, 0, 0 }      /* color */
      },{
         { -w,  h, 0 },
         {  0,  0, 0 },
         {  0,  1 },
         { 0, 0, 0, 0 }
      },{
         {  w, -h, 0 },
         {  0,  0, 0 },
         {  1,  0 },
         { 0, 0, 0, 0 }
      },{
         { -w, -h, 0 },
         {  0,  0, 0 },
         {  0,  0 },
         { 0, 0, 0, 0 }
      }
   };

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertexData(object,
            LENGTH(vertices), &vertices[0]) != RETURN_OK)
      goto fail;

   /* scale keeping aspect ratio */
   glhckObjectScalef(object, size, size, size);

   /* pass reference to object, and free this */
   glhckObjectSetTexture(object, texture);
   glhckTextureFree(texture);

   /* set filename of object */
   _glhckObjectSetFile(object, file);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckTextureFree, texture);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
