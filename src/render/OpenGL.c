#include "../internal.h"
#include "render.h"
#include <limits.h>
#include <stdio.h>   /* for sscanf */

#if GLHCK_USE_GLES1
#  include <GLES/gl.h> /* for opengl ES 1.x */
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
#  warning "GLES 1.x not working yet"
#elif GLHCK_USE_GLES2
#  include <GLES2/gl2.h> /* for opengl ES 2.x */
#  include <GLES2/gl2ext.h>
#  error "GLES 2.x not implemented"
#else
#  include <GL/glew.h> /* for opengl */
#endif

#if defined(GLHCK_USE_GLES1)
#  ifdef GL_OES_element_index_uint
#     ifndef GL_UNSIGNED_INT
#        define GL_UNSIGNED_INT                   0x1405
#     endif
#  else
#     error "GLHCK needs GL_OES_element_index_uint for GLESv1 support!"
#  endif
#endif /* GLESv1 SUPPORT */

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define RENDER_NAME "OpenGL Renderer"
#define GL_DEBUG 1

#define GLHCK_ATTRIB_COUNT 4

const GLenum _glhckAttribName[] = {
   GL_VERTEX_ARRAY,
   GL_TEXTURE_COORD_ARRAY,
   GL_NORMAL_ARRAY,
   GL_COLOR_ARRAY,
};

/* state flags */
enum {
   GL_STATE_DEPTH       = 1,
   GL_STATE_CULL        = 2,
   GL_STATE_ALPHA       = 4,
   GL_STATE_TEXTURE     = 8,
   GL_STATE_DRAW_AABB   = 16,
   GL_STATE_DRAW_OBB    = 32,
   GL_STATE_WIREFRAME   = 64
};

/* global data */
typedef struct __OpenGLstate {
   char attrib[GLHCK_ATTRIB_COUNT];
   unsigned int flags;
} __OpenGLstate;

typedef struct __OpenGLrender {
   struct __OpenGLstate state;
   size_t indexCount;
   kmMat4 projection;
   kmMat4 orthographic;
} __OpenGLrender;
static __OpenGLrender _OpenGL;

/* check if we have certain draw state active */
#define GL_HAS_STATE(x) (_OpenGL.state.flags & x)

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
#  define GL_CALL(x) x; GL_ERROR(__LINE__, __func__, __STRING(x));
static inline void GL_ERROR(unsigned int line, const char *func, const char *glfunc)
{
   GLenum error;
   if ((error = glGetError()) != GL_NO_ERROR)
      DEBUG(GLHCK_DBG_ERROR, "GL @%d:%-20s %-20s >> %s",
            line, func, glfunc,
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

/* \brief get pixels from renderer */
static void getPixels(int x, int y, int width, int height,
      unsigned int format, unsigned char *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %p",
         x, y, width, height, format, data);
   GL_CALL(glReadPixels(x, y, width, height,
            format, GL_UNSIGNED_BYTE, data));
}

/* \brief create texture from data and upload it */
static unsigned int createTexture(const unsigned char *buffer, size_t size,
      int width, int height, unsigned int format,
      unsigned int reuse_texture_ID, unsigned int flags)
{
   unsigned int object;
   CALL(0, "%p, %zu, %d, %d, %d, %d", buffer, size,
         width, height, format, reuse_texture_ID);

   /* create empty texture */
   if (!(object = reuse_texture_ID)) {
      GL_CALL(glGenTextures(1, &object));
   }

   /* fail? */
   if (!object)
      goto _return;

   glhckBindTexture(object);
   if (flags & GLHCK_TEXTURE_NEAREST) {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
   } else {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
   }

   if (flags & GLHCK_TEXTURE_REPEAT) {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
   } else {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
   }

   if (_glhckIsCompressedFormat(format)) {
      GL_CALL(glCompressedTexImage2D(GL_TEXTURE_2D, 0,
               format, width, height, 0, size, buffer));
   } else {
      GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0,
               format, width, height, 0,
               format, GL_UNSIGNED_BYTE, buffer));
   }

_return:
   RET(0, "%d", object);
   return object;
}

/* \brief fill texture with data */
static void fillTexture(unsigned int texture,
      const unsigned char *data, size_t size,
      int x, int y, int width, int height,
      unsigned int format)
{
   CALL(1, "%d, %p, %zu, %d, %d, %d, %d, %d", texture,
         data, size, x, y, width, height, format );
   glhckBindTexture(texture);

   if (_glhckIsCompressedFormat(format)) {
      GL_CALL(glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
               width, height, format, size, data));
   } else{
      GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
               width, height, format, GL_UNSIGNED_BYTE, data));
   }
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
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief set clear color */
static void setClearColor(char r, char g, char b, char a)
{
   TRACE(1);
   float fr = (float)r/255, fg = (float)g/255, fb = (float)b/255;
   float fa = (float)a/255;

   GL_CALL(glClearColor(fr, fg, fb, fa));
   _GLHCKlibrary.render.draw.clearColor.r = r;
   _GLHCKlibrary.render.draw.clearColor.g = g;
   _GLHCKlibrary.render.draw.clearColor.b = b;
   _GLHCKlibrary.render.draw.clearColor.a = a;
}


/* \brief clear scene */
static void clear(void)
{
   TRACE(2);

   /* clear buffers, FIXME: keep track of em */
   GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

   /* reset stats */
   _OpenGL.indexCount = 0;
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

   if (m != &_OpenGL.projection)
      memcpy(&_OpenGL.projection, m, sizeof(kmMat4));
}

/* \brief get current projection */
static const kmMat4* getProjection(void)
{
   TRACE(2);
   RET(2, "%p", &_OpenGL.projection);
   return &_OpenGL.projection;
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
         0, width, height, 0, -1, 1);
}

/* helper macro for passing geometry */
#define geometryV2ToOpenGL(vprec, tprec, type, tunion)                        \
   GL_CALL(glVertexPointer(2, vprec,                                          \
            sizeof(type), &geometry->vertices.tunion[0].vertex));             \
   GL_CALL(glTexCoordPointer(2, tprec,                                        \
            sizeof(type), &geometry->vertices.tunion[0].coord));              \
   GL_CALL(glColorPointer(4, GL_UNSIGNED_BYTE,                                \
            sizeof(type), &geometry->vertices.tunion[0].color));

#define geometryV3ToOpenGL(vprec, tprec, type, tunion)                        \
   GL_CALL(glVertexPointer(3, vprec,                                          \
            sizeof(type), &geometry->vertices.tunion[0].vertex));             \
   GL_CALL(glNormalPointer(vprec,                                             \
            sizeof(type), &geometry->vertices.tunion[0].normal));             \
   GL_CALL(glTexCoordPointer(2, tprec,                                        \
            sizeof(type), &geometry->vertices.tunion[0].coord));              \
   GL_CALL(glColorPointer(4, GL_UNSIGNED_BYTE,                                \
            sizeof(type), &geometry->vertices.tunion[0].color));

/* \brief pass interleaved vertex data to OpenGL nicely. */
static inline void geometryPointer(const glhckGeometry *geometry)
{
   // printf("%s (%zu) : %u\n", glhckVertexTypeString(geometry->vertexType), geometry->vertexCount, geometry->textureRange);

   /* vertex data */
   switch (geometry->vertexType) {
      case GLHCK_VERTEX_V3B:
         geometryV3ToOpenGL(GL_BYTE, GL_BYTE, glhckVertexData3b, v3b);
         break;

      case GLHCK_VERTEX_V2B:
         geometryV2ToOpenGL(GL_BYTE, GL_BYTE, glhckVertexData2b, v2b);
         break;

      case GLHCK_VERTEX_V3S:
         geometryV3ToOpenGL(GL_SHORT, GL_SHORT, glhckVertexData3s, v3s);
         break;

      case GLHCK_VERTEX_V2S:
         geometryV2ToOpenGL(GL_SHORT, GL_SHORT, glhckVertexData2s, v2s);
         break;

      case GLHCK_VERTEX_V3F:
         geometryV3ToOpenGL(GL_FLOAT, GL_FLOAT, glhckVertexData3f, v3f);
         break;

      case GLHCK_VERTEX_V2F:
         geometryV2ToOpenGL(GL_FLOAT, GL_FLOAT, glhckVertexData2f, v2f);
         break;

      default:
         break;
   }
}

/* the helpers are no longer needed */
#undef geometryV2ToOpenGL
#undef geometryV3ToOpenGL

/* helper macro for passing indices to OpenGL */
#define indicesToOpenGL(iprec, tunion)                \
   GL_CALL(glDrawElements(type, geometry->indexCount, \
            iprec, &geometry->indices.tunion[0]))

/* \brief draw interleaved geometry */
static inline void geometryDraw(const glhckGeometry *geometry, unsigned int type)
{
   // printf("%s (%zu)\n", glhckIndexTypeString(geometry->indexType), geometry->indexCount);

   if (geometry->indexType != GLHCK_INDEX_NONE) {
      switch (geometry->indexType) {
         case GLHCK_INDEX_BYTE:
            indicesToOpenGL(GL_UNSIGNED_BYTE, ivb);
            break;

         case GLHCK_INDEX_SHORT:
            indicesToOpenGL(GL_UNSIGNED_SHORT, ivs);
            break;

         case GLHCK_INDEX_INTEGER:
            indicesToOpenGL(GL_UNSIGNED_INT, ivi);
            break;

         default:
            break;
      }
   } else {
      GL_CALL(glDrawArrays(type, 0, geometry->vertexCount));
   }
}

/* the helper is no longer needed */
#undef indicesToOpenGL

/* helper for checking state changes */
#define GL_STATE_CHANGED(x) (_OpenGL.state.flags & x) != (old.flags & x)

/* \brief set needed state from object data */
static inline void materialState(const _glhckObject *object)
{
   unsigned int i;
   __OpenGLstate old   = _OpenGL.state;
   _OpenGL.state.flags = 0; /* reset this state */

   /* always draw vertices */
   _OpenGL.state.attrib[0] = 1;

   /* enable normals always for now */
   _OpenGL.state.attrib[2] = glhckVertexTypeHasNormal(object->geometry->vertexType);

   /* need color? */
   _OpenGL.state.attrib[3] = (glhckVertexTypeHasColor(object->geometry->vertexType) &&
         object->material.flags & GLHCK_MATERIAL_COLOR);

   /* need texture? */
   _OpenGL.state.attrib[1] = object->material.texture?1:0;
   _OpenGL.state.flags |= object->material.texture?GL_STATE_TEXTURE:0;

   /* aabb? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_DRAW_AABB)?GL_STATE_DRAW_AABB:0;

   /* obb? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_DRAW_OBB)?GL_STATE_DRAW_OBB:0;

   /* wire? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_WIREFRAME)?GL_STATE_WIREFRAME:0;

   /* alpha? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_ALPHA)?GL_STATE_ALPHA:0;

   /* depth? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_DEPTH)?GL_STATE_DEPTH:0;

   /* cull? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_CULL)?GL_STATE_CULL:0;

   /* depth? */
   if (GL_STATE_CHANGED(GL_STATE_DEPTH)) {
      if (GL_HAS_STATE(GL_STATE_DEPTH)) {
         GL_CALL(glEnable(GL_DEPTH_TEST));
         GL_CALL(glDepthMask(GL_TRUE));
         GL_CALL(glDepthFunc(GL_LEQUAL));
      } else {
         GL_CALL(glDisable(GL_DEPTH_TEST));
      }
   }

   /* check culling */
   if (GL_STATE_CHANGED(GL_STATE_CULL)) {
      if (GL_HAS_STATE(GL_STATE_CULL)) {
         GL_CALL(glEnable(GL_CULL_FACE));
         GL_CALL(glCullFace(GL_BACK));
      } else {
         GL_CALL(glDisable(GL_CULL_FACE));
      }
   }

   /* disable culling for strip geometry
    * FIXME: Fix the stripping to get rid of this */
   if (GL_HAS_STATE(GL_STATE_CULL) &&
       object->geometry->type == GLHCK_TRIANGLE_STRIP) {
      GL_CALL(glDisable(GL_CULL_FACE));
   }

   /* check alpha */
   if (GL_STATE_CHANGED(GL_STATE_ALPHA)) {
      if (GL_HAS_STATE(GL_STATE_ALPHA)) {
         GL_CALL(glEnable(GL_BLEND));
         GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
      } else {
         GL_CALL(glDisable(GL_BLEND));
      }
   }

   /* check texture */
   if (GL_STATE_CHANGED(GL_STATE_TEXTURE)) {
      if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
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
   if (GL_HAS_STATE(GL_STATE_TEXTURE))
      glhckTextureBind(object->material.texture);
}

#undef GL_STATE_CHANGED

/* \brief draw object's oriented bounding box */
static inline void drawOBB(const _glhckObject *object)
{
   unsigned int i = 0;
   kmVec3 min = object->view.bounding.min;
   kmVec3 max = object->view.bounding.max;
   const float points[] = {
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
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glDisable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glDisableClientState(_glhckAttribName[i]));
      }

   GL_CALL(glColor3ub(0, 255, 0));
   GL_CALL(glVertexPointer(3, GL_FLOAT, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glColor3ub(255, 255, 255));

   /* re enable stuff we disabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glEnable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glEnableClientState(_glhckAttribName[i]));
      }
}

/* \brief draw object's axis-aligned bounding box */
static inline void drawAABB(const _glhckObject *object)
{
   unsigned int i = 0;
   kmVec3 min = object->view.aabb.min;
   kmVec3 max = object->view.aabb.max;
   const float points[] = {
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
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glDisable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glDisableClientState(_glhckAttribName[i]));
      }

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadIdentity());
   GL_CALL(glColor3ub(0, 0, 255));
   GL_CALL(glVertexPointer(3, GL_FLOAT, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glColor3ub(255, 255, 255));

   /* go back */
#ifdef GLHCK_KAZMATH_FLOAT
   GL_CALL(glLoadMatrixf((float*)&object->view.matrix));
#else
   GL_CALL(glLoadMatrixd((double*)&object->view.matrix));
#endif

   /* re enable stuff we disabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glEnable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glEnableClientState(_glhckAttribName[i]));
      }
}

/* \brief begin object draw */
static inline void objectStart(const _glhckObject *object) {
   /* set texture coords according to how geometry wants them */
   GL_CALL(glMatrixMode(GL_TEXTURE));
   GL_CALL(glLoadIdentity());
   GL_CALL(glScalef((float)1.0f/object->geometry->textureRange, (float)1.0f/object->geometry->textureRange, 1.0f));

   /* load view matrix */
   GL_CALL(glMatrixMode(GL_MODELVIEW));
#ifdef GLHCK_KAZMATH_FLOAT
   GL_CALL(glLoadMatrixf((float*)&object->view.matrix));
#else
   GL_CALL(glLoadMatrixd((double*)&object->view.matrix));
#endif

   /* reset color */
   GL_CALL(glColor4ub(object->material.color.r,
                      object->material.color.g,
                      object->material.color.b,
                      object->material.color.a));

   /* set states and draw */
   materialState(object);
}

/* \brief end object draw */
static inline void objectEnd(const _glhckObject *object) {
   unsigned int type = object->geometry->type;

   /* switch to wireframe if requested */
   if (GL_HAS_STATE(GL_STATE_WIREFRAME)) {
      type = object->geometry->type == GLHCK_TRIANGLES
         ? GLHCK_LINES : GLHCK_LINE_STRIP;
   }

   /* draw geometry */
   geometryDraw(object->geometry, type);

   /* draw axis-aligned bounding box, if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_AABB))
      drawAABB(object);

   /* draw oriented bounding box, if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_OBB))
      drawOBB(object);

   /* enable the culling back
    * NOTE: this is a hack */
   if (GL_HAS_STATE(GL_STATE_CULL) &&
       object->geometry->type == GLHCK_TRIANGLE_STRIP) {
      GL_CALL(glEnable(GL_CULL_FACE));
   }

   _OpenGL.indexCount += object->geometry->indexCount;
}

/* \brief draw single 3d object */
static inline void objectDraw(const _glhckObject *object)
{
   CALL(2, "%p", object);

   /* no point drawing without vertex data */
   if (object->geometry->vertexType == GLHCK_VERTEX_NONE ||
      !object->geometry->vertexCount)
      return;

   objectStart(object);
   geometryPointer(object->geometry);
   objectEnd(object);
}

/* \brief draw text */
static inline void textDraw(const _glhckText *text)
{
   __GLHCKtextTexture *texture;
   CALL(2, "%p", text);

   /* set states */
   if (!GL_HAS_STATE(GL_STATE_CULL)) {
      _OpenGL.state.flags |= GL_STATE_CULL;
      GL_CALL(glEnable(GL_CULL_FACE));
   }

   if (!GL_HAS_STATE(GL_STATE_ALPHA)) {
      _OpenGL.state.flags |= GL_STATE_ALPHA;
      GL_CALL(glEnable(GL_BLEND));
      GL_CALL(glBlendFunc(GL_SRC_ALPHA,
               GL_ONE_MINUS_SRC_ALPHA));
   }

   if (!GL_HAS_STATE(GL_STATE_TEXTURE)) {
      _OpenGL.state.flags |= GL_STATE_TEXTURE;
      _OpenGL.state.attrib[1] = 1;
      GL_CALL(glEnable(GL_TEXTURE_2D));
      GL_CALL(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
   }

   if (!_OpenGL.state.attrib[0]) {
      _OpenGL.state.attrib[0] = 1;
      GL_CALL(glEnableClientState(GL_VERTEX_ARRAY));
   }

   if (_OpenGL.state.attrib[3]) {
      _OpenGL.state.attrib[3] = 0;
      GL_CALL(glDisableClientState(GL_COLOR_ARRAY));
   }

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glDisable(GL_DEPTH_TEST));
   }

   /* set 2d projection */
   GL_CALL(glMatrixMode(GL_PROJECTION));
#ifdef GLHCK_KAZMATH_FLOAT
   GL_CALL(glLoadMatrixf((float*)&_OpenGL.orthographic));
#else
   GL_CALL(glLoadMatrixd((double*)&_OpenGL.orthographic));
#endif

   GL_CALL(glMatrixMode(GL_TEXTURE));
   GL_CALL(glLoadIdentity());
   GL_CALL(glScalef((float)1.0f/text->textureRange, (float)1.0f/text->textureRange, 1.0f));

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadIdentity());

   GL_CALL(glColor4ub(text->color.r, text->color.b, text->color.g, text->color.a));
   for (texture = text->tcache; texture;
        texture = texture->next) {
      if (!texture->geometry.vertexCount)
         continue;
      glhckBindTexture(texture->object);
      GL_CALL(glVertexPointer(2, (GLHCK_TEXT_FLOAT_PRECISION?GL_FLOAT:GL_SHORT),
            (GLHCK_TEXT_FLOAT_PRECISION?sizeof(glhckVertexData2f):sizeof(glhckVertexData2s)),
            &texture->geometry.vertexData[0].vertex));
      GL_CALL(glTexCoordPointer(2, (GLHCK_TEXT_FLOAT_PRECISION?GL_FLOAT:GL_SHORT),
            (GLHCK_TEXT_FLOAT_PRECISION?sizeof(glhckVertexData2f):sizeof(glhckVertexData2s)),
            &texture->geometry.vertexData[0].coord));
      GL_CALL(glDrawArrays(GLHCK_TRISTRIP?GL_TRIANGLE_STRIP:GL_TRIANGLES,
               0, texture->geometry.vertexCount));
   }

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glEnable(GL_DEPTH_TEST));
   }

   setProjection(&_OpenGL.projection);
}

/* \brief draw frustum */
static inline void frustumDraw(glhckFrustum *frustum, const kmMat4 *model)
{
   kmMat4 inv;
   unsigned int i = 0;
   kmVec3 *near = frustum->nearCorners;
   kmVec3 *far  = frustum->farCorners;
   kmMat4Inverse(&inv, model);
   const float points[] = {
                      near[0].x, near[0].y, near[0].z,
                      near[1].x, near[1].y, near[1].z,
                      near[1].x, near[1].y, near[1].z,
                      near[2].x, near[2].y, near[2].z,
                      near[2].x, near[2].y, near[2].z,
                      near[3].x, near[3].y, near[3].z,
                      near[3].x, near[3].y, near[3].z,
                      near[0].x, near[0].y, near[0].z,

                      far[0].x, far[0].y, far[0].z,
                      far[1].x, far[1].y, far[1].z,
                      far[1].x, far[1].y, far[1].z,
                      far[2].x, far[2].y, far[2].z,
                      far[2].x, far[2].y, far[2].z,
                      far[3].x, far[3].y, far[3].z,
                      far[3].x, far[3].y, far[3].z,
                      far[0].x, far[0].y, far[0].z,

                      near[0].x, near[0].y, near[0].z,
                       far[0].x,  far[0].y,  far[0].z,
                      near[1].x, near[1].y, near[1].z,
                       far[1].x,  far[1].y,  far[1].z,
                      near[2].x, near[2].y, near[2].z,
                       far[2].x,  far[2].y,  far[2].z,
                      near[3].x, near[3].y, near[3].z,
                       far[3].x,  far[3].y,  far[3].z  };

   /* disable stuff if enabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glDisable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glDisableClientState(_glhckAttribName[i]));
      }

   setProjection(&_OpenGL.projection);
   GL_CALL(glMatrixMode(GL_MODELVIEW));
#ifdef GLHCK_KAZMATH_FLOAT
   GL_CALL(glLoadMatrixf((float*)model));
   GL_CALL(glMultMatrixf((float*)&inv));
#else
   GL_CALL(glLoadMatrixd((double*)model));
   GL_CALL(glMultMatrixf((double*)&inv));
#endif

   GL_CALL(glLineWidth(4));
   GL_CALL(glColor3ub(255, 0, 0));
   GL_CALL(glVertexPointer(3, GL_FLOAT, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glColor3ub(255, 255, 255));
   GL_CALL(glLineWidth(1));

   /* re enable stuff we disabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glEnable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (_OpenGL.state.attrib[i]) {
         GL_CALL(glEnableClientState(_glhckAttribName[i]));
      }
}

/* \brief get parameters */
static void getIntegerv(unsigned int pname, int *params)
{
   CALL(1, "%u, %p", pname, params);
   GL_CALL(glGetIntegerv(pname, params));
}

/* ---- Initialization ---- */

/* \brief get render information */
static int renderInfo(void)
{
   int maxTex;
   char *version, *vendor, *extensions;
   unsigned int major = 0, minor = 0, patch = 0;
   TRACE(0);

   version = (char*)GL_CALL(glGetString(GL_VERSION));
   vendor  = (char*)GL_CALL(glGetString(GL_VENDOR));

   if (!version || !vendor)
      goto fail;

   for (; version &&
          !sscanf(version, "%d.%d.%d", &major, &minor, &patch);
          ++version);

   /* try to identify driver */
   if (_glhckStrupstr(vendor, "MESA"))
      _GLHCKlibrary.render.driver = GLHCK_DRIVER_MESA;
   else if (strstr(vendor, "NVIDIA"))
      _GLHCKlibrary.render.driver = GLHCK_DRIVER_NVIDIA;
   else if (strstr(vendor, "ATI"))
      _GLHCKlibrary.render.driver = GLHCK_DRIVER_ATI;
   else
      _GLHCKlibrary.render.driver = GLHCK_DRIVER_OTHER;

   DEBUG(3, "%s [%u.%u.%u] [%s]\n", RENDER_NAME, major, minor, patch, vendor);
   extensions = (char*)GL_CALL(glGetString(GL_EXTENSIONS));

   if (extensions)
      DEBUG(3, extensions);

   getIntegerv(GLHCK_MAX_TEXTURE_SIZE, &maxTex);
   DEBUG(3, "GL_MAX_TEXTURE_SIZE: %d", maxTex);
   getIntegerv(GLHCK_MAX_RENDERBUFFER_SIZE, &maxTex);
   DEBUG(3, "GL_MAX_RENDERBUFFER_SIZE: %d", maxTex);

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
   setClearColor(0,0,0,1);
   GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

   /* NOTE: Currently we don't use GLEW for anything.
    * GLEW used to be in libraries as submodule.
    * But since GLEW is so simple, we could just compile it with,
    * the OpenGL renderer within GLHCK in future when needed. */
#if 0
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
   GLHCK_RENDER_FUNC(frustumDraw, frustumDraw);

   /* screen */
   GLHCK_RENDER_FUNC(getPixels, getPixels);

   /* common */
   GLHCK_RENDER_FUNC(viewport, viewport);
   GLHCK_RENDER_FUNC(terminate, renderTerminate);

   /* parameters */
   GLHCK_RENDER_FUNC(getIntegerv, getIntegerv);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

fail:
   return;
}

/* vim: set ts=8 sw=3 tw=0 :*/
