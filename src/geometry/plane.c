#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* \brief create new plane object */
GLHCKAPI glhckObject* glhckPlaneNew(kmScalar width, kmScalar height)
{
   glhckGeometryVertexType vtype;
   glhckGetGlobalPrecision(NULL, &vtype);
   if (vtype == GLHCK_VERTEX_NONE) vtype = GLHCK_VERTEX_V2B;
   return glhckPlaneNewEx(width, height, GLHCK_INDEX_NONE, vtype);
}

/* \brief create new plane object (precision specify) */
GLHCKAPI glhckObject* glhckPlaneNewEx(kmScalar width, kmScalar height,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   glhckObject *object;
   CALL(0, "%f, %f, %d, %d", width, height, itype, vtype);

   static const glhckImportVertexData vertices[] = {
      {
         {  1,  1,  0 },     /* vertices */
         {  0,  0, -1 },     /* normals */
         {  1,  1 },         /* uv coord */
         { 0, 0, 0, 0 }      /* color */
      },{
         { -1,  1,  0 },
         {  0,  0, -1 },
         {  0,  1 },
         { 0, 0, 0, 0 }
      },{
         {  1, -1,  0 },
         {  0,  0, -1 },
         {  1,  0 },
         { 0, 0, 0, 0 }
      },{
         { -1, -1,  0 },
         {  0,  0, -1 },
         {  0,  0 },
         { 0, 0, 0, 0 }
      }
   };

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertices(object, vtype,
            &vertices[0], LENGTH(vertices)) != RETURN_OK)
      goto fail;

   /* assigning indices would be waste
    * on the plane geometry */

   /* scale the cube */
   glhckObjectScalef(object, width/2.0f, height/2.0f, 1.0f);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief create new sprite */
GLHCKAPI glhckObject* glhckSpriteNewFromFile(const char *file, kmScalar width, kmScalar height, unsigned int importFlags, const glhckTextureParameters *params)
{
   glhckObject *object;
   glhckTexture *texture;
   CALL(0, "%s, %f, %f, %u, %p", file, width, height, importFlags, params);

   /* load texture */
   if (!(texture = glhckTextureNew(file, importFlags, params))) {
      RET(0, "%p", NULL);
      return NULL;
   }

   /* create the sprite object with the texture */
   object = glhckSpriteNew(texture, width, height);

   /* object owns texture now, free this */
   glhckTextureFree(texture);

   RET(0, "%p", object);
   return object;
}

/* \brief create new sprite */
GLHCKAPI glhckObject* glhckSpriteNew(glhckTexture *texture, kmScalar width, kmScalar height)
{
   float w, h;
   glhckObject *object;
   glhckGeometryVertexType vtype;
   CALL(0, "%p, %f, %f", texture, width, height);

   w = (float)(width?width:texture->width)/2.0f;
   h = (float)(height?height:texture->height)/2.0f;
   const glhckImportVertexData vertices[] = {
      {
         {  w,  h,  0 },
         {  0,  0, -1 },     /* normals */
         {  1,  1 },         /* uv coord */
         { 0, 0, 0, 0 }      /* color */
      },{
         { -w,  h,  0 },
         {  0,  0, -1 },
         {  0,  1 },
         { 0, 0, 0, 0 }
      },{
         {  w, -h,  0 },
         {  0,  0, -1 },
         {  1,  0 },
         { 0, 0, 0, 0 }
      },{
         { -w, -h,  0 },
         {  0,  0, -1 },
         {  0,  0 },
         { 0, 0, 0, 0 }
      }
   };

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* choose optimal precision */
   glhckGetGlobalPrecision(NULL, &vtype);
   if (vtype == GLHCK_VERTEX_NONE) {
      vtype = glhckVertexTypeGetV2Counterpart(glhckVertexTypeForSize(texture->width, texture->height));
   }

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertices(object, vtype,
            &vertices[0], LENGTH(vertices)) != RETURN_OK)
      goto fail;

   /* don't make things humongous */
   w = 1.0f-(1.0f/(texture->width>texture->height?
            texture->width:texture->height));

   /* scale keeping aspect ratio */
   glhckObjectScalef(object, w, w, w);

   /* pass reference to object */
   glhckObjectTexture(object, texture);

   /* set filename of object */
   _glhckObjectSetFile(object, texture->file);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckTextureFree, texture);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
