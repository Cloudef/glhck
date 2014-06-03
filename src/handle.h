#ifndef __glhck_handle_h__
#define __glhck_handle_h__

#include <glhck/glhck.h>

#include "pool/pool.h"

typedef void (*_glhckHandleDestructor)(const glhckHandle);
glhckHandle _glhckInternalHandleCreateFrom(const glhckType type, chckPool **pools, const unsigned int *sizes, const size_t last, _glhckHandleDestructor destructor, glhckHandle *outInternalHandle);
glhckHandle _glhckHandleGetInternalHandle(const glhckHandle handle);
void _glhckHandleTerminate(void);
const char* _glhckHandleRepr(const glhckType type, const glhckHandle internalHandle);
const char* _glhckHandleReprArray(const glhckType type, const glhckHandle *internalHandles, const size_t memb);

#endif /* __glhck_handle_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
