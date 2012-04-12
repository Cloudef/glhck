#define _GNU_SOURCE
#include "internal.h"
#include <assert.h>  /* for assert */
#include <malloc.h>  /* for malloc */
#include <dlfcn.h>   /* for dlsym  */

#ifndef NDEBUG

#define GLHCK_ALLOC_CRITICAL 100 * 1048576 /* 100 MiB */
#define GLHCK_ALLOC_HIGH     80  * 1048576 /* 80  MiB */
#define GLHCK_ALLOC_AVERAGE  40  * 1048576 /* 40  MiB */

static void* (*_glhckRealMalloc)(size_t size) = NULL;

/* \brief internal malloc hook */
void* malloc(size_t size)
{
   if (!_glhckRealMalloc) _glhckRealMalloc = dlsym(RTLD_NEXT, "malloc");

   /* tracking code here */

   return _glhckRealMalloc(size);
}

#endif /* DEBUG */

/* \brief internal malloc function. */
void* _glhckMalloc(size_t size)
{
   void *ptr;
   CALL("%zu", size);

   if (!(ptr = malloc(size)))
      goto fail;

   RET("%p", ptr);
   return ptr;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to allocate %zu bytes", size);
   RET("%p", NULL);
   return NULL;
}

/* \brief internal calloc function. */
void* _glhckCalloc(unsigned int items, size_t size)
{
   void *ptr;
   CALL("%zu", size);

   if (!(ptr = calloc(items, size)))
      goto fail;

   RET("%p", ptr);
   return ptr;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to allocate %zu bytes",
         (size_t)items * size);
   RET("%p", NULL);
   return NULL;
}

/* \brief internal realloc function. */
void* _glhckRealloc(void *ptr, unsigned int old_items, unsigned int items, size_t size)
{
   void *ptr2;
   assert(ptr);
   CALL("%p, %u, %u, %zu", ptr, old_items, items, size);

   if (!(ptr2 = realloc(ptr, (size_t)items * size))) {
      if (!(ptr2 = _glhckMalloc(items * size)))
         goto fail;
      memcpy(ptr2, ptr, old_items * size);
      free(ptr);
      ptr = ptr2;
   } else ptr = ptr2;

   RET("%p", ptr);
   return ptr;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to reallocate %zu bytes",
         ((size_t)items * size) - ((size_t)old_items * size));
   RET("%p", NULL);
   return NULL;
}

/* \brief internal memcpy function. */
void* _glhckCopy(void *ptr, size_t size)
{
   void *ptr2;
   assert(ptr);
   CALL("%p, %zu", ptr, size);

   if (!(ptr2 = _glhckMalloc(size)))
      goto fail;

   memcpy(ptr2, ptr, size);
   RET("%p", ptr2);
   return ptr2;

fail:
   RET("%p", NULL);
   return NULL;
}

/* \brief internal free function. */
void _glhckFree(void *ptr)
{
   CALL("%p", ptr);
   if (!ptr) return;
   free(ptr);
}

/* \brief output memory usage graph */
#if 0
void glue_memory_graph(void)
{
   TRACE();
#ifdef DEBUG
   unsigned int i;

   dlPuts("");
   logWhite(); dlPuts("--- Memory Graph ---");
   i = 0; DL_ALLOC[ALLOC_TOTAL] = 0;
   for (; i != ALLOC_LAST; ++i)
   {
      if (i == ALLOC_TOTAL)
      { logWhite(); dlPuts("--------------------"); }

      if (DL_ALLOC[i] >= ALLOC_CRITICAL)     logRed();
      else if (DL_ALLOC[i] >= ALLOC_HIGH)    logBlue();
      else if (DL_ALLOC[i] >= ALLOC_AVERAGE) logYellow();
      else logGreen();
      dlPrint("%13s : ",    DL_ALLOCN[i]); logWhite();
      if (DL_ALLOC[i] / 1048576 != 0)
         dlPrint("%.2f MiB\n", (float)DL_ALLOC[i] / 1048576);
      else if (DL_ALLOC[i] / 1024 != 0)
         dlPrint("%.2f KiB\n", (float)DL_ALLOC[i] / 1024);
      else
         dlPrint("%lu B\n", DL_ALLOC[i]);

      /* increase total */
      DL_ALLOC[ALLOC_TOTAL] += DL_ALLOC[i];
   }
   dlPuts("--------------------"); logNormal();
   dlPuts("");

#else
   glue_puts("-- Memory graph only available on debug build --");
#endif
}
#endif

/* vim: set ts=8 sw=3 tw=0 :*/
