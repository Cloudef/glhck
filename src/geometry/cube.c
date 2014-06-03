#include <glhck/glhck.h>
#include "trace.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* \brief create new cube object */
GLHCKAPI glhckHandle glhckCubeNew(const kmScalar size)
{
   glhckVertexType vtype = GLHCK_VTX_AUTO;
   // glhckGetGlobalPrecision(NULL, &vtype);

   if (vtype == GLHCK_VTX_AUTO)
      vtype = GLHCK_VTX_V3B;

   return glhckCubeNewEx(size, GLHCK_IDX_AUTO, vtype);
}

/* \brief create new cube object (specify precision) */
GLHCKAPI glhckHandle glhckCubeNewEx(const kmScalar size, const glhckIndexType itype, const glhckVertexType vtype)
{
   CALL(0, "%f, %u, %u", size, itype, vtype);

   static const glhckImportVertexData vertices[] = {
      {
       /* FRONT */
         { -1, -1,  1 },       /* vertices */
         {  0,  0,  1 },       /* normals  */
         {  0,  0     },       /* uv coord */
         0,                    /* color    */
      },{
         {  1, -1,  1 },
         {  0,  0,  1 },
         {  1,  0     },
         0
      },{
         { -1,  1,  1 },
         {  0,  0,  1 },
         {  0,  1     },
         0
      },{
         {  1,  1,  1 },
         {  0,  0,  1 },
         {  1,  1     },
         0
       /* RIGHT */
      },{
         {  1,  1,  1 },
         {  1,  0,  0 },
         {  0,  1     },
         0
      },{
         {  1, -1,  1 },
         {  1,  0,  0 },
         {  0,  0     },
         0
      },{
         {  1,  1, -1 },
         {  1,  0,  0 },
         {  1,  1     },
         0
      },{
         {  1, -1, -1 },
         {  1,  0,  0 },
         {  1,  0     },
         0
       /* BACK */
      },{
         {  1, -1, -1 },
         {  0,  0, -1 },
         {  0,  0     },
         0
      },{
         { -1, -1, -1 },
         {  0,  0, -1 },
         {  1,  0     },
         0
      },{
         {  1,  1, -1 },
         {  0,  0, -1 },
         {  0,  1     },
         0
      },{
         { -1,  1, -1 },
         {  0,  0, -1 },
         {  1,  1     },
         0
       /* LEFT */
      },{
         { -1,  1, -1 },
         { -1,  0,  0 },
         {  0,  1     },
         0
      },{
         { -1, -1, -1 },
         { -1,  0,  0 },
         {  0,  0     },
         0
      },{
         { -1,  1,  1 },
         { -1,  0,  0 },
         {  1,  1     },
         0
      },{
         { -1, -1,  1 },
         { -1,  0,  0 },
         {  1,  0     },
         0
       /* BOTTOM */
      },{
         { -1, -1,  1 },
         {  0, -1,  0 },
         {  0,  1     },
         0
      },{
         { -1, -1, -1 },
         {  0, -1,  0 },
         {  0,  0     },
         0
      },{
         {  1, -1,  1 },
         {  0, -1,  0 },
         {  1,  1     },
         0
      },{
         {  1, -1, -1 },
         {  0, -1,  0 },
         {  1,  0     },
         0
      /* DEGENERATE */
      },{
         {  1, -1, -1 },
         {  0, -1,  0 },
         {  1,  0     },
         0
      },{
         { -1,  1,  1 },
         {  0,  1,  0 },
         {  0,  0     },
         0
      /* TOP */
      },{
         { -1,  1,  1 },
         {  0,  1,  0 },
         {  0,  0     },
         0
      },{
         {  1,  1,  1 },
         {  0,  1,  0 },
         {  1,  0     },
         0
      },{
         { -1,  1, -1 },
         {  0,  1,  0 },
         {  0,  1     },
         0
      },{
         {  1,  1, -1 },
         {  0,  1,  0 },
         {  1,  1     },
         0
      }
   };

   glhckHandle handle;
   if (!(handle = glhckObjectNew()))
      goto fail;

   if (glhckObjectInsertVertices(handle, vtype, vertices, LENGTH(vertices)) != RETURN_OK)
      goto fail;

   glhckObjectScalef(handle, size, size, size);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   RET(0, "%s", glhckHandleRepr(handle));
   return 0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
