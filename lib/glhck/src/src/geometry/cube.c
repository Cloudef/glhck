#include "../internal.h"

#define LENGTH(X) (sizeof X / sizeof X[0])

GLHCKAPI _glhckObject* glhckCubeNew(int size)
{
   _glhckObject *object;

   static const glhckVertexData vertices[] = {
      {
         { -1, -1,  1 },       /* vertices */
         {  0,  0,  0 },       /* normals  */
         {  0,  0,  0,  0 }    /* color    */
      },{
         {  1, -1,  1 },
         {  0,  0,  0 },
         {  0,  0,  0,  0 },
      },{
         { -1,  1,  1 },
         {  0,  0,  0 },
         {  0,  0,  0,  0 },
      },{
         {  1,  1,  1 },
         {  0,  0,  0 },
         {  0,  0,  0,  0 },
      },{
         { -1, -1, -1 },
         {  0,  0,  0 },
         {  0,  0,  0,  0 },
      },{
         {  1, -1, -1 },
         {  0,  0,  0 },
         {  0,  0,  0,  0 },
      },{
         { -1,  1, -1 },
         {  0,  0,  0 },
         {  0,  0,  0,  0 },
      },{
         {  1,  1, -1 },
         {  0,  0,  0 },
         {  0,  0,  0,  0 },
      }
   };

   static const unsigned short indices[] = {
      0, 1, 2, 3, 7, 1, 5, 4, 7, 6, 2, 4, 0, 1
   };

   CALL("%d", size);

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertexData(object,
            8, &vertices[0]) != RETURN_OK)
      goto fail;

   /* insert indices to object's geometry */
   if (glhckObjectInsertIndices(object,
            LENGTH(indices), &indices[0]) != RETURN_OK)
      goto fail;

   RET("%p", object);
   return object;

fail:
   if (object) glhckObjectFree(object);
   RET("%p", NULL);
   return NULL;
}
