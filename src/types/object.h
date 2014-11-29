#ifndef __glhck_object_h__
#define __glhck_object_h__

#include <glhck/glhck.h>

enum glhckObjectFlags {
   GLHCK_OBJECT_NONE           = 0,
   GLHCK_OBJECT_ROOT           = 1<<0,
   GLHCK_OBJECT_CULL           = 1<<1,
   GLHCK_OBJECT_DEPTH          = 1<<2,
   GLHCK_OBJECT_VERTEX_COLOR   = 1<<3,
   GLHCK_OBJECT_DRAW_AABB      = 1<<4,
   GLHCK_OBJECT_DRAW_OBB       = 1<<5,
   GLHCK_OBJECT_DRAW_SKELETON  = 1<<6,
   GLHCK_OBJECT_DRAW_WIREFRAME = 1<<7,
};

void _glhckObjectUpdateMatrixMany(const glhckHandle *handles, const size_t memb);
glhckHandle glhckObjectGetView(const glhckHandle handle);

#endif /* __glhck_object_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
