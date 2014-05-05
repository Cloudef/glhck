#include "internal.h"
#include <stdlib.h>  /* for malloc */
#include <stdio.h>   /* for printf */
#include <assert.h>  /* for assert */

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
   if (!ptr)
      return;

   /* allocate pool */
   if (!glhckContextGet()->allocs) {
      if (!(glhckContextGet()->allocs = chckIterPoolNew(32, 32, sizeof(__GLHCKalloc))))
         return;
   }

   /* track */
   if (!(data = chckIterPoolAdd(glhckContextGet()->allocs, NULL, NULL)))
      return;

   /* init */
   data->size = size;
   data->ptr = ptr;
   data->channel = channel;
}

/* \brief internal realloc hook */
static void trackRealloc(const char *channel, const void *ptr, void *ptr2, size_t size)
{
   chckPoolIndex iter;
   __GLHCKalloc *data;

   for (iter = 0; (data = chckIterPoolIter(glhckContextGet()->allocs, &iter)) && data->ptr != ptr;);

   if (!data)
      return trackAlloc(channel, ptr2, size);

   data->ptr = ptr2;
   data->size = size;
}

/* \brief internal free hook */
static void trackFree(const void *ptr)
{
   chckPoolIndex iter;
   __GLHCKalloc *data;

   for (iter = 0; (data = chckIterPoolIter(glhckContextGet()->allocs, &iter)) && data->ptr != ptr;);

   if (data)
      chckIterPoolRemove(glhckContextGet()->allocs, iter - 1);
}

/* \brief add known allocate data to tracking
 * (from seperate library for example)
 *
 * NOTE: This won't allocate new node,
 * instead it modifies size of already allocated node. */
void __glhckTrackFake(const char *channel, void *ptr, size_t size)
{
   trackRealloc(channel, ptr, ptr, size);
}

/* \brief steal the pointer to current tracking channel
 * useful when internal api function returns allocated object,
 * which is then handled by other object. */
void __glhckTrackSteal(const char *channel, void *ptr)
{
#ifndef NDEBUG
   chckPoolIndex iter;
   __GLHCKalloc *data;

   for (iter = 0; (data = chckIterPoolIter(glhckContextGet()->allocs, &iter)) && data->ptr != ptr;);

   if (data)
      data->channel = channel;
#else
   (void)channel, (void)ptr;
#endif /* NDEBUG */
}

/* \brief terminate all tracking */
void _glhckTrackTerminate(void)
{
   IFDO(chckIterPoolFree, glhckContextGet()->allocs);
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

   if (!(s2 = _glhckStrdupNoTrack(s)))
      goto fail;

#ifndef NDEBUG
   trackAlloc(channel, s2, strlen(s2) + 1);
#endif

   RET(3, "%s", s2);
   return s2;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to strdup '%s'", s);
   RET(3, "%p", NULL);
   return NULL;
}

/* \brief internal memcpy function. */
void* __glhckCopy(const char *channel, const void *ptr, size_t nmemb)
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
void* __glhckRealloc(const char *channel, void *ptr, size_t omemb, size_t nmemb, size_t size)
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
   /* http://llvm.org/bugs/show_bug.cgi?id=16499 */
#ifndef __clang_analyzer__
   trackRealloc(channel, ptr, ptr2, nmemb * size);
#endif
#endif

   RET(3, "%p", ptr2);
   return ptr2;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to reallocate %zu bytes", nmemb * size - omemb * size);
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

/***
 * public api
 ***/

/* \brief output memory usage graph */
GLHCKAPI void glhckMemoryGraph(void)
{
   TRACE(0);
#if !defined(NDEBUG) && !GLHCK_DISABLE_TRACE
   __GLHCKalloc *data;
   __GLHCKtraceChannel *trace;
   unsigned int i;
   chckPoolIndex iter, allocChannel, allocTotal;
   trace = GLHCKT()->channel;

   puts("");
   puts("--- Memory Graph ---");

   allocTotal = 0;
   for (i = 0; trace[i].name; ++i) {
      allocChannel = 0;

      if (!glhckContextGet()->allocs)
         break;

      if (trace[i].name) {
         for (iter = 0; (data = chckIterPoolIter(glhckContextGet()->allocs, &iter));) {
            if (!strcmp(data->channel, trace[i].name))
               allocChannel += data->size;
         }
      } else {
         allocChannel = allocTotal;
         puts("--------------------");
      }

      /* don't print zero channels */
      if (!allocChannel)
         continue;

      /* set color */
      if (allocChannel >= GLHCK_ALLOC_CRITICAL) _glhckRed();
      else if (allocChannel >= GLHCK_ALLOC_HIGH) _glhckBlue();
      else if (allocChannel >= GLHCK_ALLOC_AVERAGE) _glhckYellow();
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

      allocTotal += allocChannel;
   }

   _glhckGreen();
   printf( "%-13s : %zu\n", "Allocations", chckIterPoolCount(glhckContextGet()->allocs));
   _glhckNormal();

   puts("--------------------");
   puts("");
   return;
#else
   puts("-- Memory graph only available on debug build --");
#endif
}

/* vim: set ts=8 sw=3 tw=0 :*/
