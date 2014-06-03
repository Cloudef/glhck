#include <glhck/glhck.h>
#include "trace.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* \brief create new plane object */
GLHCKAPI glhckHandle glhckPlaneNew(const kmScalar width, const kmScalar height)
{
   glhckVertexType vtype = GLHCK_VTX_AUTO;
   // glhckGetGlobalPrecision(NULL, &vtype);

   if (vtype == GLHCK_VTX_AUTO)
      vtype = GLHCK_VTX_V2B;

   return glhckPlaneNewEx(width, height, GLHCK_IDX_AUTO, vtype);
}

/* \brief create new plane object (precision specify) */
GLHCKAPI glhckHandle glhckPlaneNewEx(const kmScalar width, const kmScalar height, const glhckIndexType itype, const glhckVertexType vtype)
{
   CALL(0, "%f, %f, %u, %u", width, height, itype, vtype);

   static const glhckImportVertexData vertices[] = {
      {
         {  1,  1,  0 },     /* vertices */
         {  0,  0, -1 },     /* normals  */
         {  1,  1 },         /* uv coord */
         0                   /* colors   */
      },{
         { -1,  1,  0 },
         {  0,  0, -1 },
         {  0,  1 },
         0
      },{
         {  1, -1,  0 },
         {  0,  0, -1 },
         {  1,  0 },
         0
      },{
         { -1, -1,  0 },
         {  0,  0, -1 },
         {  0,  0 },
         0
      }
   };

   glhckHandle handle;
   if (!(handle = glhckObjectNew()))
      goto fail;

   if (glhckObjectInsertVertices(handle, vtype, vertices, LENGTH(vertices)) != RETURN_OK)
      goto fail;

   glhckObjectScalef(handle, width * 0.5f, height * 0.5f, 1.0f);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   RET(0, "%s", glhckHandleRepr(handle));
   return handle;
}

/* \brief create new sprite */
GLHCKAPI glhckHandle glhckSpriteNewFromFile(const char *file, const kmScalar width, const kmScalar height, const glhckPostProcessImageParameters *postParams, const glhckTextureParameters *params)
{
   CALL(0, "%s, %f, %f, %p, %p", file, width, height, postParams, params);

   glhckHandle texture;
   if (!(texture = glhckTextureNewFromFile(file, postParams, (params ? params : glhckTextureDefaultSpriteParameters()))))
      goto fail;

   glhckHandle sprite = glhckSpriteNew(texture, width, height);
   glhckHandleRelease(texture);

   RET(0, "%s", glhckHandleRepr(sprite));
   return sprite;

fail:
   IFDO(glhckHandleRelease, texture);
   RET(0, "%s", glhckHandleRepr(texture));
   return 0;
}

/* \brief create new sprite */
GLHCKAPI glhckHandle glhckSpriteNew(const glhckHandle texture, const kmScalar width, const kmScalar height)
{
   CALL(0, "%s, %f, %f", glhckHandleRepr(texture), width, height);
   assert(texture > 0);

   const int tw = (glhckTextureGetWidth(texture) > 0 ? glhckTextureGetWidth(texture) : 1);
   const int th = (glhckTextureGetHeight(texture) > 0 ? glhckTextureGetHeight(texture) : 1);
   const float w = (float)(width > 0.0f ? width : tw) * 0.5f;
   const float h = (float)(height > 0.0f ? height : th) * 0.5f;
   const glhckImportVertexData vertices[] = {
      {
         {  w,  h,  0 },     /* vertices */
         {  0,  0, -1 },     /* normals  */
         {  1,  1 },         /* uv coord */
         0                   /* color    */
      },{
         { -w,  h,  0 },
         {  0,  0, -1 },
         {  0,  1 },
         0
      },{
         {  w, -h,  0 },
         {  0,  0, -1 },
         {  1,  0 },
         0
      },{
         { -w, -h,  0 },
         {  0,  0, -1 },
         {  0,  0 },
         0
      }
   };

   glhckHandle handle, material = 0;
   if (!(handle = glhckObjectNew()))
      goto fail;

   if (!(material = glhckMaterialNew(texture)))
      goto fail;

   glhckVertexType vtype = GLHCK_VTX_AUTO;
   // glhckGetGlobalPrecision(NULL, &vtype);

#if 0
   /* choose optimal precision */
   if (vtype == GLHCK_VTX_AUTO) {
      kmScalar max = (tw > th ? tw : th);
      for (glhckVertexType vtype = GLHCK_VTX_V2F, i = 0; i < glhckGeometryVertexTypeCount(); ++i) {
         if (i == vtype || GLHCKVT(i)->memb[0] != 2)
            continue;

         if (GLHCKVT(i)->max[0] >= max && GLHCKVT(i)->max[0] < GLHCKVT(vtype)->max[0])
            vtype = i;
      }
   }
#endif

   if (glhckObjectInsertVertices(handle, vtype, vertices, LENGTH(vertices)) != RETURN_OK)
      goto fail;

   /* don't make things humongous */
   const float s = 1.0f - (1.0f / (tw > th ? tw : th));

   /* scale keeping aspect ratio */
   glhckObjectScalef(handle, s, s, s);

   /* set material to object */
   glhckObjectMaterial(handle, material);
   glhckHandleRelease(material);

#if 0
   /* set filename of object */
   glhckObjectFile(object, texture->file);
#endif

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   IFDO(glhckHandleRelease, material);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
