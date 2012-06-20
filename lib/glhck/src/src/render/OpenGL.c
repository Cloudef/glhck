#include "../internal.h"
#include "../../include/SOIL.h"
#include "render.h"
#include <limits.h>
#include <stdio.h>   /* for sscanf */

#if !defined(GLES)
#  include <GL/glew.h> /* for opengl */
#else
#  include <GLES/gl.h> /* for opengl ES */
#  include <GLES/glext.h>
#  define GL_FRAMEBUFFER_COMPLETE                        GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_UNDEFINED                       GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT           GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER          GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER          GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT   GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE          GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS        GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_UNSUPPORTED                     GL_FRAMEBUFFER_UNSUPPORTED_OES
#  define GL_FRAMEBUFFER                                 GL_FRAMEBUFFER_OES
#endif

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define RENDER_NAME "OpenGL Renderer"
#define GL_DEBUG 1

#if GLHCK_VERTEXDATA_COLOR
#  define GLHCK_ATTRIB_COUNT 2 /* 4 */
#else
#  define GLHCK_ATTRIB_COUNT 2 /* 3 */
#endif

const GLenum _glhckAttribName[] = {
   GL_VERTEX_ARRAY,
   GL_TEXTURE_COORD_ARRAY,
   GL_NORMAL_ARRAY,
#if GLHCK_VERTEXDATA_COLOR
   GL_COLOR_ARRAY,
#endif
};

/* global data */
typedef struct __OpenGLstate {
   char attrib[GLHCK_ATTRIB_COUNT];
   char cull, alpha, texture, draw_aabb, wireframe;
} __OpenGLstate;

typedef struct __OpenGLrender {
   struct __OpenGLstate state;
   size_t indicesCount;
   kmMat4 projection;
   kmMat4 orthographic;
} __OpenGLrender;
static __OpenGLrender _OpenGL;

/* declare gl generation function */
#define DECLARE_GL_GEN_FUNC(x,y)                                     \
static void x(size_t count, unsigned int *objects)                   \
{                                                                    \
   CALL(0, "%zu, %p", count, objects);                               \
   GL_CALL(y(count, objects));                                       \
}

/* declare gl bind function */
#define DECLARE_GL_BIND_FUNC(x,y)                                    \
static void x(unsigned int object)                                   \
{                                                                    \
   CALL(2, "%d", object);                                            \
   GL_CALL(y);                                                       \
}

/* check gl errors on debug build */
#ifdef NDEBUG
#  define GL_CALL(x) x
#else
#  define GL_CALL(x) x; GL_ERROR(__func__, __STRING(x));
static inline void GL_ERROR(const char *func, const char *glfunc)
{
   GLenum error;
   if ((error = glGetError())
         != GL_NO_ERROR)
      DEBUG(GLHCK_DBG_ERROR, "GL @%-20s %-20s >> %s",
            func, glfunc,
            error==GL_INVALID_ENUM?
            "GL_INVALID_ENUM":
            error==GL_INVALID_VALUE?
            "GL_INVALID_VALUE":
            error==GL_INVALID_OPERATION?
            "GL_INVALID_OPERATION":
            error==GL_STACK_OVERFLOW?
            "GL_STACK_OVERFLOW":
            error==GL_STACK_UNDERFLOW?
            "GL_STACK_UNDERFLOW":
            error==GL_OUT_OF_MEMORY?
            "GL_OUT_OF_MEMORY":
            error==GL_INVALID_OPERATION?
            "GL_INVALID_OPERATION":
            "GL_UNKNOWN_ERROR");
}
#endif

/* check return value of gl function on debug build */
#ifdef NDEBUG
#  define GL_CHECK(x) x
#else
#  define GL_CHECK(x) GL_CHECK_ERROR(__func__, __STRING(x), x)
static inline GLenum GL_CHECK_ERROR(const char *func, const char *glfunc,
      GLenum error)
{
   if (error != GL_NO_ERROR &&
       error != GL_FRAMEBUFFER_COMPLETE)
      DEBUG(GLHCK_DBG_ERROR, "GL @%-20s %-20s >> %s",
            func, glfunc,
            error==GL_FRAMEBUFFER_UNDEFINED?
            "GL_FRAMEBUFFER_UNDEFINED":
            error==GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT?
            "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT":
            error==GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER?
            "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER":
            error==GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER?
            "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER":
            error==GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT?
            "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT":
            error==GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE?
            "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE":
            error==GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS?
            "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS":
            error==GL_FRAMEBUFFER_UNSUPPORTED?
            "GL_FRAMEBUFFER_UNSUPPORTED":
            "GL_UNKNOWN_ERROR");
   return error;
}
#endif

/* ---- Render API ---- */

/* \brief upload texture to renderer */
static int uploadTexture(_glhckTexture *texture, unsigned int flags)
{
   CALL(0, "%p, %d", texture, flags);
   texture->object = SOIL_create_OGL_texture(
         texture->data,
         texture->width, texture->height,
      _glhckNumChannels(texture->format),
         texture->object?texture->object:0,
         flags);

   RET(0, "%d", texture->object?RETURN_OK:RETURN_FAIL);
   return texture->object?RETURN_OK:RETURN_FAIL;
}

static void getPixels(int x, int y, int width, int height,
      unsigned int format, unsigned char *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %p",
         x, y, width, height, format, data);
   GL_CALL(glReadPixels(x, y, width, height,
            format, GL_UNSIGNED_BYTE, data));
}

/* \brief create texture from data and upload it */
static unsigned int createTexture(const unsigned char *const buffer,
      int width, int height, unsigned int format,
      unsigned int reuse_texture_ID,
      unsigned int flags)
{
   unsigned int object;
   CALL(0, "%p, %d, %d, %d, %d, %d", buffer,
         width, height, format,
         reuse_texture_ID, flags);

   /* create empty texture */
   if (!(object = reuse_texture_ID)) {
      GL_CALL(glGenTextures(1, &object));
   }

   /* fail? */
   if (!object)
      goto _return;

   glhckBindTexture(object);
   GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
   GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
   GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
   GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
   GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0,
            format, width, height, 0,
            format, GL_UNSIGNED_BYTE, buffer));

_return:
   RET(0, "%d", object);
   return object;
}

/* \brief fill texture with data */
static void fillTexture(unsigned int texture,
      unsigned char *data,
      int x, int y, int width, int height,
      unsigned int format)
{
   CALL(1, "%d, %p, %d, %d, %d, %d, %d", texture,
         data, x, y, width, height, format);
   glhckBindTexture(texture);
   GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
            width, height, format, GL_UNSIGNED_BYTE, data));
}

/* \brief link FBO with texture */
static int linkFramebufferWithTexture(unsigned int object,
      unsigned int texture, unsigned int attachment)
{
   CALL(0, "%d, %d, %d", object, texture, attachment);

   GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, object));
   GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
         GL_TEXTURE_2D, texture, 0));

   if (GL_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER))
         != GL_FRAMEBUFFER_COMPLETE)
      goto fbo_fail;

   GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fbo_fail:
   DEBUG(GLHCK_DBG_ERROR, "Framebuffer is not complete");
fail:
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief set clear color */
static void setClearColor(const float r, const float g, const float b, const float a)
{
   TRACE(1);
   GL_CALL(glClearColor(r, g, b, a));
}


/* \brief clear scene */
static void clear(void)
{
   TRACE(2);

   /* clear buffers, TODO: keep track of em */
   GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

   /* reset stats */
   _OpenGL.indicesCount = 0;
}

/* \brief set projection matrix */
static void setProjection(const kmMat4 *m)
{
   CALL(2, "%p", m);
   GL_CALL(glMatrixMode(GL_PROJECTION));
#ifdef GLHCK_KAZMATH_FLOAT
   GL_CALL(glLoadMatrixf((float*)m));
#else
   GL_CALL(glLoadMatrixd((double*)m));
#endif
   memcpy(&_OpenGL.projection, m, sizeof(kmMat4));
}

/* \brief get current projection */
static kmMat4 getProjection(void)
{
   TRACE(2);
   RET(2, "%p", &_OpenGL.projection);
   return _OpenGL.projection;
}

/* \brief resize viewport */
static void viewport(int x, int y, int width, int height)
{
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
   assert(width > 0 && height > 0);

   /* set viewport */
   GL_CALL(glViewport(x, y, width, height));

   /* create orthographic projection matrix */
   kmMat4OrthographicProjection(&_OpenGL.orthographic,
         0, width, 0, height, -1, 1);
}

/* \brief pass interleaved vertex data to OpenGL nicely. */
static inline void geometryPointer(__GLHCKobjectGeometry *geometry)
{
   /* vertex data */
   GL_CALL(glVertexPointer(3, GLHCK_PRECISION_VERTEX,
            sizeof(__GLHCKvertexData), &geometry->vertexData[0].vertex));
   GL_CALL(glNormalPointer(GLHCK_PRECISION_VERTEX,
            sizeof(__GLHCKvertexData), &geometry->vertexData[0].normal));
   GL_CALL(glTexCoordPointer(2, GLHCK_PRECISION_COORD,
            sizeof(__GLHCKvertexData), &geometry->vertexData[0].coord));
#if GLHCK_VERTEXDATA_COLOR
   GL_CALL(glColorPointer(4, GLHCK_PRECISION_COLOR,
            sizeof(__GLHCKvertexData), &geometry->vertexData[0].color));
#endif
}

/* \brief draw interleaved geometry */
static inline void geometryDraw(__GLHCKobjectGeometry *geometry)
{
   if (geometry->indices) {
      GL_CALL(glDrawElements(geometry->type , geometry->indicesCount,
               GLHCK_PRECISION_INDEX, &geometry->indices[0]));
   } else {
      GL_CALL(glDrawArrays(geometry->type, 0, geometry->vertexCount));
   }
}

/* \brief set needed state from object data */
static inline void materialState(_glhckObject *object)
{
   unsigned int i;
   __OpenGLstate old = _OpenGL.state;

   /* always draw vertices */
   _OpenGL.state.attrib[0] = 1;

   /* need texture? */
   _OpenGL.state.texture   = object->material.texture?1:0;
   _OpenGL.state.attrib[1] = _OpenGL.state.texture;

   /* aabb? */
   _OpenGL.state.draw_aabb =
      object->material.flags & GLHCK_MATERIAL_DRAW_AABB;

   /* wire? */
   _OpenGL.state.wireframe =
      object->material.flags & GLHCK_MATERIAL_WIREFRAME;

   /* alpha? */
   if (object->material.flags & GLHCK_MATERIAL_ALPHA)
      _OpenGL.state.alpha = 1;

   /* check culling */
   if (_OpenGL.state.cull != old.cull) {
      if (_OpenGL.state.cull) {
         GL_CALL(glEnable(GL_DEPTH_TEST));
         GL_CALL(glEnable(GL_DEPTH_BUFFER_BIT));
         GL_CALL(glDepthMask(GL_TRUE));
         GL_CALL(glDepthFunc(GL_LEQUAL));
         GL_CALL(glCullFace(GL_BACK));
         GL_CALL(glEnable(GL_CULL_FACE));
      } else {
         GL_CALL(glDisable(GL_DEPTH_TEST));
         GL_CALL(glDisable(GL_CULL_FACE));
      }
   }

   /* disable culling for strip geometry
    * TODO: Fix the stripping to get rid of this */
   if (_OpenGL.state.cull &&
       object->geometry.type == GLHCK_TRIANGLE_STRIP) {
      GL_CALL(glDisable(GL_CULL_FACE));
   }

   /* check alpha */
   if (_OpenGL.state.alpha != old.alpha) {
      if (_OpenGL.state.alpha) {
         GL_CALL(glEnable(GL_BLEND));
         GL_CALL(glBlendFunc(GL_SRC_ALPHA,
                  GL_ONE_MINUS_SRC_ALPHA));
      } else {
         GL_CALL(glDisable(GL_BLEND));
      }
   }

   /* check texture */
   if (_OpenGL.state.texture != old.texture) {
      if (_OpenGL.state.texture) {
         GL_CALL(glEnable(GL_TEXTURE_2D));
      } else {
         GL_CALL(glDisable(GL_TEXTURE_2D));
      }
   }

   /* check attribs */
   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i) {
      if (_OpenGL.state.attrib[i] != old.attrib[i]) {
         if (_OpenGL.state.attrib[i]) {
            GL_CALL(glEnableClientState(_glhckAttribName[i]));
         } else {
            GL_CALL(glDisableClientState(_glhckAttribName[i]));
         }
      }
   }

   /* bind texture if used */
   if (_OpenGL.state.texture)
      glhckTextureBind(object->material.texture);
}

/* \brief draw object's bounding box */
static void drawAABB(_glhckObject *object)
{
   unsigned int i = 0;
   kmVec3 min = object->view.bounding.min;
   kmVec3 max = object->view.bounding.max;
   const short points[] = {
                      min.x, min.y, min.z,
                      max.x, min.y, min.z,
                      min.x, min.y, min.z,
                      min.x, max.y, min.z,
                      min.x, min.y, min.z,
                      min.x, min.y, max.z,

                      max.x, max.y, max.z,
                      min.x, max.y, max.z,
                      max.x, max.y, max.z,
                      max.x, min.y, max.z,
                      max.x, max.y, max.z,
                      max.x, max.y, min.z,

                      min.x, max.y, min.z,
                      max.x, max.y, min.z,
                      min.x, max.y, min.z,
                      min.x, max.y, max.z,

                      max.x, min.y, min.z,
                      max.x, max.y, min.z,
                      max.x, min.y, min.z,
                      max.x, min.y, max.z,

                      min.x, min.y, max.z,
                      max.x, min.y, max.z,
                      min.x, min.y, max.z,
                      min.x, max.y, max.z  };

   /* disable stuff if enabled */
   if (_OpenGL.state.texture) {
      GL_CALL(glDisable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glDisableClientState(_glhckAttribName[i]));
      }

   GL_CALL(glColor4f(0, 1, 0, 1));
   GL_CALL(glVertexPointer(3, GL_SHORT, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glColor4f(1, 1, 1, 1));

   /* re enable stuff we disabled */
   if (_OpenGL.state.texture) {
      GL_CALL(glEnable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glEnableClientState(_glhckAttribName[i]));
      }
}

/* \brief draw single object */
static void objectDraw(_glhckObject *object)
{
   unsigned int i;
   unsigned int old_primitive;
   CALL(2, "%p", object);

   /* no point drawing without vertex data */
   if (!object->geometry.vertexData ||
       !object->geometry.vertexCount)
      return;

   /* load view matrix */
   GL_CALL(glMatrixMode(GL_MODELVIEW));
#ifdef GLHCK_KAZMATH_FLOAT
   GL_CALL(glLoadMatrixf((float*)&object->view.matrix));
#else
   GL_CALL(glLoadMatrixd((double*)&object->view.matrix));
#endif

   /* set states and draw */
   materialState(object);
   geometryPointer(&object->geometry);

   /* switch to wireframe if requested */
   if (_OpenGL.state.wireframe) {
      old_primitive = object->geometry.type;
      object->geometry.type = object->geometry.type == GLHCK_TRIANGLES
         ? GLHCK_LINES : GLHCK_LINE_STRIP;
   }

   /* draw geometry */
   geometryDraw(&object->geometry);

   /* restore old primitive type */
   if (_OpenGL.state.wireframe)
      object->geometry.type = old_primitive;

   /* draw bounding box, if requested */
   if (_OpenGL.state.draw_aabb)
      drawAABB(object);

   /* enable the culling back
    * NOTE: this is a hack*/
   if (_OpenGL.state.cull &&
       object->geometry.type == GLHCK_TRIANGLE_STRIP) {
      GL_CALL(glEnable(GL_CULL_FACE));
   }

   _OpenGL.indicesCount += object->geometry.indicesCount;
}

/* \brief draw text */
static void textDraw(_glhckText *text)
{
   __GLHCKtextTexture *texture;
   CALL(2, "%p", text);

   /* set states */
   if (!_OpenGL.state.cull) {
      _OpenGL.state.cull = 1;
      GL_CALL(glEnable(GL_CULL_FACE));
   }

   if (!_OpenGL.state.alpha) {
      _OpenGL.state.alpha = 1;
      GL_CALL(glEnable(GL_BLEND));
      GL_CALL(glBlendFunc(GL_SRC_ALPHA,
               GL_ONE_MINUS_SRC_ALPHA));
   }

   if (!_OpenGL.state.texture) {
      _OpenGL.state.texture = 1;
      _OpenGL.state.attrib[1] = 1;
      GL_CALL(glEnable(GL_TEXTURE_2D));
      GL_CALL(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
   }

   if (!_OpenGL.state.attrib[0]) {
      _OpenGL.state.attrib[0] = 1;
      GL_CALL(glEnableClientState(GL_VERTEX_ARRAY));
   }

   /* set 2d projection */
   GL_CALL(glMatrixMode(GL_PROJECTION));
   GL_CALL(glLoadMatrixf((float*)&_OpenGL.orthographic));

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadIdentity());

   /* the culling is flipped in orthographic */
   glCullFace(GL_FRONT);

   GL_CALL(glDisable(GL_DEPTH_TEST));
   for (texture = text->tcache; texture;
        texture = texture->next) {
      if (!texture->geometry.vertexCount)
         continue;
      glhckBindTexture(texture->object);
      GL_CALL(glVertexPointer(2, GLHCK_PRECISION_VERTEX,
            sizeof(__GLHCKtextVertexData),
            &texture->geometry.vertexData[0].vertex));
      GL_CALL(glTexCoordPointer(2, GLHCK_PRECISION_COORD,
            sizeof(__GLHCKtextVertexData),
            &texture->geometry.vertexData[0].coord));
      GL_CALL(glDrawArrays(GL_TRIANGLES, 0,
            texture->geometry.vertexCount));
      texture->geometry.vertexCount = 0;
   }
   GL_CALL(glEnable(GL_DEPTH_TEST));

   /* switch back to normal cull */
   glCullFace(GL_BACK);

   setProjection(&_OpenGL.projection);
}

/* ---- Initialization ---- */

/* \brief get render information */
static int renderInfo(void)
{
   char *version, *vendor, *extensions;
   unsigned int major = 0, minor = 0, patch = 0;
   int num_parsed;
   TRACE(0);

   version = (char*)GL_CALL(glGetString(GL_VERSION));
   vendor  = (char*)GL_CALL(glGetString(GL_VENDOR));

   if (!version || !vendor)
      goto fail;

   for (; version &&
          !sscanf(version, "%d.%d.%d", &major, &minor, &patch);
          ++version);

   DEBUG(3, "%s [%u.%u.%u] [%s]\n", RENDER_NAME, major, minor, patch, vendor);
   extensions = (char*)GL_CALL(glGetString(GL_EXTENSIONS));

   if (extensions)
      DEBUG(3, extensions);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief init renderers global state */
static int renderInit(void)
{
   TRACE(0);

   /* init global data */
   memset(&_OpenGL, 0, sizeof(__OpenGLrender));
   memset(&_OpenGL.state, 0, sizeof(__OpenGLstate));

   /* save from some headache */
   GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT,1));

   /* set texture coords according to how glhck wants them */
   GL_CALL(glMatrixMode(GL_TEXTURE));
   GL_CALL(glScalef(1.0f/GLHCK_RANGE_COORD, 1.0f/GLHCK_RANGE_COORD, 1.0f));
   GL_CALL(glMatrixMode(GL_MODELVIEW));

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief terminate renderer */
static void renderTerminate(void)
{
   TRACE(0);

   /* this tells library that we are no longer alive. */
   GLHCK_RENDER_TERMINATE(RENDER_NAME);
}

/* OpenGL bindings */
DECLARE_GL_GEN_FUNC(_glGenTextures, glGenTextures)
DECLARE_GL_GEN_FUNC(_glDeleteTextures, glDeleteTextures)
DECLARE_GL_BIND_FUNC(_glBindTexture, glBindTexture(GL_TEXTURE_2D, object))

DECLARE_GL_GEN_FUNC(_glGenFramebuffers, glGenFramebuffers);
DECLARE_GL_GEN_FUNC(_glDeleteFramebuffers, glDeleteFramebuffers);
DECLARE_GL_BIND_FUNC(_glBindFramebuffer, glBindFramebuffer(GL_FRAMEBUFFER, object))

/* ---- Main ---- */

/* \brief renderer main function */
void _glhckRenderOpenGL(void)
{
   TRACE(0);

   /* init OpenGL context */
   GL_CALL(glClearColor(0,0,0,1));
   GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

#if !defined(GLES)
   /* we use GLEW */
   if (glewInit() != GLEW_OK)
      goto fail;
#endif

   /* output information */
   if (renderInfo() != RETURN_OK)
      goto fail;

   /* reset global data */
   if (renderInit() != RETURN_OK)
      goto fail;

   /* register api functions */

   /* textures */
   GLHCK_RENDER_FUNC(generateTextures, _glGenTextures);
   GLHCK_RENDER_FUNC(deleteTextures, _glDeleteTextures);
   GLHCK_RENDER_FUNC(bindTexture, _glBindTexture);
   GLHCK_RENDER_FUNC(uploadTexture, uploadTexture);
   GLHCK_RENDER_FUNC(createTexture, createTexture);
   GLHCK_RENDER_FUNC(fillTexture, fillTexture);

   /* framebuffer objects */
   GLHCK_RENDER_FUNC(generateFramebuffers, _glGenFramebuffers);
   GLHCK_RENDER_FUNC(deleteFramebuffers, _glDeleteFramebuffers);
   GLHCK_RENDER_FUNC(bindFramebuffer, _glBindFramebuffer);
   GLHCK_RENDER_FUNC(linkFramebufferWithTexture, linkFramebufferWithTexture);

   /* drawing functions */
   GLHCK_RENDER_FUNC(setProjection, setProjection);
   GLHCK_RENDER_FUNC(getProjection, getProjection);
   GLHCK_RENDER_FUNC(setClearColor, setClearColor);
   GLHCK_RENDER_FUNC(clear, clear);
   GLHCK_RENDER_FUNC(objectDraw, objectDraw);
   GLHCK_RENDER_FUNC(textDraw, textDraw);

   /* screen */
   GLHCK_RENDER_FUNC(getPixels, getPixels);

   /* common */
   GLHCK_RENDER_FUNC(viewport, viewport);
   GLHCK_RENDER_FUNC(terminate, renderTerminate);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

fail:
   return;
}

/* vim: set ts=8 sw=3 tw=0 :*/
