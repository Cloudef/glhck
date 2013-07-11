#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if GLHCK_USE_GLES1
#  define GLFW_INCLUDE_ES1
#elif GLHCK_USE_GLES2
#  define GLFW_INCLUDE_ES2
#endif
#include "GLFW/glfw3.h"
#include "glhck/glhck.h"

static int RUNNING = 0;
static int WIDTH = 800, HEIGHT = 480;

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

int main(int argc, char **argv)
{
   GLFWwindow* window;
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

   glfwSetWindowCloseCallback(window, close_callback);
   glfwSetWindowSizeCallback(window, resize_callback);
   glfwMakeContextCurrent(window);
   puts("-!- window create");

   /* Turn on VSYNC if driver allows */
   glfwSwapInterval(0);

   if (!glhckContextCreate(argc, argv))
      return EXIT_FAILURE;

   puts("-!- glhck init");

   if (!glhckDisplayCreate(WIDTH, HEIGHT, GLHCK_RENDER_OPENGL))
      return EXIT_FAILURE;

   puts("-!- glhck display create");

   glhckText *text = glhckTextNew(4096, 4096);
   if (!text) return EXIT_FAILURE;
   unsigned int font = glhckTextFontNewKakwafont(text, NULL);
   unsigned int font2 = glhckTextFontNew(text, "example/media/DejaVuSans.ttf");

   const char *shaderContent =
      "-- GLhck.DistanceFieldGenerate.Fragment\n"
      "const float Spread = 12.0;"
      "void main() {"
      "  const float Step = 0.01;"
      "  float Distance = 1.0;"
      "  vec4 Diffuse = texture2D(GlhckTexture0, GlhckFUV0);"
      "  if (Diffuse.a == 0.0) {"
      "     Distance = 1.0;"
      "     for (float y = 0.0; y < 1.0; y+=Step) {"
      "        for (float x = 0.0; x < 1.0; x+=Step) {"
      "           vec2 Pos = vec2(x, y);"
      "           vec4 Point = texture2D(GlhckTexture0, Pos);"
      "           if (Point.a == 0.0) continue;"
      "           float d = distance(GlhckFUV0, Pos)*Spread;"
      "           if (d < Distance) Distance = d;"
      "        }"
      "     }"
      "     Distance = 1.0-Distance;"
      "  }"
      "  if (Distance > 1.0)"
      "     GlhckFragColor = vec4(1.0, 0.0, 0.0, 1.0);"
      "  else"
      "     GlhckFragColor = vec4(Distance);"
      "}\n"
      ""
      "-- GLhck.DistanceFieldRender.Fragment\n"
      "void main() {"
      "  vec4 Diffuse = GlhckMaterial.Diffuse;"
      "  float Distance = texture2D(GlhckTexture0, GlhckFUV0).a;"
      "  if (Distance < 0.5) Diffuse.a = 0.0;"
      "  else Diffuse.a = 1.0;"
      "  Diffuse.a *= smoothstep(0.45, 0.55, Distance);"
      "  GlhckFragColor = Diffuse;"
      "}";

   glhckShader *distanceFieldGenerateShader =
      glhckShaderNew(".GLhck.Base.Vertex", ".GLhck.DistanceFieldGenerate.Fragment", shaderContent);

   glhckShader *distanceFieldRenderShader =
      glhckShaderNew(".GLhck.Base.Vertex", ".GLhck.DistanceFieldRender.Fragment", shaderContent);

   /* draw some text */
   int width, height;
   glhckObject *object = glhckTextPlane(text, font2, 256, "1.", NULL);
   glhckMaterial *mat = glhckObjectGetMaterial(object);
   glhckTexture *texture = glhckMaterialGetTexture(mat);
   glhckTextureGetInformation(texture, NULL, &width, &height, NULL, NULL, NULL, NULL);
   glhckMaterialShader(mat, distanceFieldGenerateShader);

   int tsize = 32;
   int twidth = (width>tsize?tsize:width), theight = (height>tsize?tsize:height);
   if (width > height) theight *= (float)height/width;
   if (height > width) twidth  *= (float)width/height;

   // twidth = width, theight = height;
   glhckTexture *distanceTexture = glhckTextureNew();
   glhckTextureCreate(distanceTexture, GLHCK_TEXTURE_2D, 0, twidth, theight, 0, 0, GLHCK_RGBA, GLHCK_DATA_UNSIGNED_BYTE, 0, NULL);
   glhckTextureParameter(distanceTexture, glhckTextureDefaultLinearParameters());

   glhckFramebuffer *fbo = glhckFramebufferNew(GLHCK_FRAMEBUFFER);
   glhckFramebufferAttachTexture(fbo, distanceTexture, GLHCK_COLOR_ATTACHMENT0);
   glhckFramebufferRecti(fbo, 0, 0, twidth, theight);
   glhckFramebufferBegin(fbo);
   glhckRenderClearColorb(0,0,0,0);
   glhckRenderPass(GLHCK_PASS_TEXTURE);
   glhckRenderClear(GLHCK_COLOR_BUFFER);
   glhckObjectScalef(object, (float)twidth/width, (float)theight/height, 1.0f);
   glhckObjectPositionf(object, twidth/2, theight/2, 0.0f);
   glhckObjectRender(object);
   glhckFramebufferEnd(fbo);

   glhckObjectScalef(object, 1.0f, 1.0f, 1.0f);
   glhckMaterialShader(mat, NULL);
   glhckMaterialTexture(mat, distanceTexture);
   // glhckMaterialBlendFunc(mat, GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);

   glhckObject *object2 = glhckSpriteNew(distanceTexture, width, height);
   glhckMaterial *mat2 = glhckObjectGetMaterial(object2);
   glhckMaterialShader(mat2, distanceFieldRenderShader);
   glhckMaterialBlendFunc(mat2, GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);

   glhckObjectDrawAABB(object, 1);
   glhckObjectDrawAABB(object2, 1);

   RUNNING = 1;
   float sinv = 0.0f;
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      last  =  now;
      now   =  glfwGetTime();
      delta =  now - last;
      glhckRenderTime(now);

      glfwPollEvents();

      sinv += 0.5f * delta;
      float scale = sin(sinv)*1.4f+1.5f;
      glhckObjectRotationf(object2, 0, 0, scale*127.0f);
      glhckObjectScalef(object2, scale, scale, scale);
      glhckObjectPositionf(object, WIDTH/2-width/2, HEIGHT/2, 0.0f);
      glhckObjectPositionf(object2, WIDTH/2+width/2-scale*30.0f, HEIGHT/2, 0.0f);
      glhckObjectRender(object);
      glhckObjectRender(object2);


      glhckTextStash(text, font, 12, 0, HEIGHT-4, WIN_TITLE, NULL);
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

   glhckTextFree(text);

   /* should cleanup all
    * objects as well */
   glhckContextTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}
