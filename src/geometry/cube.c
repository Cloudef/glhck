#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* \brief create new cube object */
GLHCKAPI glhckObject* glhckCubeNew(kmScalar size)
{
   glhckGeometryVertexType vtype;

   /* choose internal vertexdata precision */
   vtype = _GLHCKlibrary.render.globalVertexType;
   if (vtype == GLHCK_VERTEX_NONE) vtype = GLHCK_VERTEX_V3B;
   return glhckCubeNewEx(size, GLHCK_INDEX_NONE, vtype);
}

/* \brief create new cube object (specify precision) */
GLHCKAPI glhckObject* glhckCubeNewEx(kmScalar size,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   _glhckObject *object;

   const glhckImportVertexData vertices[] = {
      {
         /* FRONT */
         { -1, -1,  1 },       /* vertices */
         {  0,  0,  0 },       /* normals  */
         {  0,  0     },       /* uv coord */
         { 0,  0,  0,  0 }     /* color    */
      },{
         {  1, -1,  1 },
         {  0,  0,  0 },
         {  1,  0     },
         { 0,  0,  0,  0 }
      },{
         { -1,  1,  1 },
         {  0,  0,  0 },
         {  0,  1     },
         { 0,  0,  0,  0 }
      },{
         {  1,  1,  1 },
         {  0,  0,  0 },
         {  1,  1     },
         { 0,  0,  0,  0 }
       /* RIGHT */
      },{
         {  1,  1,  1 },
         {  0,  0,  0 },
         {  0,  1     },
         { 0,  0,  0,  0 }
      },{
         {  1, -1,  1 },
         {  0,  0,  0 },
         {  0,  0     },
         { 0,  0,  0,  0 }
      },{
         {  1,  1, -1 },
         {  0,  0,  0 },
         {  1,  1     },
         { 0,  0,  0,  0 }
      },{
         {  1, -1, -1 },
         {  0,  0,  0 },
         {  1,  0     },
         { 0,  0,  0,  0 }
       /* BACK */
      },{
         {  1, -1, -1 },
         {  0,  0,  0 },
         {  0,  0     },
         { 0,  0,  0,  0 }
      },{
         { -1, -1, -1 },
         {  0,  0,  0 },
         {  1,  0     },
         { 0,  0,  0,  0 }
      },{
         {  1,  1, -1 },
         {  0,  0,  0 },
         {  0,  1     },
         { 0,  0,  0,  0 }
      },{
         { -1,  1, -1 },
         {  0,  0,  0 },
         {  1,  1     },
         { 0,  0,  0,  0 }
       /* LEFT */
      },{
         { -1,  1, -1 },
         {  0,  0,  0 },
         {  0,  1     },
         { 0,  0,  0,  0 }
      },{
         { -1, -1, -1 },
         {  0,  0,  0 },
         {  0,  0     },
         { 0,  0,  0,  0 }
      },{
         { -1,  1,  1 },
         {  0,  0,  0 },
         {  1,  1     },
         { 0,  0,  0,  0 }
      },{
         { -1, -1,  1 },
         {  0,  0,  0 },
         {  1,  0     },
         { 0,  0,  0,  0 }
       /* BOTTOM */
      },{
         { -1, -1,  1 },
         {  0,  0,  0 },
         {  0,  1     },
         { 0,  0,  0,  0 }
      },{
         { -1, -1, -1 },
         {  0,  0,  0 },
         {  0,  0     },
         { 0,  0,  0,  0 }
      },{
         {  1, -1,  1 },
         {  0,  0,  0 },
         {  1,  1     },
         { 0,  0,  0,  0 }
      },{
         {  1, -1, -1 },
         {  0,  0,  0 },
         {  1,  0     },
         { 0,  0,  0,  0 }
      /* DEGENERATE */
      },{
         {  1, -1, -1 },
         {  0,  0,  0 },
         {  1,  0     },
         { 0,  0,  0,  0 }
      },{
         { -1,  1,  1 },
         {  0,  0,  0 },
         {  0,  0     },
         { 0,  0,  0,  0 }
      /* TOP */
      },{
         { -1,  1,  1 },
         {  0,  0,  0 },
         {  0,  0     },
         { 0,  0,  0,  0 }
      },{
         {  1,  1,  1 },
         {  0,  0,  0 },
         {  1,  0     },
         { 0,  0,  0,  0 }
      },{
         { -1,  1, -1 },
         {  0,  0,  0 },
         {  0,  1     },
         { 0,  0,  0,  0 }
      },{
         {  1,  1, -1 },
         {  0,  0,  0 },
         {  1,  1     },
         { 0,  0,  0,  0 }
      }
   };

   CALL(0, "%f", size);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertices(object, vtype,
            &vertices[0], LENGTH(vertices)) != RETURN_OK)
      goto fail;

   /* assigning indices would be waste
    * on the cube geometry */

   /* scale the cube */
   glhckObjectScalef(object, size, size, size);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
