#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GL/glfw3.h"
#include "GL/glhck.h"

static int RUNNING = 0;
static int WIDTH = 800, HEIGHT = 480;
static int MOUSEX = 0, MOUSEY = 0;
static int LASTMOUSEX = 0, LASTMOUSEY = 0;
static int close_callback(GLFWwindow window)
{
   RUNNING = 0;
   return 1;
}

static void resize_callback(GLFWwindow window, int width, int height)
{
   WIDTH = width; HEIGHT = height;
   glhckDisplayResize(width, height);
}

static void mousepos_callback(GLFWwindow window, int mousex, int mousey)
{
   MOUSEX = mousex;
   MOUSEY = mousey;
   if (!LASTMOUSEX && !LASTMOUSEY) {
      LASTMOUSEX = MOUSEX;
      LASTMOUSEY = MOUSEY;
   }
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckCamera *camera;
   glhckObject *cube;
   int queuePrinted = 0;

   float          now          = 0;
   float          last         = 0;
   unsigned int   frameCounter = 0;
   unsigned int   FPS          = 0;
   unsigned int   fpsDelay     = 0;
   float          duration     = 0;
   float          delta        = 0;
   char           WIN_TITLE[256];
   memset(WIN_TITLE, 0, sizeof(WIN_TITLE));

   if (!glfwInit())
      return EXIT_FAILURE;

   glfwOpenWindowHint(GLFW_DEPTH_BITS, 24);
   if (!(window = glfwOpenWindow(WIDTH, HEIGHT, GLFW_WINDOWED, "display test", NULL)))
      return EXIT_FAILURE;

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(1);

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckDisplayCreate(WIDTH, HEIGHT, GLHCK_RENDER_AUTO))
      return EXIT_FAILURE;

   if (!glhckClientInit(NULL, 5000))
      return EXIT_FAILURE;

   RUNNING = 1;

   if (!(camera = glhckCameraNew()))
      return EXIT_FAILURE;

   glhckCameraRange(camera, 1.0f, 1000.0f);
   glhckMemoryGraph();

   if (!(cube = glhckCubeNew(1)))
      return EXIT_FAILURE;

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);
   glfwSetCursorPosCallback(mousepos_callback);
   glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_CAPTURED);

   glhckObjectPositionf(cube, -15.15f, 9999.999f, -360.0f);
   glhckObjectRotatef(cube, 0.0f, 25.0f, 1.0f);
   glhckObjectScalef(cube, -1.0f, -99999.0f, 250000.0f);
   glhckObjectColorb(cube, 0, 255, 255, 85);

   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;

      glfwPollEvents();

      if (!queuePrinted) {
         glhckPrintTextureQueue();
         glhckPrintObjectQueue();
         queuePrinted = 1;
      }

      glhckClientUpdate();
      glhckClientObjectRender(cube);
      glhckRender();

      /* Actual swap and clear */
      glfwSwapBuffers();
      glhckClear();

      if (fpsDelay < now) {
         if (duration > 0.0f) {
            FPS = (float)frameCounter / duration;
            snprintf(WIN_TITLE, sizeof(WIN_TITLE)-1, "OpenGL [FPS: %d]", FPS);
            glfwSetWindowTitle(window, WIN_TITLE);
            frameCounter = 0; fpsDelay = now + 1; duration = 0;
         }
      }

      ++frameCounter;
      duration += delta;
   }

   glhckClientKill();
   glhckObjectFree(cube);
   glhckCameraFree(camera);

   puts("GLHCK channel should have few bytes for queue list,\n"
        "which is freed on glhckTerminate");
   glhckMemoryGraph();

   /* should cleanup all
    * objects as well */
   glhckTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
