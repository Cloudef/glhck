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
GLHCKAPI _glhckObject* glhckSpriteNew(const char *file, size_t size, unsigned int flags)
{
   _glhckObject *object;
   _glhckTexture *texture;
   CALL(0, "%s, %zu", file, size);

   /* load texture */
   if (!(texture = glhckTextureNew(file, flags)))
      goto fail;

   /* make plane */
   if (!(object = glhckPlaneNew(1)))
      goto fail;

   /* scale keeping aspect ratio */
   glhckObjectScalef(object,
         (float)size/texture->width,
         (float)size/texture->height, 1);

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
