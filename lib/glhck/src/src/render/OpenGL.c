#include "../internal.h"
#include "render.h"
#include <stdio.h>   /* for sscanf */
#include <GL/glew.h> /* for opengl */

#define RENDER_NAME "OpenGL Renderer"

#define DECLARE_GL_GEN_FUNC(x,y)                                     \
static void x(unsigned int count, unsigned int *objects)             \
{                                                                    \
   CALL("%d, %p", count, objects);                                   \
   y(count, objects);                                                \
}

#define DECLARE_GL_BIND_FUNC(x,y)                                    \
static void x(unsigned int object)                                   \
{                                                                    \
   CALL("%d", object);                                               \
   y;                                                                \
}

static int renderInfo(void)
{
   char *version, *vendor, *extensions;
   unsigned int major, minor, patch;
   int num_parsed;
   TRACE();

   version = (char*)glGetString(GL_VERSION);
   vendor  = (char*)glGetString(GL_VENDOR);

   if (!version || !vendor)
      goto fail;

   num_parsed = sscanf(version, "%u.%u.%u",
         &major, &minor, &patch);

   if (num_parsed == 1) {
      minor = 0;
      patch = 0;
   } else if (num_parsed == 2)
      patch = 0;

   DEBUG(3, "%s [%u.%u.%u] [%s]\n", RENDER_NAME, major, minor, patch, vendor);
   if ((extensions = (char*)glGetString(GL_EXTENSIONS)))
      DEBUG(3, extensions);

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

static void terminate(void)
{
   TRACE();
}

/* OpenGL bindings */
DECLARE_GL_GEN_FUNC(_glGenTextures, glGenTextures)
DECLARE_GL_GEN_FUNC(_glDeleteTextures, glDeleteTextures)
DECLARE_GL_GEN_FUNC(_glGenBuffers, glGenBuffers)
DECLARE_GL_GEN_FUNC(_glDeleteBuffers, glDeleteBuffers)
DECLARE_GL_BIND_FUNC(_glBindTexture, glBindTexture(GL_TEXTURE_2D, object))

void _glhckRenderOpenGL(void)
{
   TRACE();

   /* init OpenGL context */
   glClear(GL_COLOR_BUFFER_BIT);

   /* we use GLEW */
   if (glewInit() != GLEW_OK)
      goto fail;

   /* output information */
   if (renderInfo() != RETURN_OK)
      goto fail;

   /* register api functions */
   GLHCK_RENDER_FUNC(generateTextures, _glGenTextures);
   GLHCK_RENDER_FUNC(deleteTextures, _glDeleteTextures);
   GLHCK_RENDER_FUNC(generateBuffers, _glGenBuffers);
   GLHCK_RENDER_FUNC(deleteBuffers, _glDeleteBuffers);
   GLHCK_RENDER_FUNC(bindTexture, _glBindTexture);

   /* terminate function */
   GLHCK_RENDER_FUNC(terminate, terminate);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

fail:
   return;
}
