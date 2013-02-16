#include "../internal.h"
#include "render.h"
#include <limits.h>
#include <stdio.h>   /* for sscanf */

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

#if GLHCK_USE_GLES2
#  error "OpenGL Fixed Pipeline renderer doesn't support GLES 2.x!"
#endif

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define RENDER_NAME "OpenGL(FixPipe) Renderer"
#define GL_DEBUG 1
#define GLHCK_ATTRIB_COUNT 4

/* include shared OpenGL functions */
#include "OpenGLHelper.h"

static const GLenum _glhckAttribName[] = {
   GL_VERTEX_ARRAY,
   GL_TEXTURE_COORD_ARRAY,
   GL_NORMAL_ARRAY,
   GL_COLOR_ARRAY,
};

/* state flags */
enum {
   GL_STATE_DEPTH       = 1,
   GL_STATE_CULL        = 2,
   GL_STATE_BLEND       = 4,
   GL_STATE_TEXTURE     = 8,
   GL_STATE_DRAW_AABB   = 16,
   GL_STATE_DRAW_OBB    = 32,
   GL_STATE_WIREFRAME   = 64
};

/* global data */
typedef struct __OpenGLstate {
   GLchar attrib[GLHCK_ATTRIB_COUNT];
   GLuint flags;
   GLuint blenda, blendb;
} __OpenGLstate;

typedef struct __OpenGLrender {
   struct __OpenGLstate state;
   GLsizei indexCount;
   kmMat4 projection;
   kmMat4 orthographic;
} __OpenGLrender;
static __OpenGLrender _OpenGL;

/* check if we have certain draw state active */
#define GL_HAS_STATE(x) (_OpenGL.state.flags & x)

/* ---- Render API ---- */

/* \brief set time */
static void rTime(float time)
{
}

/* \brief set projection matrix */
static void rSetProjection(const kmMat4 *m)
{
   CALL(2, "%p", m);
   GL_CALL(glMatrixMode(GL_PROJECTION));
#ifdef USE_DOUBLE_PRECISION
   GL_CALL(glLoadMatrixd((GLdouble*)m));
#else
   GL_CALL(glLoadMatrixf((GLfloat*)m));
#endif

   if (m != &_OpenGL.projection)
      memcpy(&_OpenGL.projection, m, sizeof(kmMat4));
}

/* \brief get current projection */
static const kmMat4* rGetProjection(void)
{
   TRACE(2);
   RET(2, "%p", &_OpenGL.projection);
   return &_OpenGL.projection;
}

/* \brief resize viewport */
static void rViewport(int x, int y, int width, int height)
{
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
   assert(width > 0 && height > 0);

   /* set viewport */
   GL_CALL(glViewport(x, y, width, height));

   /* create orthographic projection matrix */
   kmMat4OrthographicProjection(&_OpenGL.orthographic,
         0, width, height, 0, -1, 1);
}

/* helper for checking state changes */
#define GL_STATE_CHANGED(x) (_OpenGL.state.flags & x) != (old.flags & x)

/* \brief set needed state from object data */
static inline void materialState(const _glhckObject *object)
{
   GLuint i;
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
      (object->material.blenda != GLHCK_ZERO || object->material.blendb != GLHCK_ZERO)?GL_STATE_BLEND:0;

   /* depth? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_DEPTH)?GL_STATE_DEPTH:0;

   /* cull? */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_CULL)?GL_STATE_CULL:0;

   /* blending modes */
   _OpenGL.state.blenda = object->material.blenda;
   _OpenGL.state.blendb = object->material.blendb;

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
   if (GL_STATE_CHANGED(GL_STATE_BLEND)) {
      if (GL_HAS_STATE(GL_STATE_BLEND)) {
         GL_CALL(glEnable(GL_BLEND));
         GL_CALL(glhBlendFunc(_OpenGL.state.blenda, _OpenGL.state.blendb));
      } else {
         GL_CALL(glDisable(GL_BLEND));
      }
   } else if (GL_HAS_STATE(GL_STATE_BLEND) &&
         (_OpenGL.state.blenda != old.blenda ||
          _OpenGL.state.blendb != old.blendb)) {
      GL_CALL(glhBlendFunc(_OpenGL.state.blenda, _OpenGL.state.blendb));
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

/* helper macro for passing geometry */
#define geometryV2ToOpenGL(vprec, nprec, tprec, type, tunion)                 \
   GL_CALL(glVertexPointer(2, vprec,                                          \
            sizeof(type), &geometry->vertices.tunion[0].vertex));             \
   GL_CALL(glNormalPointer(nprec,                                             \
            sizeof(type), &geometry->vertices.tunion[0].normal));             \
   GL_CALL(glTexCoordPointer(2, tprec,                                        \
            sizeof(type), &geometry->vertices.tunion[0].coord));              \
   GL_CALL(glColorPointer(4, GL_UNSIGNED_BYTE,                                \
            sizeof(type), &geometry->vertices.tunion[0].color));

#define geometryV3ToOpenGL(vprec, nprec, tprec, type, tunion)                 \
   GL_CALL(glVertexPointer(3, vprec,                                          \
            sizeof(type), &geometry->vertices.tunion[0].vertex));             \
   GL_CALL(glNormalPointer(nprec,                                             \
            sizeof(type), &geometry->vertices.tunion[0].normal));             \
   GL_CALL(glTexCoordPointer(2, tprec,                                        \
            sizeof(type), &geometry->vertices.tunion[0].coord));              \
   GL_CALL(glColorPointer(4, GL_UNSIGNED_BYTE,                                \
            sizeof(type), &geometry->vertices.tunion[0].color));

/* \brief pass interleaved vertex data to OpenGL nicely. */
static inline void rGeometryPointer(const glhckGeometry *geometry)
{
   // printf("%s (%d) : %u\n", glhckVertexTypeString(geometry->vertexType), geometry->vertexCount, geometry->textureRange);

   /* vertex data */
   switch (geometry->vertexType) {
      case GLHCK_VERTEX_V3B:
         geometryV3ToOpenGL(GL_BYTE, GL_SHORT, GL_SHORT, glhckVertexData3b, v3b);
         break;

      case GLHCK_VERTEX_V2B:
         geometryV2ToOpenGL(GL_BYTE, GL_SHORT, GL_SHORT, glhckVertexData2b, v2b);
         break;

      case GLHCK_VERTEX_V3S:
         geometryV3ToOpenGL(GL_SHORT, GL_SHORT, GL_SHORT, glhckVertexData3s, v3s);
         break;

      case GLHCK_VERTEX_V2S:
         geometryV2ToOpenGL(GL_SHORT, GL_SHORT, GL_SHORT, glhckVertexData2s, v2s);
         break;

      case GLHCK_VERTEX_V3FS:
         geometryV3ToOpenGL(GL_FLOAT, GL_SHORT, GL_SHORT, glhckVertexData3fs, v3fs);
         break;

      case GLHCK_VERTEX_V2FS:
         geometryV2ToOpenGL(GL_FLOAT, GL_SHORT, GL_SHORT, glhckVertexData2fs, v2fs);
         break;

      case GLHCK_VERTEX_V3F:
         geometryV3ToOpenGL(GL_FLOAT, GL_FLOAT, GL_FLOAT, glhckVertexData3f, v3f);
         break;

      case GLHCK_VERTEX_V2F:
         geometryV2ToOpenGL(GL_FLOAT, GL_FLOAT, GL_FLOAT, glhckVertexData2f, v2f);
         break;

      default:
         break;
   }
}

/* the helpers are no longer needed */
#undef geometryV2ToOpenGL
#undef geometryV3ToOpenGL

/* \brief render frustum */
static inline void rFrustumRender(glhckFrustum *frustum)
{
   GLuint i = 0;
   kmVec3 *near = frustum->nearCorners;
   kmVec3 *far  = frustum->farCorners;
   const GLfloat points[] = {
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

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadIdentity());

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

/* \brief render object's oriented bounding box */
static inline void rOBBRender(const _glhckObject *object)
{
   GLuint i = 0;
   kmVec3 min = object->view.bounding.min;
   kmVec3 max = object->view.bounding.max;
   const GLfloat points[] = {
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

/* \brief render object's axis-aligned bounding box */
static inline void rAABBRender(const _glhckObject *object)
{
   GLuint i = 0;
   kmVec3 min = object->view.aabb.min;
   kmVec3 max = object->view.aabb.max;
   const GLfloat points[] = {
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
#ifdef USE_DOUBLE_PRECISION
   GL_CALL(glLoadMatrixd((GLdouble*)&object->view.matrix));
#else
   GL_CALL(glLoadMatrixf((GLfloat*)&object->view.matrix));
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

/* \brief begin object render */
static inline void rObjectStart(const _glhckObject *object) {
   /* set texture coords according to how geometry wants them */
   GL_CALL(glMatrixMode(GL_TEXTURE));
   GL_CALL(glLoadIdentity());
   GL_CALL(glScalef((GLfloat)1.0f/object->geometry->textureRange, (GLfloat)1.0f/object->geometry->textureRange, 1.0f));

   /* load view matrix */
   GL_CALL(glMatrixMode(GL_MODELVIEW));
#ifdef USE_DOUBLE_PRECISION
   GL_CALL(glLoadMatrixd((GLdouble*)&object->view.matrix));
#else
   GL_CALL(glLoadMatrixf((GLfloat*)&object->view.matrix));
#endif

   /* reset color */
   GL_CALL(glColor4ub(object->material.diffuse.r,
                      object->material.diffuse.g,
                      object->material.diffuse.b,
                      object->material.diffuse.a));

   /* set states and draw */
   materialState(object);
}

/* \brief end object render */
static inline void rObjectEnd(const _glhckObject *object) {
   glhckGeometryType type = object->geometry->type;

   /* switch to wireframe if requested */
   if (GL_HAS_STATE(GL_STATE_WIREFRAME)) {
      type = (object->geometry->type==GLHCK_TRIANGLES ? GLHCK_LINES:GLHCK_LINE_STRIP);
   }

   /* render geometry */
   glhGeometryRender(object->geometry, type);

   /* render axis-aligned bounding box, if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_AABB))
      rAABBRender(object);

   /* render oriented bounding box, if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_OBB))
      rOBBRender(object);

   /* enable the culling back
    * NOTE: this is a hack */
   if (GL_HAS_STATE(GL_STATE_CULL) &&
       object->geometry->type == GLHCK_TRIANGLE_STRIP) {
      GL_CALL(glEnable(GL_CULL_FACE));
   }

   _OpenGL.indexCount += object->geometry->indexCount;
}

/* \brief render single 3d object */
static inline void rObjectRender(const _glhckObject *object)
{
   CALL(2, "%p", object);

   /* no point drawing without vertex data */
   if (object->geometry->vertexType == GLHCK_VERTEX_NONE ||
      !object->geometry->vertexCount)
      return;

   rObjectStart(object);
   rGeometryPointer(object->geometry);
   rObjectEnd(object);
}

/* \brief draw text */
static inline void rTextRender(const _glhckText *text)
{
   glhckTexture *old;
   __GLHCKtextTexture *texture;
   CALL(2, "%p", text);

   /* set states */
   if (!GL_HAS_STATE(GL_STATE_CULL)) {
      _OpenGL.state.flags |= GL_STATE_CULL;
      GL_CALL(glEnable(GL_CULL_FACE));
   }

   if (!GL_HAS_STATE(GL_STATE_BLEND)) {
      _OpenGL.state.flags |= GL_STATE_BLEND;
      _OpenGL.state.blenda = GLHCK_SRC_ALPHA;
      _OpenGL.state.blendb = GLHCK_ONE_MINUS_SRC_ALPHA;
      GL_CALL(glEnable(GL_BLEND));
      GL_CALL(glhBlendFunc(_OpenGL.state.blenda, _OpenGL.state.blendb));
   } else if (_OpenGL.state.blenda != GLHCK_SRC_ALPHA ||
              _OpenGL.state.blendb != GLHCK_ONE_MINUS_SRC_ALPHA) {
      _OpenGL.state.blenda = GLHCK_SRC_ALPHA;
      _OpenGL.state.blendb = GLHCK_ONE_MINUS_SRC_ALPHA;
      GL_CALL(glhBlendFunc(_OpenGL.state.blenda, _OpenGL.state.blendb));
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
#ifdef USE_DOUBLE_PRECISION
   GL_CALL(glLoadMatrixd((GLdouble*)&_OpenGL.orthographic));
#else
   GL_CALL(glLoadMatrixf((GLfloat*)&_OpenGL.orthographic));
#endif

   GL_CALL(glMatrixMode(GL_TEXTURE));
   GL_CALL(glLoadIdentity());
   GL_CALL(glScalef((GLfloat)1.0f/text->textureRange, (GLfloat)1.0f/text->textureRange, 1.0f));

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadIdentity());

   GL_CALL(glColor4ub(text->color.r, text->color.b, text->color.g, text->color.a));

   /* store old texture */
   old = GLHCKRD()->texture[GLHCK_TEXTURE_2D];

   for (texture = text->tcache; texture;
        texture = texture->next) {
      if (!texture->geometry.vertexCount)
         continue;
      GL_CALL(glBindTexture(GL_TEXTURE_2D, texture->object));
      GL_CALL(glVertexPointer(2, (GLHCK_TEXT_FLOAT_PRECISION?GL_FLOAT:GL_SHORT),
            (GLHCK_TEXT_FLOAT_PRECISION?sizeof(glhckVertexData2f):sizeof(glhckVertexData2s)),
            &texture->geometry.vertexData[0].vertex));
      GL_CALL(glTexCoordPointer(2, (GLHCK_TEXT_FLOAT_PRECISION?GL_FLOAT:GL_SHORT),
            (GLHCK_TEXT_FLOAT_PRECISION?sizeof(glhckVertexData2f):sizeof(glhckVertexData2s)),
            &texture->geometry.vertexData[0].coord));
      GL_CALL(glDrawArrays(GLHCK_TRISTRIP?GL_TRIANGLE_STRIP:GL_TRIANGLES,
               0, texture->geometry.vertexCount));
   }

   /* restore old */
   if (old) glhckTextureBind(old);
   else glhckTextureUnbind(GLHCK_TEXTURE_2D);

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glEnable(GL_DEPTH_TEST));
   }

   /* restore back the original projection */
   rSetProjection(&_OpenGL.projection);
}

/* ---- Initialization ---- */

/* \brief get render information */
static int renderInfo(void)
{
   GLint maxTex;
   GLchar *version, *vendor, *extensions, *extcpy, *s;
   GLuint major = 0, minor = 0, patch = 0;
   TRACE(0);

   version = (GLchar*)GL_CALL(glGetString(GL_VERSION));
   vendor  = (GLchar*)GL_CALL(glGetString(GL_VENDOR));

   if (!version || !vendor)
      goto fail;

   for (; version &&
          !sscanf(version, "%d.%d.%d", &major, &minor, &patch);
          ++version);

   /* try to identify driver */
   if (_glhckStrupstr(vendor, "MESA"))
      GLHCKR()->driver = GLHCK_DRIVER_MESA;
   else if (strstr(vendor, "NVIDIA"))
      GLHCKR()->driver = GLHCK_DRIVER_NVIDIA;
   else if (strstr(vendor, "ATI"))
      GLHCKR()->driver = GLHCK_DRIVER_ATI;
   else
      GLHCKR()->driver = GLHCK_DRIVER_OTHER;

   DEBUG(3, "%s [%u.%u.%u] [%s]", RENDER_NAME, major, minor, patch, vendor);
   extensions = (char*)GL_CALL(glGetString(GL_EXTENSIONS));

   if (extensions) {
      extcpy = strdup(extensions);
      s = strtok(extcpy, " ");
      while (s) {
         DEBUG(3, "%s", s);
         s = strtok(NULL, " ");
      }
      free(extcpy);
   }

   glhGetIntegerv(GLHCK_MAX_TEXTURE_SIZE, &maxTex);
   DEBUG(3, "GL_MAX_TEXTURE_SIZE: %d", maxTex);
   glhGetIntegerv(GLHCK_MAX_RENDERBUFFER_SIZE, &maxTex);
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

/* stub shader functions */
static void rProgramUse(GLuint obj) {
   DEBUG(GLHCK_DBG_WARNING, "Shaders not supported on this renderer!");
}
static GLuint rProgramLink(GLuint vsobj, GLuint fsobj) {
   DEBUG(GLHCK_DBG_WARNING, "Shaders not supported on this renderer!");
   return 0;
}
static void rProgramDelete(GLuint obj) {
   DEBUG(GLHCK_DBG_WARNING, "Shaders not supported on this renderer!");
}
static _glhckShaderAttribute* rProgramAttributeList(GLuint obj) {
   return NULL;
}
static _glhckShaderUniform* rProgramUniformList(GLuint obj) {
   return NULL;
}
static GLuint rShaderCompile(glhckShaderType type, const GLchar *effectKey, const GLchar *memoryContents) {
   DEBUG(GLHCK_DBG_WARNING, "Shaders not supported on this renderer!");
   return 0;
}
static void rShaderDelete(GLuint obj) {
   DEBUG(GLHCK_DBG_WARNING, "Shaders not supported on this renderer!");
}

/* ---- Main ---- */

/* \brief renderer main function */
void _glhckRenderOpenGLFixedPipeline(void)
{
   TRACE(0);

   /* init OpenGL context */
   glhClearColor(0,0,0,1);
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
   GLHCK_RENDER_FUNC(textureGenerate, glGenTextures);
   GLHCK_RENDER_FUNC(textureDelete, glDeleteTextures);
   GLHCK_RENDER_FUNC(textureBind, glhTextureBind);
   GLHCK_RENDER_FUNC(textureCreate, glhTextureCreate);
   GLHCK_RENDER_FUNC(textureFill, glhTextureFill);

   /* framebuffer objects */
   GLHCK_RENDER_FUNC(framebufferGenerate, glGenFramebuffers);
   GLHCK_RENDER_FUNC(framebufferDelete, glDeleteFramebuffers);
   GLHCK_RENDER_FUNC(framebufferBind, glhFramebufferBind);
   GLHCK_RENDER_FUNC(framebufferTexture, glhFramebufferTexture);

   /* hardware buffer objects */
   GLHCK_RENDER_FUNC(hwBufferGenerate, glGenBuffers);
   GLHCK_RENDER_FUNC(hwBufferDelete, glDeleteBuffers);
   GLHCK_RENDER_FUNC(hwBufferBind, glhHwBufferBind);
   GLHCK_RENDER_FUNC(hwBufferBindBase, glhHwBufferBindBase);
   GLHCK_RENDER_FUNC(hwBufferBindRange, glhHwBufferBindRange);
   GLHCK_RENDER_FUNC(hwBufferCreate, glhHwBufferCreate);
   GLHCK_RENDER_FUNC(hwBufferFill, glhHwBufferFill);
   GLHCK_RENDER_FUNC(hwBufferMap, glhHwBufferMap);
   GLHCK_RENDER_FUNC(hwBufferUnmap, glhHwBufferUnmap);

   /* shader objects */
   GLHCK_RENDER_FUNC(programBind, rProgramUse);
   GLHCK_RENDER_FUNC(programLink, rProgramLink);
   GLHCK_RENDER_FUNC(programDelete, rProgramDelete);
   GLHCK_RENDER_FUNC(programAttributeList, rProgramAttributeList);
   GLHCK_RENDER_FUNC(programUniformList, rProgramUniformList);
   GLHCK_RENDER_FUNC(shaderCompile, rShaderCompile);
   GLHCK_RENDER_FUNC(shaderDelete, rShaderDelete);

   /* drawing functions */
   GLHCK_RENDER_FUNC(setProjection, rSetProjection);
   GLHCK_RENDER_FUNC(getProjection, rGetProjection);
   GLHCK_RENDER_FUNC(setClearColor, glhClearColor);
   GLHCK_RENDER_FUNC(clear, glhClear);
   GLHCK_RENDER_FUNC(objectRender, rObjectRender);
   GLHCK_RENDER_FUNC(textRender, rTextRender);
   GLHCK_RENDER_FUNC(frustumRender, rFrustumRender);

   /* screen */
   GLHCK_RENDER_FUNC(bufferGetPixels, glhBufferGetPixels);

   /* common */
   GLHCK_RENDER_FUNC(time, rTime);
   GLHCK_RENDER_FUNC(viewport, rViewport);
   GLHCK_RENDER_FUNC(terminate, renderTerminate);

   /* parameters */
   GLHCK_RENDER_FUNC(getIntegerv, glhGetIntegerv);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

fail:
   return;
}

/* vim: set ts=8 sw=3 tw=0 :*/
