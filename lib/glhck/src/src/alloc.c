#define _GNU_SOURCE
#include "internal.h"
#include <stdio.h>   /* for printf */
#include <stdlib.h>  /* for malloc */
#include <assert.h>  /* for assert */
#include <dlfcn.h>   /* for dlsym  */

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ALLOC

/* allocation tracking on debug build */
#ifndef NDEBUG
#define GLHCK_ALLOC_CRITICAL 100 * 1048576 /* 100 MiB */
#define GLHCK_ALLOC_HIGH     80  * 1048576 /* 80  MiB */
#define GLHCK_ALLOC_AVERAGE  40  * 1048576 /* 40  MiB */

/* \brief add new data to tracking */
static void trackAlloc(const char *channel, void *ptr, size_t size)
{
   __GLHCKalloc *data;

   /* real alloc failed */
   if (!ptr) return;
   data = _GLHCKlibrary.alloc;

   /* track */
   if (!data)
      data = _GLHCKlibrary.alloc = malloc(sizeof(__GLHCKalloc));
   else {
      for (; data && data->next; data = data->next);
      data = data->next = malloc(sizeof(__GLHCKalloc));
   }

   /* alloc failed */
   if (!data)
      return;

   /* init */
   memset(data, 0, sizeof(__GLHCKalloc));
   data->size = size;
   data->ptr = ptr;
   data->channel = channel;
}

/* \brief internal realloc hook */
static void trackRealloc(void *ptr, void *ptr2, size_t size)
{
   __GLHCKalloc *data = _GLHCKlibrary.alloc;

   /* find */
   for (; data; data = data->next)
      if (data->ptr == ptr) {
         data->ptr  = ptr2;
         data->size = size;
      }
}

/* \brief internal free hook */
static void trackFree(void *ptr)
{
   __GLHCKalloc *found, *data = _GLHCKlibrary.alloc;

   /* find */
   for (; data && data->next &&
          data->next->ptr != ptr;
          data = data->next);

   /* free */
   if ((found = data->next)) {
      data->next = found->next;
      free(found);
   }
}

/* \brief add known allocate data to tracking
 * (from seperate library for example)
 *
 * NOTE: This won't allocate new node,
 * instead it modifies size of already allocated node. */
void _glhckTrackFake(void *ptr, size_t size)
{
   trackRealloc(ptr, ptr, size);
}

/* \brief terminate all tracking */
void _glhckTrackTerminate(void)
{
   __GLHCKalloc *next, *data = _GLHCKlibrary.alloc;

   for (; data; data = next) {
      next = data->next;
      free(data);
   }

   _GLHCKlibrary.alloc = NULL;
}
#endif /* NDEBUG */

/* Use _glhckMalloc and _glhckCalloc and _glhckCopy macros instead
 * of __glhckMalloc and __glhckCalloc and _glhckCopy.
 * They will assign GLCHK_CHANNEL automatically.
 *
 * \brief internal malloc function. */
void* __glhckMalloc(const char *channel, size_t size)
{
   void *ptr;
   CALL(3, "%s, %zu", channel, size);

   if (!(ptr = malloc(size)))
      goto fail;

#ifndef NDEBUG
   trackAlloc(channel, ptr, size);
#endif

   RET(3, "%p", ptr);
   return ptr;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to allocate %zu bytes", size);
   RET(3, "%p", NULL);
   return NULL;
}

/* \brief internal calloc function. */
void* __glhckCalloc(const char *channel, size_t nmemb, size_t size)
{
   void *ptr;
   CALL(3, "%s, %zu, %zu", channel, nmemb, size);

   if (!(ptr = calloc(nmemb, size)))
      goto fail;

#ifndef NDEBUG
   trackAlloc(channel, ptr, nmemb * size);
#endif

   RET(3, "%p", ptr);
   return ptr;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to allocate %zu bytes", nmemb * size);
   RET(3, "%p", NULL);
   return NULL;
}

/* \brief internal strdup function. */
char* __glhckStrdup(const char *channel, const char *s)
{
   char *s2;
   CALL(3, "%s, %s", channel, s);
   if (!(s2 = strdup(s)))
      goto fail;

#ifndef NDEBUG
   trackAlloc(channel, s2, strlen(s) + 1);
#endif

   RET(3, "%s", s2);
   return s2;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to strdup '%s'", s);
   RET(3, "%p", NULL);
   return NULL;
}

/* \brief internal memcpy function. */
void* __glhckCopy(const char *channel, void *ptr, size_t nmemb)
{
   void *ptr2;
   CALL(3, "%p, %zu", ptr, nmemb);
   assert(ptr);

   if (!(ptr2 = __glhckMalloc(channel, nmemb)))
      goto fail;

   memcpy(ptr2, ptr, nmemb);
   RET(3, "%p", ptr2);
   return ptr2;

fail:
   RET(3, "%p", NULL);
   return NULL;
}

/* \brief internal realloc function. */
void* _glhckRealloc(void *ptr, size_t omemb, size_t nmemb, size_t size)
{
   void *ptr2;
   CALL(3, "%p, %zu, %zu, %zu", ptr, omemb, nmemb, size);

   if (!(ptr2 = realloc(ptr, nmemb * size))) {
      if (!(ptr2 = malloc(nmemb * size)))
         goto fail;
      memcpy(ptr2, ptr, omemb * size);
      free(ptr);
   }

#ifndef NDEBUG
   trackRealloc(ptr, ptr2, size);
#endif

   RET(3, "%p", ptr2);
   return ptr2;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to reallocate %zu bytes",
         nmemb * size - omemb * size);
   RET(3, "%p", NULL);
   return NULL;
}

/* \brief internal free function. */
void _glhckFree(void *ptr)
{
   CALL(3, "%p", ptr);
   assert(ptr);

#ifndef NDEBUG
   trackFree(ptr);
#endif

   free(ptr);
}

/* public api */

/* \brief output memory usage graph */
GLHCKAPI void glhckMemoryGraph(void)
{
   TRACE(0);
#ifndef NDEBUG
   __GLHCKalloc *data;
   __GLHCKtraceChannel *trace;
   unsigned int i;
   size_t allocChannel, allocTotal;
   trace = _GLHCKlibrary.trace.channel;

   puts("");
   puts("--- Memory Graph ---");

   allocTotal = 0;
   for (i = 0;;++i) {
      allocChannel = 0;

      if (trace[i].name) {
         for (data = _GLHCKlibrary.alloc;
              data; data = data->next)
            if (data->channel == trace[i].name)
               allocChannel += data->size;
      } else {
         allocChannel = allocTotal;
         puts("--------------------");
      }

      /* set color */
      if (allocChannel >= GLHCK_ALLOC_CRITICAL)       _glhckRed();
      else if (allocChannel >= GLHCK_ALLOC_HIGH)      _glhckBlue();
      else if (allocChannel >= GLHCK_ALLOC_AVERAGE)   _glhckYellow();
      else _glhckGreen();

      printf("%-13s : ", trace[i].name?trace[i].name:"Total");
      if (allocChannel / 1048576 != 0)
         printf("%-4.2f MiB\n", (float)allocChannel / 1048576);
      else if (allocChannel / 1024 != 0)
         printf("%-4.2f KiB\n", (float)allocChannel / 1024);
      else
         printf("%-4zu B\n", allocChannel);

      /* reset color */
      _glhckNormal();

      if (!trace[i].name) break;
      allocTotal += allocChannel;
   }

   puts("--------------------");
   puts("");
   return;
#else
   puts("-- Memory graph only available on debug build --");
#endif
}

/* vim: set ts=8 sw=3 tw=0 :*/
