#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef GLHCK_USE_GLES1
#  define GLFW_INCLUDE_ES1
#endif
#ifdef GLHCK_USE_GLES2
#  define GLFW_INCLUDE_ES2
#endif
#include "GLFW/glfw3.h"
#include "glhck/glhck.h"

static int RUNNING = 0;
static int WIDTH = 800, HEIGHT = 480;
static double MOUSEX = 0, MOUSEY = 0;
static double LASTMOUSEX = 0, LASTMOUSEY = 0;

static void error_callback(int error, const char *msg)
{
   (void)error;
   printf("-!- GLFW: %s\n", msg);
}

static void close_callback(GLFWwindow* window)
{
   (void)window;
   RUNNING = 0;
}

static void resize_callback(GLFWwindow* window, int width, int height)
{
   (void)window;
   WIDTH = width; HEIGHT = height;
   glhckDisplayResize(width, height);
}

static void mousepos_callback(GLFWwindow* window, double mousex, double mousey)
{
   (void)window;
   MOUSEX = mousex;
   MOUSEY = mousey;
   if (!LASTMOUSEX && !LASTMOUSEY) {
      LASTMOUSEX = MOUSEX;
      LASTMOUSEY = MOUSEY;
   }
}

static void handleCamera(GLFWwindow* window, float delta, kmVec3 *cameraPos, kmVec3 *cameraRot,
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
   GLFWwindow* window;
   glhckTexture *texture;
   glhckMaterial *material;
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

   glfwSetErrorCallback(error_callback);
   if (!glfwInit())
      return EXIT_FAILURE;

   puts("-!- glfwinit");

   glhckCompileFeatures features;
   glhckGetCompileFeatures(&features);
   if (features.render.glesv1 || features.render.glesv2) {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      glfwWindowHint(GLFW_DEPTH_BITS, 16);
   }
   if (features.render.glesv2) {
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
   }
   if (features.render.opengl) {
      glfwWindowHint(GLFW_DEPTH_BITS, 24);
   }
   if (!(window = glfwCreateWindow(WIDTH, HEIGHT, "display test", NULL, NULL)))
      return EXIT_FAILURE;

   glfwMakeContextCurrent(window);
   puts("-!- window create");

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(0);

   if (!glhckContextCreate(argc, argv))
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

   if (!(texture = glhckTextureNewFromFile("example/media/glhck.png", NULL, NULL)))
      return EXIT_FAILURE;

   if (!(material = glhckMaterialNew(texture)))
      return EXIT_FAILURE;

   glhckTextureFree(texture);

   sprite  = glhckSpriteNew(texture, 0, 0);
   cube2   = glhckCubeNew(1.0f);
   glhckObjectDrawAABB(cube2, 1);
   glhckObjectDrawOBB(cube2, 1);
   glhckObjectScalef(cube2, 1.0f, 1.0f, 2.0f);
   glhckObjectMaterial(cube2, material);

   sprite3 = glhckObjectCopy(sprite);
   glhckObjectScalef(sprite, 0.05f, 0.05f, 0.05f);
   glhckObjectPositionf(sprite3, 64*2, 48*2, 0);

#define SKIP_MMD    0
#define SKIP_OCTM   0
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

/* cmake -DGLHCK_IMPORT_ASSIMP=YES */
#if SKIP_ASSIMP
#  define ASSIMP_PATH ""
#else
//#  define ASSIMP_PATH "example/media/chaosgate/chaosgate.obj"
//#  define ASSIMP_PATH "example/media/room/room.obj"
#  define ASSIMP_PATH "example/media/player.x"
#endif

   glhckImportModelParameters animatedParams;
   memcpy(&animatedParams, glhckImportDefaultModelParameters(), sizeof(glhckImportModelParameters));
   animatedParams.animated = 1;

   if ((cube = glhckModelNewEx(MMD_PATH, 1.0f, NULL, GLHCK_INDEX_SHORT, GLHCK_VERTEX_V3S))) {
      cameraPos.y =  10.0f;
      cameraPos.z = -40.0f;
   } else if ((cube = glhckModelNewEx(OCTM_PATH, 5.0f, NULL, GLHCK_INDEX_SHORT, GLHCK_VERTEX_V3S))) {
      cameraPos.y =  10.0f;
      cameraPos.z = -40.0f;
      glhckObjectPositionf(cube, 0.0f, 5.0f, 0.0f);
   } else if ((cube = glhckModelNewEx(ASSIMP_PATH, 0.1f, &animatedParams, GLHCK_INDEX_SHORT, GLHCK_VERTEX_V3S))) {
      glhckMaterial *mat;
      glhckTexture *tex;
      if ((tex = glhckTextureNewFromFile("example/media/texture-b.png", NULL, NULL))) {
         if ((mat = glhckMaterialNew(tex))) glhckObjectMaterial(cube, mat);
         glhckTextureFree(tex);
      }
      glhckObjectMovef(cube, 0, 15, 0);
      cameraPos.y =  10.0f;
      cameraPos.z = -40.0f;
   } else if ((cube = glhckCubeNew(1.0f))) {
      glhckObjectMaterial(cube, material);
      cameraPos.z = -20.0f;
   } else return EXIT_FAILURE;

#define SHADOW 0
#if SHADOW
   glhckShader *shader = glhckShaderNew(NULL, "VSM.GLhck.Lighting.ShadowMapping.Unpacking.Fragment", NULL);
   glhckShader *depthShader = glhckShaderNew(NULL, "VSM.GLhck.Depth.Packing.Fragment", NULL);
   glhckShader *depthRenderShader = glhckShaderNew(NULL, "VSM.GLhck.DepthRender.Unpacking.Fragment", NULL);
   glhckShaderSetUniform(shader, "ShadowMap", 1, &((int[]){1}));

   int sW = 1024, sH = 1024;
   glhckRenderbuffer *depthBuffer = glhckRenderbufferNew(sW, sH, GLHCK_DEPTH_COMPONENT16);
   glhckTexture *depthColorMap = glhckTextureNew();
   glhckTextureCreate(depthColorMap, GLHCK_TEXTURE_2D, 0, sW, sH, 0, 0, GLHCK_RGBA, GLHCK_DATA_UNSIGNED_BYTE, 0, NULL);
   glhckFramebuffer *fbo = glhckFramebufferNew(GLHCK_FRAMEBUFFER);
   glhckFramebufferAttachTexture(fbo, depthColorMap, GLHCK_COLOR_ATTACHMENT0);
   glhckFramebufferAttachRenderbuffer(fbo, depthBuffer, GLHCK_DEPTH_ATTACHMENT);
   glhckFramebufferRecti(fbo, 0, 0, sW, sH);

   glhckObject *screen = glhckSpriteNew(depthColorMap, 128, 128);
   glhckObjectShader(screen, depthRenderShader);
   glhckObjectMaterialFlags(screen, 0);
#endif

   glhckObject *plane = glhckPlaneNew(200.0f, 200.0f);
   glhckObjectMaterial(plane, material);
   glhckObjectPositionf(plane, 0.0f, -0.1f, 0.0f);
   glhckObjectRotationf(plane, -90.0f, 0.0f, 0.0f);

   glhckObjectDrawAABB(cube, 1);
   glhckObjectDrawOBB(cube, 1);
   glhckObjectDrawSkeleton(cube, 0);

   glhckText *text = glhckTextNew(512, 512);
   if (!text) return EXIT_FAILURE;

   unsigned int font  = glhckTextFontNew(text, "example/media/sazanami-gothic.ttf");
   unsigned int font2 = glhckTextFontNew(text, "example/media/DejaVuSans.ttf");
   unsigned int font3 = glhckTextFontNewKakwafont(text, NULL);

   glhckObject *rttText = NULL, *rttText2 = NULL;
   glhckTextColorb(text, 255, 255, 255, 255);
   if ((rttText = glhckTextPlane(text, font2, 42, "RTT Text", NULL))) {
      glhckObjectScalef(rttText, 0.05f, 0.05f, 1.0f); /* scale to fit our 3d world */
      glhckObjectPositionf(rttText, 0, 2, 0);
      glhckObjectAddChild(cube2, rttText);

      /* ignore lighting */
      glhckMaterialOptions(glhckObjectGetMaterial(rttText), 0);
   }
   glhckTextClear(text);

   if ((rttText2 = glhckTextPlane(text, font, 42, "GLHCKあいしてる", NULL))) {
      glhckObjectRotatef(rttText2, 0, 210, 0);
      glhckObjectPositionf(rttText2, 200, 0, 500);

      /* ignore lighting */
      glhckMaterialOptions(glhckObjectGetMaterial(rttText2), 0);
   }
   glhckTextClear(text);

   glfwSetWindowCloseCallback(window, close_callback);
   glfwSetWindowSizeCallback(window, resize_callback);
   glfwSetCursorPosCallback(window, mousepos_callback);

   if (features.render.opengl) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
   }

   glhckProjectionType projectionType = GLHCK_PROJECTION_PERSPECTIVE;

   int numLights = 1, li;
   glhckLight *light[numLights];
   for (li = 0; li != numLights; ++li) {
      light[li] = glhckLightNew();
      glhckLightCutoutf(light[li], 45.0f, 0.0f);
      glhckLightPointLightFactor(light[li], 0.0f);
      glhckLightAttenf(light[li], 0.0, 0.0, 0.01);
      glhckLightColorb(light[li], rand()%255, rand()%255, rand()%255, 255);
      glhckObject *c = glhckEllipsoidNew(2.0f, 4.0f, 2.0f);
      glhckObjectAddChild(glhckLightGetObject(light[li]), c);
      glhckObjectMaterial(c, material);
      glhckObjectFree(c);
   }

   glhckMemoryGraph();

   float xspin = 0.0f;
   unsigned int numBones = 0;
   glhckAnimator *animator = NULL;
   glhckBone **bones = glhckObjectBones(cube, &numBones);

   if (bones) {
      if ((animator = glhckAnimatorNew())) {
         glhckAnimatorAnimation(animator, glhckObjectAnimations(cube, NULL)[0]);
         glhckAnimatorInsertBones(animator, bones, numBones);
         glhckAnimatorUpdate(animator, xspin);
         glhckAnimatorTransform(animator, cube);
      }
   }

   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;
      glhckRenderTime(now);

      glfwPollEvents();
      handleCamera(window, delta, &cameraPos, &cameraRot, &projectionType);
      glhckCameraProjection(camera, projectionType);

      /* update the camera */
      if (glfwGetKey(window, GLFW_KEY_O)) {
         kmMat4 identity;
         kmMat4Identity(&identity);
         glhckRenderProjection(&identity);
      } else if (glfwGetKey(window, GLFW_KEY_I)) {
         kmMat4 mat2d, pos;
         glhckCameraUpdate(camera);
         kmMat4Translation(&pos, -cameraPos.x, -cameraPos.y, -cameraPos.z);
         kmMat4Scaling(&mat2d, -2.0f/WIDTH, 2.0f/HEIGHT, 0.0f);
         kmMat4Multiply(&mat2d, &mat2d, &pos);
         glhckRenderProjection(&mat2d);
      } else {
         glhckCameraUpdate(camera);
      }

      //glhckMaterialTextureOffsetf(material, sin(xspin*0.01), 0);
      //glhckMaterialTextureScalef(material, sin(xspin*0.01), 1);

      /* rotate */
      glhckObjectPosition(camObj, &cameraPos);
      glhckObjectTargetf(camObj, cameraPos.x, cameraPos.y, cameraPos.z + 1);
      glhckObjectRotate(camObj, &cameraRot);

      /* target */
      //glhckObjectTarget(cube, glhckObjectGetPosition(sprite));

      /* do spinning effect */
      aabb = glhckObjectGetAABB(cube);
      spinRadius = aabb->max.x>aabb->max.z?
         aabb->max.x>aabb->max.y?aabb->max.x:aabb->max.y:
         aabb->max.z>aabb->max.y?aabb->max.z:aabb->max.y;
      spinRadius += 5.0f;

      if (glfwGetKey(window, GLFW_KEY_SPACE)) xspin += 30.0f * delta;
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

      if (animator) {
         glhckAnimatorUpdate(animator, xspin*0.05);
         glhckAnimatorTransform(animator, cube);
      }

      for (li = 0; li != numLights; ++li) {
#if 1
         glhckObjectPositionf(glhckLightGetObject(light[li]),
               sin(xspin*(li+1)*0.01)*85.0f, sin(xspin*0.01)*30.0f+60.0f, cos(xspin*(li+1)*0.01)*85.0f);
#else
         glhckObjectPositionf(glhckLightGetObject(light[li]),
               sin(li+1)*80.0f, sin(xspin*0.1)*30.0f+45.0f, cos(li+1)*80.0f);
#endif
         glhckObjectTargetf(glhckLightGetObject(light[li]), 0, 0, 0);

#if SHADOW
         glhckRenderBlendFunc(GLHCK_ZERO, GLHCK_ZERO);
         glhckRenderPassFlags(GLHCK_PASS_DEPTH | GLHCK_PASS_CULL);
         glhckRenderPassShader(depthShader);

         glhckObjectDraw(cube);
         glhckObjectDraw(plane);

         glhckCameraRange(camera, 5.0f, 250.0f);
         glhckLightBeginProjectionWithCamera(light[li], camera);
         glhckLightBind(light[li]);
         glhckFramebufferBegin(fbo);
         glhckRenderClear(GLHCK_DEPTH_BUFFER | GLHCK_COLOR_BUFFER);
         glhckRender();
         glhckFramebufferEnd(fbo);
         glhckLightEndProjectionWithCamera(light[li], camera);

         glActiveTexture(GL_TEXTURE1);
         glhckTextureBind(depthColorMap);
         glActiveTexture(GL_TEXTURE0);

         glhckCameraRange(camera, 1.0f, 1000.0f);
         glhckCameraUpdate(camera);

         glhckRenderPassFlags(GLHCK_PASS_DEFAULTS);
         glhckRenderPassShader(shader);
#else
         glhckLightBeginProjectionWithCamera(light[li], camera);
         //glhckLightBind(light[li]);
         glhckLightEndProjectionWithCamera(light[li], camera);
#endif

         if (rttText2) glhckObjectDraw(rttText2);
         glhckObjectDraw(sprite);
         glhckObjectDraw(sprite3);
         glhckObjectDraw(cube2);
         glhckObjectDraw(cube);
         glhckObjectDraw(plane);
         glhckObjectDraw(glhckLightGetObject(light[li]));

         if (li) glhckRenderBlendFunc(GLHCK_ONE, GLHCK_ONE);
         glhckRender();
      }
      glhckRenderBlendFunc(GLHCK_ZERO, GLHCK_ZERO);

      /* debug */
      if (!queuePrinted) {
         glhckRenderPrintTextureQueue();
         glhckRenderPrintObjectQueue();
         queuePrinted = 1;
      }

      /* render frustum */
      glhckFrustumRender(glhckCameraGetFrustum(camera));

#if SHADOW
      kmMat4 mat2d;
      kmMat4Scaling(&mat2d, -2.0f/WIDTH, 2.0f/HEIGHT, 0.0f);
      glhckRenderProjectionOnly(&mat2d);
      glhckObjectPositionf(screen, WIDTH/2.0f-128.0f/2.0f, HEIGHT/2.0f-128.0f/2.0f, 0);
      glhckObjectRender(screen);
#endif

      glhckRenderStatePush2D(WIDTH, HEIGHT, -1000, 1000);
      kmVec3 oldPos = *glhckObjectGetPosition(cube);
      kmVec3 oldScale = *glhckObjectGetScale(cube);
      glhckRenderPass(glhckRenderGetPass() & ~GLHCK_PASS_LIGHTING);
      glhckObjectScalef(cube, oldScale.x*5, oldScale.y*5, oldScale.z*5);
      glhckObjectPositionf(cube, WIDTH/2, HEIGHT/2, 0.0f);
      glhckObjectDraw(cube);
      glhckRender();
      glhckObjectPositionf(cube, 45, 85, 0.0f);
      glhckObjectDraw(cube);
      glhckRender();
      glhckObjectScale(cube, &oldScale);
      glhckObjectPosition(cube, &oldPos);
      glhckRenderStatePop();

      /* draw some text */
      glhckTextColorb(text, 255, 255, 255, 255);
      glhckTextStash(text, font2, 18,         0,  HEIGHT-4, WIN_TITLE, NULL);
      glhckTextStash(text, font,  42,        25, HEIGHT-80, "愛してるGLHCK", NULL);
      glhckTextStash(text, font2, 32,        25, HEIGHT-40, "Äöäö DejaVuSans perkele", NULL);
      glhckTextStash(text, font3, 12,         0,        12, "SADASD!?,.:;\01\02\03\04\05\06", NULL);
      glhckTextRender(text);
      glhckTextClear(text);

      glhckTextColorb(text, 255, 0, 0, 255);
      glhckTextStash(text, font2, 12, WIDTH-100,        18, "Wall of text", NULL);
      glhckTextRender(text);
      glhckTextClear(text);

      /* actual swap and clear */
      glfwSwapBuffers(window);
      glhckRenderClear(GLHCK_COLOR_BUFFER | GLHCK_DEPTH_BUFFER);

      /* fps calc */
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

   /* check that everything is freed like should */
   for (li = 0; li != numLights; ++li)
      glhckLightFree(light[li]);
   glhckObjectFree(cube);
   glhckObjectFree(sprite);
   glhckObjectFree(cube2);
   glhckObjectFree(sprite3);
   glhckObjectFree(plane);
   if (rttText)  glhckObjectFree(rttText);
   if (rttText2) glhckObjectFree(rttText2);
   if (material) glhckMaterialFree(material);
   if (animator) glhckAnimatorFree(animator);
   glhckCameraFree(camera);
   glhckTextFree(text);

   /* should cleanup all
    * objects as well */
   glhckContextTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
