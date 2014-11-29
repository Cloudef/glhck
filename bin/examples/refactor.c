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

static void error_callback(int error, const char *msg)
{
   (void)error;
   printf("-!- GLFW: %s\n", msg);
}

int main(int argc, char **argv)
{
   glfwSetErrorCallback(error_callback);
   if (!glfwInit())
      return EXIT_FAILURE;

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   GLFWwindow *window;

   for (size_t i = 0; i < glhckRendererCount(); ++i) {
      const char *name;
      const glhckRendererContextInfo *info = glhckRendererGetContextInfo(i, &name);

      glfwDefaultWindowHints();

      if (info->type == GLHCK_CONTEXT_OPENGL) {
         glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, info->major);
         glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, info->minor);
         glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, (info->forwardCompatible ? GL_TRUE : GL_FALSE));
         glfwWindowHint(GLFW_OPENGL_PROFILE, (info->coreProfile ? GLFW_OPENGL_CORE_PROFILE : GLFW_OPENGL_ANY_PROFILE));
         glfwWindowHint(GLFW_DEPTH_BITS, 16);
      }

      if (!(window = glfwCreateWindow(800, 480, "display test", NULL, NULL)))
         continue;

      glfwMakeContextCurrent(window);

      if (glhckDisplayCreate(800, 480, i))
         break;

      glfwDestroyWindow(window);
   }

   if (!glhckDisplayCreated())
      return EXIT_FAILURE;

   for (size_t i = 0; i < glhckImporterCount(); ++i)
      printf("%s\n", glhckImporterGetName(i));

   glhckHandle cube = glhckCubeNew(1.0f);
   glhckHandle cube2 = glhckCubeNew(1.0f);
   glhckObjectAddChild(cube, cube2);

   glhckHandle texture = glhckTextureNewFromFile("bin/media/madoka/md_m_eye.bmp", NULL, NULL);
   glhckObjectMaterial(cube, glhckMaterialNew(texture));

   glhckHandle text = glhckTextNew(256, 256);
   glhckFont kakwafont = glhckTextFontNewKakwafont(text, NULL);
   while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      glfwPollEvents();
      glhckRenderObject(cube);
      glhckTextClear(text);
      glhckTextStash(text, kakwafont, 12, 0, 12, "TESTI TEXT", NULL);
      glhckRenderText(text);
      glfwSwapBuffers(window);
      glhckRenderClear(GLHCK_COLOR_BUFFER_BIT | GLHCK_DEPTH_BUFFER_BIT);
   }

   glhckTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}
