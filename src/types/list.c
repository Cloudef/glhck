#include "string.h"

#include <stdlib.h>

#include "handle.h"
#include "trace.h"
#include "system/tls.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_STRING

struct list {
   chckIterPool *pool;
};

enum pool {
   $list, // struct list
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(struct list), // list
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   struct list *list = (struct list*)get($list, handle);
   IFDO(chckIterPoolFree, list->pool);
}

GLHCKAPI glhckHandle glhckListNew(const size_t items, const size_t member)
{
   chckIterPool *pool = NULL;
   TRACE(0);

   if (!(pool = chckIterPoolNew(32, items, member)))
      goto fail;

   glhckHandle handle;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_LIST, pools, pool_sizes, POOL_LAST, destructor, NULL)))
      goto fail;

   struct list *list = (struct list*)get($list, handle);
   list->pool = pool;

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(chckIterPoolFree, pool);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

GLHCKAPI glhckHandle glhckListNewFromCArray(const void *items, const size_t memb, const size_t member)
{
   const glhckHandle handle = glhckListNew(memb, member);

   if (!handle)
      return 0;

   glhckListSet(handle, items, memb);
   return handle;
}

GLHCKAPI int glhckListSet(const glhckHandle handle, const void *items, const size_t memb)
{
   CALL(2, "%s, %p, %zu", glhckHandleRepr(handle), items, memb);

   struct list *list = (struct list*)get($list, handle);

   if (!chckIterPoolSetCArray(list->pool, items, memb))
      goto fail;

   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const void* glhckListGet(const glhckHandle handle, size_t *outMemb)
{
   CALL(2, "%s, %p", glhckHandleRepr(handle), outMemb);
   struct list *list = (struct list*)get($list, handle);
   const void *ptr = (list->pool ? chckIterPoolToCArray(list->pool, outMemb) : NULL);
   RET(2, "%p", ptr);
   return ptr;
}

GLHCKAPI size_t glhckListCount(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   struct list *list = (struct list*)get($list, handle);
   const size_t count = (list->pool ? chckIterPoolCount(list->pool) : 0);
   RET(2, "%zu", count);
   return count;
}

GLHCKAPI void* glhckListAdd(const glhckHandle handle, const void *item)
{
   CALL(2, "%s, %p", glhckHandleRepr(handle), item);
   struct list *list = (struct list*)get($list, handle);
   void *ptr = chckIterPoolAdd(list->pool, item, NULL);
   RET(2, "%p", ptr);
   return ptr;
}

GLHCKAPI void* glhckListFetch(const glhckHandle handle, const size_t index)
{
   CALL(2, "%s, %zu", glhckHandleRepr(handle), index);
   struct list *list = (struct list*)get($list, handle);
   void *ptr = chckIterPoolGet(list->pool, index);
   RET(2, "%p", ptr);
   return ptr;
}

GLHCKAPI void glhckListRemove(const glhckHandle handle, const size_t index)
{
   CALL(2, "%s, %zu", glhckHandleRepr(handle), index);
   struct list *list = (struct list*)get($list, handle);
   chckIterPoolRemove(list->pool, index);
}

GLHCKAPI void glhckListFlush(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   struct list *list = (struct list*)get($list, handle);
   chckIterPoolFlush(list->pool);
}

/* vim: set ts=8 sw=3 tw=0 :*/
