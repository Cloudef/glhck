#include "string.h"

#include <stdlib.h>

#include "handle.h"
#include "trace.h"
#include "system/tls.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_STRING

struct string {
   char *str;
   size_t length;
};

enum pool {
   $string, // struct string
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(struct string), // string
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static char* pstrdup(const char *str, size_t *outLen)
{
   assert(outLen);
   *outLen = strlen(str);
   char *cpy = calloc(1, *outLen + 1);
   return (cpy ? memcpy(cpy, str, *outLen) : NULL);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   struct string *string = (struct string*)get($string, handle);
   IFDO(free, string->str);
}

GLHCKAPI glhckHandle glhckStringNew(void)
{
   TRACE(0);
   const glhckHandle handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_STRING, pools, pool_sizes, POOL_LAST, destructor, NULL);
   RET(0, "%s", glhckHandleRepr(handle));
   return handle;
}

GLHCKAPI glhckHandle glhckStringNewFromCStr(const char *cstr)
{
   const glhckHandle handle = glhckStringNew();

   if (!handle)
      return 0;

   glhckStringCStr(handle, cstr);
   return handle;
}

GLHCKAPI int glhckStringCStr(const glhckHandle handle, const char *cstr)
{
   struct string *string = (struct string*)get($string, handle);

   if (cstr) {
      size_t newLen;
      char *newStr = pstrdup(cstr, &newLen);
      if (!newStr)
         return RETURN_FAIL;

      IFDO(free, string->str);
      string->str = newStr;
      string->length = newLen;
   } else {
      IFDO(free, string->str);
      string->length = 0;
   }

   return RETURN_OK;
}

GLHCKAPI const char* glhckStringGetCStr(const glhckHandle handle)
{
   return ((struct string*)get($string, handle))->str;
}

GLHCKAPI size_t glhckStringGetLength(const glhckHandle handle)
{
   return ((struct string*)get($string, handle))->length;
}

/* vim: set ts=8 sw=3 tw=0 :*/
