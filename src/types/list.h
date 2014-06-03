#ifndef __glhck_list_h__
#define __glhck_list_h__

#include <glhck/glhck.h>

GLHCKAPI glhckHandle glhckListNew(const size_t items, const size_t member);
GLHCKAPI glhckHandle glhckListNewFromCArray(const void *items, const size_t memb, const size_t member);
GLHCKAPI int glhckListSet(const glhckHandle handle, const void *items, const size_t memb);
GLHCKAPI const void* glhckListGet(const glhckHandle handle, size_t *outMemb);
GLHCKAPI size_t glhckListCount(const glhckHandle handle);
GLHCKAPI void* glhckListAdd(const glhckHandle handle, const void *item);
GLHCKAPI void* glhckListFetch(const glhckHandle handle, const size_t index);
GLHCKAPI void glhckListRemove(const glhckHandle handle, const size_t index);
GLHCKAPI void glhckListFlush(const glhckHandle handle);

#endif /* __glhck_list_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
