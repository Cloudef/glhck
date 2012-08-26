#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GL/glhck.h"

int main(int argc, char **argv)
{
   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckServerInit(NULL, 5000))
      return EXIT_FAILURE;

   glhckMemoryGraph();

   while (1) {
      glhckServerUpdate();
   }

   glhckServerKill();

   puts("GLHCK channel should have few bytes for queue list,\n"
        "which is freed on glhckTerminate");
   glhckMemoryGraph();

   /* should cleanup all
    * objects as well */
   glhckTerminate();
   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
