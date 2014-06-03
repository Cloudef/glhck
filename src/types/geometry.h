#ifndef __glhck_geometry_internal_h__
#define __glhck_geometry_internal_h__

#include <glhck/glhck.h>

/* internal vertex type */
struct glhckVertexType {
   glhckVertexTypeFunctionMap api;
   size_t size, max[4], offset[4];
   glhckDataType dataType[4];
   char memb[4], normalized[4];
   unsigned char msize[4];
};

/* internal index type */
struct glhckIndexType {
   glhckIndexTypeFunctionMap api;
   size_t max;
   glhckDataType dataType;
   unsigned char size;
};

int _glhckGeometryInit(void);
void _glhckGeometryTerminate(void);

const struct glhckVertexType* _glhckGeometryGetVertexType(size_t index);
const struct glhckIndexType* _glhckGeometryGetIndexType(size_t index);

#endif /* __glhck_geometry_internal_h__ */
