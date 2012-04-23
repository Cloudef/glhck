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

   float          now          = 0;
   float          last         = 0;
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

   if (!(texture = glhckTextureNew("../media/glhck.png",
               GLHCK_TEXTURE_DEFAULTS)))
      return EXIT_FAILURE;
   glhckTextureBind(texture);

#if 1
   cube = glhckCubeNew(1);
   //cube = glhckPlaneNew(1);
   glhckObjectPositionf(cube, 0, 0, -10.0f);
#else
   cube = glhckModelNew("../media/md_m.pmd", 1);
   glhckObjectPositionf(cube, 0, -10.0f, -50.0f);
#endif
   glhckMemoryGraph();

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;

      glfwPollEvents();

      /* rotate */
      glhckObjectRotatef(cube, 0, spin = spin + 10.0f * delta, 0);

      /* glhck drawing */
      glhckObjectDraw(cube);
      glhckRender();

      /* Actual swap and clear */
      glfwSwapBuffers();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (fpsDelay < now) {
         if (duration > 0.0f) {
            FPS = (float)frameCounter / duration;
            sprintf(WIN_TITLE, "OpenGL [FPS: %d]", FPS);
            glfwSetWindowTitle(window, WIN_TITLE);
            printf("FPS: %d\n", FPS);
            printf("DELTA: %f\n", delta);
            frameCounter = 0; fpsDelay = now + 1; duration = 0;
         }
      }

      ++frameCounter;
      duration += delta;
   }
   glhckObjectFree(cube);
   glhckTextureFree(texture);

   glhckTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}
