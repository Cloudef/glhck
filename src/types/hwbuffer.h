#ifndef __glhck_hwbuffer_h__
#define __glhck_hwbuffer_h__

#include "shader.h"

/* glhck hw buffer uniform location type */
struct glhckHwBufferShaderUniform {
   char *name;
   const char *typeName;
   int offset, size;
   enum glhckShaderVariableType type;
};

#endif /* __glhck_hwbuffer_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
