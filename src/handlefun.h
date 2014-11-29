#ifndef __glhck_handlefun_h__
#define __glhck_handlefun_h__

#include "types/string.h"
#include "types/list.h"

#include <stdlib.h>

#define GET(x, y) chckPoolGet(pools[x], _glhckHandleGetInternalHandle(y) - 1)

static inline int setHandle(const enum pool var, const glhckHandle handle, const glhckHandle data)
{
   const glhckHandle current = *(glhckHandle*)GET(var, handle);

   if (current == data)
      return 0;

   if (current)
      glhckHandleRelease(current);

   if (data)
      glhckHandleRef(data);

   memcpy(GET(var, handle), &data, pool_sizes[var]);
   return 1;
}

static inline void releaseHandle(const enum pool var, const glhckHandle handle)
{
   glhckHandle *h = GET(var, handle);

   if (*h)
      glhckHandleRelease(*h);

   *h = 0;
}

static inline glhckHandle copyHandle(const glhckHandle src, glhckHandle (constructor)(void), const int *copy, const int *ref)
{
   assert(constructor);

   glhckHandle handle = 0;
   if (!(handle = constructor()))
      return 0;

   for (int i = 0; copy && copy[i] != -1; ++i)
      memcpy(GET(copy[i], handle), GET(copy[i], src), pool_sizes[i]);

   for (int i = 0; ref && ref[i] != -1; ++i)
      setHandle(ref[i], handle, *(glhckHandle*)GET(ref[i], src));

   return handle;
}

static inline void set(const enum pool var, const glhckHandle handle, const void *data)
{
   assert(handle > 0);
   memcpy(GET(var, handle), data, pool_sizes[var]);
}

static inline const void* get(const enum pool var, const glhckHandle handle)
{
   assert(handle > 0);
   return GET(var, handle);
}

static inline int setCStr(const enum pool var, const glhckHandle handle, const char *cstr)
{
   assert(handle > 0);

   if (!cstr) {
      releaseHandle(var, handle);
      return RETURN_OK;
   }

   glhckHandle string = *(glhckHandle*)GET(var, handle);
   if (!string) {
      if ((string = glhckStringNewFromCStr(cstr)))
         *(glhckHandle*)GET(var, handle) = string;
      return (string ? RETURN_OK : RETURN_FAIL);
   }

   return glhckStringCStr(string, cstr);
}

static inline const char* getCStr(const enum pool var, const glhckHandle handle)
{
   assert(handle > 0);
   const glhckHandle string = *(glhckHandle*)GET(var, handle);
   return (string > 0 ? glhckStringGetCStr(string) : NULL);
}

static inline int setList(const enum pool var, const glhckHandle handle, const void *items, const size_t memb, const size_t member)
{
   assert(handle > 0);

   glhckHandle list = *(glhckHandle*)GET(var, handle);

   if (!items || memb <= 0) {
      if (list)
         glhckListFlush(list);
      return RETURN_OK;
   }

   if (!list) {
      if ((list = glhckListNewFromCArray(items, memb, member)))
         *(glhckHandle*)GET(var, handle) = list;
      return (list ? RETURN_OK : RETURN_FAIL);
   }

   return glhckListSet(list, items, memb);
}

static inline int setListFromPool(const enum pool var, const glhckHandle handle, chckIterPool *pool, const size_t member)
{
   assert(handle > 0 && pool);

   size_t memb;
   const void *items = chckIterPoolToCArray(pool, &memb);
   return setList(var, handle, items, memb, member);
}

static inline const void* getList(const enum pool var, const glhckHandle handle, size_t *outMemb)
{
   assert(handle > 0);

   if (outMemb)
      *outMemb = 0;

   const glhckHandle list = *(glhckHandle*)GET(var, handle);
   return (list ? glhckListGet(list, outMemb) : NULL);
}

static inline size_t getListCount(const enum pool var, const glhckHandle handle)
{
   assert(handle > 0);
   const glhckHandle list = *(glhckHandle*)GET(var, handle);
   return (list ? glhckListCount(list) : 0);
}

static inline int setListHandles(const enum pool var, const glhckHandle handle, const glhckHandle *handles, const size_t memb)
{
   assert(handle > 0);

   size_t num;
   const glhckHandle *current = getList(var, handle, &num);

   glhckHandle *copy = NULL;
   if (current && num > 0) {
      if (!(copy = malloc(num * sizeof(glhckHandle))))
         return RETURN_FAIL;

      memcpy(copy, current, num * sizeof(glhckHandle));
   }

   if (!setList(var, handle, handles, memb, sizeof(glhckHandle)))
      return RETURN_FAIL;

   for (size_t i = 0; i < num; ++i)
      glhckHandleRelease(copy[i]);

   IFDO(free, copy);
   return RETURN_OK;
}

#undef GET

#endif /* __glhck_handlefun_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
