#include <stdlib.h>
#include <assert.h>
#include "common.h"

int main(int argc, char **argv)
{
   (void)argc, (void)argv;

   assert(!glhckInitialized());

   assert(glhckInit(argc, argv));
   assert(glhckInitialized());

   assert(glhckInit(argc, argv));
   assert(glhckInitialized());

   assert(!glhckDisplayCreated());
   assert(glhckDisplayCreate(32, 32, getStubRenderer()));
   assert(glhckDisplayCreated());

   assert(glhckDisplayCreate(32, 32, getStubRenderer()));
   assert(glhckDisplayCreated());

   glhckDisplayClose();
   assert(!glhckDisplayCreated());

   glhckTerminate();
   assert(!glhckInitialized());

   return EXIT_SUCCESS;
}
