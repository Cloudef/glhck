#include "handle.h"

#include <stdlib.h>

#include "trace.h"

#include "system/tls.h"
#include "system/condition.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_HANDLE

struct info {
   size_t last;
   chckPool **pools;
   const unsigned int *sizes;
   _glhckHandleDestructor destructor;
   glhckType type;
};

enum {
   $ids, // glhckHandle
   $infos, // struct info
   $references, // unsigned int
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // ids
   sizeof(struct info), // infos
   sizeof(unsigned int), // references
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#define GET(x, y) chckPoolGet(pools[x], y - 1)

static const char *names[GLHCK_TYPE_LAST] = {
   "animation", /* GLHCK_TYPE_ANIMATION */
   "animation node", /* GLHCK_TYPE_ANIMATION_NODE */
   "animator", /* GLHCK_TYPE_ANIMATOR */
   "atlas", /* GLHCK_TYPE_ATLAS */
   "bone", /* GLHCK_TYPE_BONE */
   "camera", /* GLHCK_TYPE_CAMERA */
   "framebuffer", /* GLHCK_TYPE_FRAMEBUFFER */
   "hwbuffer", /* GLHCK_TYPE_HWBUFFER */
   "light", /* GLHCK_TYPE_LIGHT */
   "material", /* GLHCK_TYPE_MATERIAL */
   "object", /* GLHCK_TYPE_OBJECT */
   "renderbuffer", /* GLHCK_TYPE_RENDERBUFFER */
   "shader", /* GLHCK_TYPE_SHADER */
   "skin bone", /* GLHCK_TYPE_SKINBONE */
   "text", /* GLHCK_TYPE_TEXT */
   "texture", /* GLHCK_TYPE_TEXTURE */
   "geometry", /* GLHCK_TYPE_GEOMETRY */
   "view", /* GLHCK_TYPE_VIEW */
   "string", /* GLHCK_TYPE_STRING */
   "list", /* GLHCK_TYPE_LIST */
};

static _GLHCK_TLS glhckType lastInternalType = GLHCK_TYPE_LAST;

static const char* internalHandleRepr(const void *data, size_t *outLen)
{
   assert(data);

   glhckHandle internalHandle = *(glhckHandle*)data;
   int garbage = _unlikely_(internalHandle <= 0 || lastInternalType == GLHCK_TYPE_LAST);

   const char *name = (garbage ? "garbage" : names[lastInternalType]);
   return _glhckSprintf(outLen, "(internal %s %zu)", name, internalHandle);
}

static const char* handleReprLen(const void *data, size_t *outLen)
{
   assert(data);

   glhckHandle handle = *(glhckHandle*)data;
   int garbage = _unlikely_(handle <= 0 || handle - 1 >= chckPoolCount(pools[$ids]));

   const struct info *info = (garbage ? NULL : GET($infos, handle));
   const glhckHandle internalHandle = (garbage ? 0 : *(glhckHandle*)GET($ids, handle));
   unsigned int *references = (garbage ? (unsigned int[]){0} : GET($references, handle));
   const char *name = (garbage ? "garbage" : names[info->type]);

   return _glhckSprintf(outLen, "(%s %zu:%zu, %u)", name, handle, internalHandle, *references);
}

const char* _glhckHandleReprArray(const glhckType type, const glhckHandle *internalHandles, const size_t memb)
{
   lastInternalType = type;
   return _glhckSprintfArray(NULL, internalHandles, memb, sizeof(glhckHandle), internalHandleRepr);
}

const char* _glhckHandleRepr(const glhckType type, const glhckHandle internalHandle)
{
   lastInternalType = type;
   return internalHandleRepr(&internalHandle, NULL);
}

static glhckHandle handleAdd(const struct info *info, const glhckHandle internalHandle)
{
   glhckHandle outHandle = -1;

   if (_unlikely_(!pools[0])) {
      for (size_t i = 0; i < POOL_LAST; ++i) {
         if (!(pools[i] = chckPoolNew(32, 32, pool_sizes[i])))
            goto fail;
      }
   }

   if (!(chckPoolAdd(pools[$ids], &internalHandle, &outHandle)))
      goto fail;

   if (!(chckPoolAdd(pools[$infos], info, NULL)))
      goto fail;

   if (!(chckPoolAdd(pools[$references], &(int){1}, NULL)))
      goto fail;

   return outHandle + 1;

fail:
   if (outHandle != (glhckHandle)-1) {
      for (size_t i = 0; i < POOL_LAST; ++i)
         chckPoolRemove(pools[i], outHandle);
   }
   return 0;
}

glhckHandle _glhckInternalHandleCreateFrom(const glhckType type, chckPool **pools, const unsigned int *sizes, const size_t last, _glhckHandleDestructor destructor, glhckHandle *outInternalHandle)
{
   assert(pools && sizes && destructor);

   glhckHandle handle;
   glhckHandle internalHandle = -1;

   for (size_t i = 0; i < last; ++i) {
      if (!pools[i] && !(pools[i] = chckPoolNew(32, 32, sizes[i])))
         goto fail;

      if (!(chckPoolAdd(pools[i], NULL, &internalHandle)))
         goto fail;
   }

   struct info info = {
      .last = last,
      .pools = pools,
      .sizes = sizes,
      .destructor = destructor,
      .type = type,
   };

   if (!(handle = handleAdd(&info, internalHandle + 1)))
      goto fail;

   if (outInternalHandle)
      *outInternalHandle = internalHandle + 1;

   return handle;

fail:
   if (internalHandle != (glhckHandle)-1) {
      for (size_t i = 0; i < last; ++i)
         chckPoolRemove(pools[i], internalHandle);
   }
   return 0;
}

glhckHandle _glhckHandleGetInternalHandle(const glhckHandle handle)
{
   assert(handle > 0);

   if (_unlikely_(handle <= 0 || handle - 1 >= chckPoolCount(pools[$ids])))
      return 0;

   return *(glhckHandle*)GET($ids, handle);
}

void _glhckHandleTerminate(void)
{
   TRACE(0);

   const glhckHandle *id;
   for (chckPoolIndex iter = 0; (id = chckPoolIter(pools[$ids], &iter));)
      while (glhckHandleRelease(*id) > 0);

   for (size_t i = 0; i < POOL_LAST; ++i)
      NULLDO(chckPoolFree, pools[i]);
}

GLHCKAPI unsigned int glhckHandleRef(const glhckHandle handle)
{
   assert(handle > 0);

   if (_unlikely_(handle <= 0 || handle - 1 >= chckPoolCount(pools[$ids])))
      return 0;

   return ++(*(unsigned int*)GET($references, handle));
}

GLHCKAPI unsigned int glhckHandleRefPtr(const glhckHandle *handle)
{
   assert(handle);
   return glhckHandleRef(*handle);
}

GLHCKAPI unsigned int glhckHandleRelease(const glhckHandle handle)
{
   assert(handle > 0);

   if (_unlikely_(handle <= 0 || handle - 1 >= chckPoolCount(pools[$ids])))
      return 0;

   unsigned int *refs = GET($references, handle);

   if (*refs > 0 && --(*refs) > 0)
      return *refs;

   const struct info *info = GET($infos, handle);
   info->destructor(handle);

   glhckHandle internal = *(glhckHandle*)GET($ids, handle);
   for (size_t i = 0; i < info->last; ++i)
      chckPoolRemove(info->pools[i], internal);

   for (size_t i = 0; i < POOL_LAST; ++i)
      chckPoolRemove(pools[i], handle - 1);

   return *refs;
}

GLHCKAPI unsigned int glhckHandleReleasePtr(const glhckHandle *handle)
{
   assert(handle);
   return glhckHandleRelease(*handle);
}

GLHCKAPI glhckType glhckHandleGetType(const glhckHandle handle)
{
   assert(handle > 0);

   if (_unlikely_(handle <= 0 || handle - 1 >= chckPoolCount(pools[$ids])))
      return GLHCK_TYPE_LAST;

   const struct info *info = GET($infos, handle);
   return info->type;
}

GLHCKAPI const char* glhckHandleReprArray(const glhckHandle *handles, const size_t memb)
{
   return _glhckSprintfArray(NULL, handles, memb, sizeof(glhckHandle), handleReprLen);
}

GLHCKAPI const char* glhckHandleRepr(const glhckHandle handle)
{
   return handleReprLen(&handle, NULL);
}

/* vim: set ts=8 sw=3 tw=0 :*/
