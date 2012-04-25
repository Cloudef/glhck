#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* temporary macro */
#define LENGTH(X) (sizeof X / sizeof X[0])

/* \brief create new cube object */
GLHCKAPI _glhckObject* glhckCubeNew(size_t size)
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

   CALL("%d", size);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertexData(object,
            LENGTH(vertices), &vertices[0]) != RETURN_OK)
      goto fail;

   /* assigning indices would be waste
    * on the cube geometry */

   /* scale the cube */
   glhckObjectScalef(object, size, size, size);

   RET("%p", object);
   return object;

fail:
   if (object) glhckObjectFree(object);
   RET("%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
