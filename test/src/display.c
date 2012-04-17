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

void resize_callback(GLFWwindow window, int width, int height)
{
   glhckDisplayResize(width, height);
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckTexture *texture;
   glhckObject *cube;
   float spin = 0;

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

   if (!glhckDisplayCreate(800, 480, 0))
      return EXIT_FAILURE;

   RUNNING = 1;

#if 1
   if (!(texture = glhckTextureNew("../media/glhck.png",
               GLHCK_TEXTURE_DEFAULTS)))
      return EXIT_FAILURE;
   glhckTextureBind(texture);
#endif

   int i = 0;
   cube = glhckCubeNew(1);
   glhckObjectPositionf(cube, 0, 0, -10.0f);
   glhckMemoryGraph();

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;

      glfwPollEvents();

      /* rotate */
      glhckObjectRotatef(cube, 0, spin+=0.01f, 0);

      /* glhck drawing */
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
