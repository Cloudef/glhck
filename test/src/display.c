#include <stdio.h>
#include <stdlib.h>
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

static void handleCamera(GLFWwindow window, float delta, kmVec3 *cameraPos, kmVec3 *cameraRot) {
   if (glfwGetKey(window, GLFW_KEY_W)) {
      cameraPos->x += cos((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->z -= sin((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->y += cos((cameraRot->x + 90) * kmPIOver180) * 25.0f * delta;
   } else if (glfwGetKey(window, GLFW_KEY_S)) {
      cameraPos->x -= cos((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->z += sin((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->y -= cos((cameraRot->x + 90) * kmPIOver180) * 25.0f * delta;
   }

   if (glfwGetKey(window, GLFW_KEY_A)) {
      cameraPos->x += cos((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      cameraPos->z -= sin((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
   } else if (glfwGetKey(window, GLFW_KEY_D)) {
      cameraPos->x -= cos((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      cameraPos->z += sin((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
   }

   cameraRot->y -= (float)(MOUSEX - LASTMOUSEX) / 7;
   cameraRot->x -= (float)(MOUSEY - LASTMOUSEY) / 7;
   LASTMOUSEX = MOUSEX;
   LASTMOUSEY = MOUSEY;
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckTexture *texture;
   glhckObject *cube = NULL, *sprite = NULL, *sprite2 = NULL;
   glhckCamera *camera;
   const kmAABB *aabb;
   kmVec3 cameraPos = { 0, 0, 0 };
   kmVec3 cameraRot = { 180, 180, 0 };
   float spin = 0.0f, spinRadius;

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

   glfwOpenWindowHint(GLFW_DEPTH_BITS, 24);
   if (!(window = glfwOpenWindow(WIDTH, HEIGHT, GLFW_WINDOWED, "display test", NULL)))
      return EXIT_FAILURE;

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(1);

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckDisplayCreate(WIDTH, HEIGHT, GLHCK_RENDER_AUTO))
      return EXIT_FAILURE;

   RUNNING = 1;

   /* test camera */
   if (!(camera = glhckCameraNew()))
      return EXIT_FAILURE;

   glhckCameraRange(camera, 0.5f, 1000.0f);

   /* this texture is useless when toggling PMD testing */
   if (!(texture = glhckTextureNew("test/media/glhck.png",
               GLHCK_TEXTURE_DEFAULTS)))
      return EXIT_FAILURE;

   sprite  = glhckSpriteNewFromFile("test/media/glhck.png", 1.0f, GLHCK_TEXTURE_DEFAULTS);
   sprite2 = glhckSpriteNewFromFile("test/media/glhck.png", 1.0f, GLHCK_TEXTURE_DEFAULTS);
   //sprite2 = glhckObjectCopy(sprite); /* FIXME */
#if 1
   cube = glhckCubeNew(1.0f);
   if (cube) glhckObjectSetTexture(cube, texture);
   cameraPos.z = -20.0f;
#else
   cube = glhckModelNew("test/media/madoka/md_m.pmd", 1.0f);
   cameraPos.y =  10.0f;
   cameraPos.z = -40.0f;
#endif

   glhckObjectSetMaterialFlags(cube, GLHCK_MATERIAL_DRAW_AABB |
                                     GLHCK_MATERIAL_DRAW_OBB);
   glhckMemoryGraph();

   glhckText *text = glhckTextNew(512, 512);
   if (!text) return EXIT_FAILURE;

   unsigned int font = glhckTextNewFont(text, "/usr/share/fonts/TTF/ipag.ttf");
   unsigned int font2 = glhckTextNewFont(text, "/usr/share/fonts/TTF/DejaVuSans.ttf");

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);
   glfwSetMousePosCallback(mousepos_callback);
   glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_CAPTURED);
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;

      glfwPollEvents();
      handleCamera(window, delta, &cameraPos, &cameraRot);

      /* update the camera */
      glhckCameraUpdate(camera);

      /* rotate */
      glhckCameraPosition(camera, &cameraPos);
      glhckCameraTargetf(camera, cameraPos.x, cameraPos.y, cameraPos.z + 1);
      glhckCameraRotate(camera, &cameraRot);

      glhckObjectRotatef(cube, 30.0f * delta, 0, 30.0f * delta);
      glhckObjectRotatef(sprite, 0, 30.0f * delta, 0);

      /* glhck drawing */
      glhckObjectDraw(cube);

      /* do spinning effect */
      aabb = glhckObjectGetAABB(cube);
      spinRadius = aabb->max.x>aabb->max.z?
         aabb->max.x>aabb->max.y?aabb->max.x:aabb->max.y:
         aabb->max.z>aabb->max.y?aabb->max.z:aabb->max.y;
      spinRadius += 5.0f;

      glhckObjectPositionf(sprite,
            spinRadius*sin((glhckObjectGetRotation(cube))->x/2),
            (glhckObjectGetOBB(cube))->max.y,
            spinRadius*cos((glhckObjectGetRotation(cube))->x/2));
      glhckObjectDraw(sprite);

      /* TODO: save object state on draw call
       * so we don't need sprite2 here! */
      glhckObjectPositionf(sprite2,
            spinRadius*sin((glhckObjectGetRotation(cube))->z/2),
            (spinRadius*cos((glhckObjectGetRotation(cube))->z/2)+aabb->max.y/2),
            (glhckObjectGetOBB(cube))->max.z);
      glhckObjectDraw(sprite2);

      glhckRender();

      glhckTextDraw(text, font, 42, 54, 200, "愛してるGLHCK", NULL);
      glhckTextDraw(text, font2, 32, 54, 240, "Äöäö DejaVuSans perkele", NULL);
      glhckTextDraw(text, font, 18, 0, 0, "SADASD!?,.:;", NULL);
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
