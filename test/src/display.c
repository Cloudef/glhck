#include <stdio.h>
#include <stdlib.h>
#include "GL/glfw3.h"
#include "GL/glhck.h"

static int RUNNING = 0;
int close_callback(GLFWwindow window)
{
   RUNNING = 0;
   return 1;
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckTexture *texture;
   glhckObject *cube;

   unsigned int   now          = 0;
   unsigned int   last         = 0;
   unsigned int   frameCounter = 0;
   unsigned int   FPS          = 0;
   unsigned int   fpsDelay     = 0;
   float          duration     = 0;
   float          delta        = 0;
   char           WIN_TITLE[256];

   if (!glfwInit())
      return EXIT_FAILURE;

   if (!(window = glfwOpenWindow(800, 480, GLFW_WINDOWED, "display test", NULL)))
      return EXIT_FAILURE;

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckCreateDisplay(800, 480, 0))
      return EXIT_FAILURE;

   RUNNING = 1;

#if 0
   if (!(texture = glhckTextureNew("test.png",
               GLHCK_TEXTURE_DEFAULTS)))
      return EXIT_FAILURE;
   glhckTextureBind(texture);
#endif

   int i = 0;
   cube = glhckCubeNew(1);

   glfwSetWindowCloseCallback(close_callback);
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;

      glfwPollEvents();

      /* glhck drawing */
      for (i = 0; i != 3500; ++i)
         glhckObjectDraw(cube);
      glhckRender();

      /* Actual swap and clear */
      glfwSwapBuffers();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (fpsDelay < glfwGetTime()) {
         if(duration > 0.0f)
            FPS = (float)frameCounter / duration;

         sprintf(WIN_TITLE, "OpenGL [FPS: %d]", FPS);
         glfwSetWindowTitle(window, WIN_TITLE);
         frameCounter = 0; fpsDelay = now + 1; duration = 0;
      }

      ++frameCounter;
      duration += delta;
   }

   glhckTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}
