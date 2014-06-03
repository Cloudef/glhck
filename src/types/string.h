#ifndef __glhck_string_h__
#define __glhck_string_h__

#include <glhck/glhck.h>

GLHCKAPI glhckHandle glhckStringNew(void);
GLHCKAPI glhckHandle glhckStringNewFromCStr(const char *cstr);
GLHCKAPI int glhckStringCStr(const glhckHandle handle, const char *cstr);
GLHCKAPI const char* glhckStringGetCStr(const glhckHandle handle);
GLHCKAPI size_t glhckStringGetLength(const glhckHandle handle);

#endif /* __glhck_string_h__ */
