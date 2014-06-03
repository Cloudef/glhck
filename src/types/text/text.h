#ifndef __glhck_text_h__
#define __glhck_text_h__

#include <glhck/glhck.h>
#include <limits.h>

#include "pool/pool.h"

#if GLHCK_TEXT_FLOAT_PRECISION
#  define GLHCK_TEXT_TEXTURE_RANGE 1.0f
#  define GLHCK_TEXT_VTYPE glhckVertexData2f
#  define GLHCK_TEXT_PRECISION GLHCK_FLOAT
#else
#  define GLHCK_TEXT_TEXTURE_RANGE SHRT_MAX
#  define GLHCK_TEXT_VTYPE glhckVertexData2s
#  define GLHCK_TEXT_PRECISION GLHCK_SHORT
#endif

#if GLHCK_TRISTRIP
#  define GLHCK_TEXT_GTYPE GLHCK_TRIANGLE_STRIP
#else
#  define GLHCK_TEXT_GTYPE GLHCK_TRIANGLES
#endif

/* representation of text geometry */
struct glhckTextGeometry {
   GLHCK_TEXT_VTYPE *vertexData;
   int vertexCount, allocatedCount;
};

/* text texture container */
struct glhckTextTexture {
   struct glhckTextGeometry geometry;
   chckIterPool *rows;
   glhckHandle texture;
   float internalWidth, internalHeight;
   char bitmap;
};

struct glhckTextTexture* _glhckTextGetTextTexture(const glhckHandle handle, const chckPoolIndex index);

#endif /* __glhck_text_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
