#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* \brief create new plane object */
GLHCKAPI glhckObject* glhckPlaneNew(kmScalar width, kmScalar height)
{
   unsigned char vtype;
   glhckGetGlobalPrecision(NULL, &vtype);

   if (vtype == GLHCK_VTX_AUTO)
      vtype = GLHCK_VTX_V2B;

   return glhckPlaneNewEx(width, height, GLHCK_IDX_AUTO, vtype);
}

/* \brief create new plane object (precision specify) */
GLHCKAPI glhckObject* glhckPlaneNewEx(kmScalar width, kmScalar height, unsigned char itype, unsigned char vtype)
{
   glhckObject *object = NULL;
   CALL(0, "%f, %f, %u, %u", width, height, itype, vtype);

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
   if (glhckObjectInsertVertices(object, vtype, &vertices[0], LENGTH(vertices)) != RETURN_OK)
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
GLHCKAPI glhckObject* glhckSpriteNewFromFile(const char *file, kmScalar width, kmScalar height,
      const glhckImportImageParameters *importParams, const glhckTextureParameters *params)
{
   glhckTexture *texture = NULL;
   CALL(0, "%s, %f, %f, %p, %p", file, width, height, importParams, params);

   /* load texture */
   if (!(texture = glhckTextureNewFromFile(file, importParams, (params?params:glhckTextureDefaultSpriteParameters()))))
      goto fail;

   /* create the sprite object with the texture */
   glhckObject *object = glhckSpriteNew(texture, width, height);

   /* object owns texture now, free this */
   glhckTextureFree(texture);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckTextureFree, texture);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief create new sprite */
GLHCKAPI glhckObject* glhckSpriteNew(glhckTexture *texture, kmScalar width, kmScalar height)
{
   glhckObject *object = NULL;
   glhckMaterial *material = NULL;
   CALL(0, "%p, %f, %f", texture, width, height);

   float w = (float)(width>0.0f ? width : texture->width) * 0.5f;
   float h = (float)(height>0.0f ? height : texture->height) * 0.5f;
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

   /* create new material */
   if (!(material = glhckMaterialNew(texture)))
      goto fail;

   /* choose optimal precision */
   unsigned char vtype;
   glhckGetGlobalPrecision(NULL, &vtype);

   if (vtype == GLHCK_VTX_AUTO) {
      kmScalar max = (texture->width > texture->height ? texture->width : texture->height);
      for (unsigned int vtype = GLHCK_VTX_V2F, i = 0; i < chckPoolCount(GLHCKW()->vertexTypes); ++i) {
         if (i == vtype || GLHCKVT(i)->memb[0] != 2)
            continue;

         if (GLHCKVT(i)->max[0] >= max && GLHCKVT(i)->max[0] < GLHCKVT(vtype)->max[0])
            vtype = i;
      }
   }

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertices(object, vtype, &vertices[0], LENGTH(vertices)) != RETURN_OK)
      goto fail;

   /* don't make things humongous */
   w = 1.0f - (1.0f/(texture->width > texture->height ? texture->width : texture->height));

   /* scale keeping aspect ratio */
   glhckObjectScalef(object, w, w, w);

   /* set material to object */
   glhckObjectMaterial(object, material);
   glhckMaterialFree(material);

   /* set filename of object */
   _glhckObjectFile(object, texture->file);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
   IFDO(glhckMaterialFree, material);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
