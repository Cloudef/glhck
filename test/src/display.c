#include <stdio.h>
#include <stdlib.h>
#include "GL/glfw3.h"
#include "GL/glhck.h"

static int RUNNING = 0;
static int WIDTH = 800, HEIGHT = 480;
static int NO_NETWM_ACTIVE_SUPPORT = 1;
int close_callback(GLFWwindow window)
{
   RUNNING = 0;
   return 1;
}

void resize_callback(GLFWwindow window, int width, int height)
{
   WIDTH = width; HEIGHT = height;
   glhckDisplayResize(width, height);
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckTexture *texture;
   glhckObject *cube = NULL;
   glhckCamera *camera;
   float spin = 0;
   int mousex, mousey;
   kmVec3 cameraPos = { 0, 0, 0 };
   kmVec3 cameraRot = { 180, 180, 0 };

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

   if (!(window = glfwOpenWindow(WIDTH, HEIGHT, GLFW_WINDOWED, "display test", NULL)))
      return EXIT_FAILURE;

   /* Turn on VSYNC if driver allows */
   // glfwSwapInterval(1);

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckDisplayCreate(WIDTH, HEIGHT, 0))
      return EXIT_FAILURE;

   RUNNING = 1;

   /* test camera */
   if (!(camera = glhckCameraNew()))
      return EXIT_FAILURE;

   glhckCameraRange(camera, 0.1f, 1000.0f);

   /* this texture is useless when toggling PMD testing */
   if (!(texture = glhckTextureNew("../media/glhck.png",
               GLHCK_TEXTURE_DEFAULTS)))
      return EXIT_FAILURE;

#if 0
   cube = glhckCubeNew(1);
   //cube = glhckPlaneNew(1);
   //cube = glhckSpriteNew("../media/glhck.png", 100, GLHCK_TEXTURE_DEFAULTS);
   if (cube) glhckObjectSetTexture(cube, texture);
   //cube = glhckTextNew("/usr/share/fonts/TTF/mikachan.ttf", 8);
   cameraPos.z = -20.0f;
#else
   cube = glhckModelNew("../media/rin.pmd", 1);
   cameraPos.y =  10.0f;
   cameraPos.z = -40.0f;
#endif
   glhckMemoryGraph();

   glhckText *text = glhckTextNew(512, 512);
   if (!text) return EXIT_FAILURE;

   unsigned int font = glhckTextNewFont(text, "/usr/share/fonts/TTF/ipag.ttf");
   unsigned int font2 = glhckTextNewFont(text, "/usr/share/fonts/TTF/DejaVuSans.ttf");

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);
   glfwSetMousePos(window, WIDTH/2, HEIGHT/2);
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;

      /* old version of dwm has no NETWM_ACTIVE support
       * workaround until I switch to monsterwm */
      if (glfwGetWindowParam(window, GLFW_ACTIVE) ||
            NO_NETWM_ACTIVE_SUPPORT) {
         glfwPollEvents();
         glfwGetMousePos(window, &mousex, &mousey);

         cameraRot.y -= (float)(mousex - WIDTH/2)  / 7;
         cameraRot.x -= (float)(mousey - HEIGHT/2) / 7;

         if (glfwGetKey(window, GLFW_KEY_W)) {
            cameraPos.x += cos((cameraRot.y + 90) * kmPIOver180) * 25.0f * delta;
            cameraPos.z -= sin((cameraRot.y + 90) * kmPIOver180) * 25.0f * delta;
         } else if (glfwGetKey(window, GLFW_KEY_S)) {
            cameraPos.x -= cos((cameraRot.y + 90) * kmPIOver180) * 25.0f * delta;
            cameraPos.z += sin((cameraRot.y + 90) * kmPIOver180) * 25.0f * delta;
         }

         if (glfwGetKey(window, GLFW_KEY_A)) {
            cameraPos.x += cos((cameraRot.y + 180) * kmPIOver180) * 25.0f * delta;
            cameraPos.z -= sin((cameraRot.y + 180) * kmPIOver180) * 25.0f * delta;
         } else if (glfwGetKey(window, GLFW_KEY_D)) {
            cameraPos.x -= cos((cameraRot.y + 180) * kmPIOver180) * 25.0f * delta;
            cameraPos.z += sin((cameraRot.y + 180) * kmPIOver180) * 25.0f * delta;
         }

         glfwSetMousePos(window, WIDTH/2, HEIGHT/2);
      }

      /* update the camera */
      glhckCameraUpdate(camera);

      /* rotate */
      glhckCameraPosition(camera, &cameraPos);
      glhckCameraTargetf(camera, cameraPos.x, cameraPos.y, cameraPos.z + 1);
      glhckCameraRotate(camera, &cameraRot);
      //glhckObjectRotatef(cube, 0, spin = spin + 10.0f * delta, 0);

      /* glhck drawing */
      glhckObjectDraw(cube);
      glhckRender();

      glhckTextDraw(text, font, 42, 54, 200, "愛してるGLHCK", NULL);
      glhckTextDraw(text, font2, 32, 54, 240, "Äöäö DejaVuSans perkele", NULL);
      glhckTextDraw(text, font, 18, 0, 0, "SADASD", NULL);
      glhckTextRender(text);

      /* Actual swap and clear */
      glfwSwapBuffers();
      glhckClear();

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

   /* should cleanup all
    * objects as well */
   glhckTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
