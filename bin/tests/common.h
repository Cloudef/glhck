#include <glhck/glhck.h>
#include <stddef.h>

__attribute__((used))
size_t getStubRenderer()
{
   static size_t renderer = -1;

   if (renderer != (size_t)-1)
      return renderer;

   for (size_t i = 0; i < glhckRendererCount(); ++i) {
      const glhckRendererContextInfo *info;
      if ((info = glhckRendererGetContextInfo(i, NULL)) && info->type == GLHCK_CONTEXT_SOFTWARE) {
         renderer = i;
         break;
      }
   }

   assert(renderer != (size_t)-1);
   return renderer;
}

/* vim: set ts=8 sw=3 tw=0 :*/
