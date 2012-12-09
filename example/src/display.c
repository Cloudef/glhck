#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef GLHCK_USE_GLES1
#  define GLFW_INCLUDE_ES1
#endif
#ifdef GLHCK_USE_GLES2
#  define GLFW_INCLUDE_ES2
#endif
#include "GL/glfw3.h"
#include "glhck/glhck.h"

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

static void handleCamera(GLFWwindow window, float delta, kmVec3 *cameraPos, kmVec3 *cameraRot,
                         glhckProjectionType *projectionType) {
   *projectionType = glfwGetKey(window, GLFW_KEY_P) ?
         GLHCK_PROJECTION_ORTHOGRAPHIC : GLHCK_PROJECTION_PERSPECTIVE;
   if (glfwGetKey(window, GLFW_KEY_W)) {
      cameraPos->x -= cos((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->z += sin((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->y += cos((cameraRot->x + 90) * kmPIOver180) * 25.0f * delta;
   } else if (glfwGetKey(window, GLFW_KEY_S)) {
      cameraPos->x += cos((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->z -= sin((cameraRot->y + 90) * kmPIOver180) * 25.0f * delta;
      cameraPos->y -= cos((cameraRot->x + 90) * kmPIOver180) * 25.0f * delta;
   }

   if (glfwGetKey(window, GLFW_KEY_A)) {
      cameraPos->x -= cos((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      cameraPos->z += sin((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
   } else if (glfwGetKey(window, GLFW_KEY_D)) {
      cameraPos->x += cos((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
      cameraPos->z -= sin((cameraRot->y + 180) * kmPIOver180) * 25.0f * delta;
   }

   cameraRot->z = 0;
   cameraRot->z -=  glfwGetKey(window, GLFW_KEY_Z) * (float)(MOUSEX - LASTMOUSEX) / 7;
   cameraRot->y -= !glfwGetKey(window, GLFW_KEY_Z) * (float)(MOUSEX - LASTMOUSEX) / 7;
   cameraRot->x += (float)(MOUSEY - LASTMOUSEY) / 7;

   LASTMOUSEX = MOUSEX;
   LASTMOUSEY = MOUSEY;
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckTexture *texture = NULL;
   glhckObject *cube, *sprite, *cube2, *sprite3, *camObj;
   glhckCamera *camera;
   const kmAABB *aabb;
   kmVec3 cameraPos = { 0, 0, 0 };
   kmVec3 cameraRot = { 0, 0, 0 };
   float spinRadius;
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

   puts("-!- glfwinit");

#if GLHCK_USE_GLES1
   glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
   glfwWindowHint(GLFW_DEPTH_BITS, 16);
#elif GLHCK_USE_GLES2
   glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
   glfwWindowHint(GLFW_DEPTH_BITS, 16);
#else
   glfwWindowHint(GLFW_DEPTH_BITS, 24);
#endif
   if (!(window = glfwCreateWindow(WIDTH, HEIGHT, GLFW_WINDOWED, "display test", NULL)))
      return EXIT_FAILURE;

   glfwMakeContextCurrent(window);
   puts("-!- window create");

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(1);

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   puts("-!- glhck init");

   if (!glhckDisplayCreate(WIDTH, HEIGHT, GLHCK_RENDER_AUTO))
      return EXIT_FAILURE;

   puts("-!- glhck display create");

   RUNNING = 1;

   /* test camera */
   if (!(camera = glhckCameraNew()))
      return EXIT_FAILURE;

   glhckCameraRange(camera, 1.0f, 1000.0f);
   camObj = glhckCameraGetObject(camera);

   if (!(texture = glhckTextureNew("example/media/glhck.png", GLHCK_TEXTURE_DEFAULTS)))
      return EXIT_FAILURE;

   sprite  = glhckSpriteNew(texture, 0, 0);
   sprite3 = glhckSpriteNewFromFile("example/media/glhck.png", 0, 0, GLHCK_TEXTURE_DEFAULTS);
   cube2   = glhckCubeNew(1.0f);
   glhckObjectMaterialFlags(cube2, glhckObjectGetMaterialFlags(cube2)|
         GLHCK_MATERIAL_DRAW_OBB|GLHCK_MATERIAL_DRAW_AABB);
   glhckObjectScalef(cube2, 1.0f, 1.0f, 2.0f);
   glhckObjectSetTexture(cube2, texture);

   // FIXME: copy is broken again \o/
   //sprite3 = glhckObjectCopy(sprite);
   glhckObjectScalef(sprite, 0.05f, 0.05f, 0.05f);
   glhckObjectPositionf(sprite3, 64*2, 48*2, 0);

#define SKIP_MMD    1
#define SKIP_OCTM   1
#define SKIP_ASSIMP 0

#if SKIP_MMD
#  define MMD_PATH ""
#else
#  define MMD_PATH "example/media/madoka/md_m.pmd"
#endif

#if SKIP_OCTM
#  define OCTM_PATH ""
#else
#  define OCTM_PATH "example/media/ambulance/ambulance.ctm"
#endif

#if SKIP_ASSIMP
#  define ASSIMP_PATH ""
#else
#  define ASSIMP_PATH "example/media/base/BaseV1.obj"
#endif

   if ((cube = glhckModelNewEx(MMD_PATH, 1.0f, GLHCK_INDEX_SHORT, GLHCK_VERTEX_V3S))) {
      cameraPos.y =  10.0f;
      cameraPos.z = -40.0f;
   } else if ((cube = glhckModelNewEx(OCTM_PATH, 5.0f, GLHCK_INDEX_SHORT, GLHCK_VERTEX_V3S))) {
      cameraPos.y =  10.0f;
      cameraPos.z = -40.0f;
   } else if ((cube = glhckModelNewEx(ASSIMP_PATH, 10.0f, GLHCK_INDEX_SHORT, GLHCK_VERTEX_V3S))) {
      cameraPos.y =  10.0f;
      cameraPos.z = -40.0f;
   } else if ((cube = glhckCubeNew(1.0f))) {
      glhckObjectSetTexture(cube, texture);
      cameraPos.z = -20.0f;
      glhckObjectScalef(cube, 1.0f, 1.0f, 2.0f);
   } else return EXIT_FAILURE;

   unsigned int flags = glhckObjectGetMaterialFlags(cube);
   flags |= GLHCK_MATERIAL_DRAW_AABB;
   flags |= GLHCK_MATERIAL_DRAW_OBB;
   glhckObjectMaterialFlags(cube, flags);

   glhckMemoryGraph();

   glhckText *text = glhckTextNew(512, 512);
   if (!text) return EXIT_FAILURE;

   unsigned int font  = glhckTextNewFont(text, "example/media/sazanami-gothic.ttf");
   unsigned int font2 = glhckTextNewFont(text, "example/media/DejaVuSans.ttf");

   glhckRtt *rtt = glhckRttNew(124, 24, GLHCK_RTT_RGBA);
   glhckObject *rttText = NULL;
   glhckTextColor(text, 255, 255, 255, 255);
   glhckTextDraw(text, font, 24, 0, 20, "RTT Text", NULL);
   if (rtt) {
      glhckRttBegin(rtt);
      glhckTextRender(text);
      glhckRttFillData(rtt);
      glhckRttEnd(rtt);
      rttText = glhckSpriteNew(glhckRttGetTexture(rtt), 6, 1);
      glhckObjectMaterialFlags(rttText, glhckObjectGetMaterialFlags(rttText)|GLHCK_MATERIAL_ALPHA);
      glhckObjectPositionf(rttText, 2.0f, 2.5f, 0);
      glhckRttFree(rtt);
      glhckObjectAddChildren(cube2, rttText);
   }

   glfwSetWindowCloseCallback(close_callback);
   glfwSetWindowSizeCallback(resize_callback);
   glfwSetCursorPosCallback(mousepos_callback);
   glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_CAPTURED);

   glhckProjectionType projectionType = GLHCK_PROJECTION_PERSPECTIVE;

   float xspin = 0.0f;
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;

      glfwPollEvents();
      handleCamera(window, delta, &cameraPos, &cameraRot, &projectionType);
      glhckCameraProjection(camera, projectionType);

      /* update the camera */
      if (glfwGetKey(window, GLFW_KEY_O)) {
         kmMat4 identity;
         kmMat4Identity(&identity);
         glhckRenderSetProjection(&identity);
      } else if (glfwGetKey(window, GLFW_KEY_I)) {
         kmMat4 mat2d, pos;
         glhckCameraUpdate(camera);
         kmMat4Translation(&pos, -cameraPos.x, -cameraPos.y, -cameraPos.z);
         kmMat4Scaling(&mat2d, -2.0f/WIDTH, 2.0f/HEIGHT, 0.0f);
         kmMat4Multiply(&mat2d, &mat2d, &pos);
         glhckRenderSetProjection(&mat2d);
      } else {
         glhckCameraUpdate(camera);
      }

      /* rotate */
      glhckObjectPosition(camObj, &cameraPos);
      glhckObjectTargetf(camObj, cameraPos.x, cameraPos.y, cameraPos.z + 1);
      glhckObjectRotate(camObj, &cameraRot);

      glhckObjectTarget(cube, glhckObjectGetPosition(sprite));
      glhckObjectDraw(cube);

      /* do spinning effect */
      aabb = glhckObjectGetAABB(cube);
      spinRadius = aabb->max.x>aabb->max.z?
         aabb->max.x>aabb->max.y?aabb->max.x:aabb->max.y:
         aabb->max.z>aabb->max.y?aabb->max.z:aabb->max.y;
      spinRadius += 5.0f;

      xspin += 30.0f * delta;
      glhckObjectPositionf(sprite,
            spinRadius*sin(xspin/8),
            spinRadius*cos(xspin/12),
            spinRadius);

      glhckObjectPositionf(cube2,
            spinRadius*sin(xspin/20),
            spinRadius*cos(xspin/20),
            spinRadius*sin(xspin/40));
      glhckObjectTarget(cube2, glhckObjectGetPosition(cube));
      glhckObjectTarget(sprite3, glhckObjectGetPosition(camObj));

      glhckObjectDraw(sprite);
      glhckObjectDraw(cube2);
      glhckObjectDraw(sprite3);
      if (rttText) glhckObjectDraw(rttText);

      if (!queuePrinted) {
         glhckPrintTextureQueue();
         glhckPrintObjectQueue();
         queuePrinted = 1;
      }

      glhckRender();

      /* draw frustum */
      glhckFrustumRender(glhckCameraGetFrustum(camera),
                         glhckCameraGetViewMatrix(camera));

      glhckTextColor(text, 255, 255, 255, 255);
      glhckTextDraw(text, font2, 18,         0,  HEIGHT-4, WIN_TITLE, NULL);
      glhckTextDraw(text, font,  42,        25, HEIGHT-80, "愛してるGLHCK", NULL);
      glhckTextDraw(text, font2, 32,        25, HEIGHT-40, "Äöäö DejaVuSans perkele", NULL);
      glhckTextDraw(text, font2, 18,         0,        18, "SADASD!?,.:;", NULL);
      glhckTextRender(text);

      glhckTextColor(text, 255, 0, 0, 255);
      glhckTextDraw(text, font2, 12, WIDTH-100,        18, "Wall of text", NULL);
      glhckTextRender(text);

      /* Actual swap and clear */
      glfwSwapBuffers(window);
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

   /* check that everything is freed
    * like should */
   glhckObjectFree(cube);
   glhckObjectFree(sprite);
   glhckObjectFree(cube2);
   glhckObjectFree(sprite3);
   if (texture) glhckTextureFree(texture);
   glhckCameraFree(camera);
   glhckTextFree(text);

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
