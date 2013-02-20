#include "../internal.h"
#include "render.h"
#include <string.h>  /* for strdup */
#include <stdio.h>   /* for sscanf */

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

#if GLHCK_USE_GLES1
#  error "OpenGL renderer doesn't support GLES 1.x!"
#endif

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define RENDER_NAME "OpenGL Renderer"
#define GL_DEBUG 1
#define GLHCK_ATTRIB_COUNT GLHCK_ATTRIB_LAST

/* include shared OpenGL functions */
#include "OpenGLHelper.h"

/* GLSW */
#include "glsw.h"

/* This is the base shader for glhck */
static const char *_glhckBaseShader =
"-- GLhck.Base.Vertex\n"
"const mat4 ScaleMatrix ="
"mat4(0.5, 0.0, 0.0, 0.0,"
"     0.0, 0.5, 0.0, 0.0,"
"     0.0, 0.0, 0.5, 0.0,"
"     0.5, 0.5, 0.5, 1.0);"
""
"void main() {"
"  GlhckFVertexWorld = GlhckModel * vec4(GlhckVertex, 1.0);"
"  GlhckFVertexView = GlhckView * GlhckFVertexWorld;"
"  GlhckFNormalWorld = normalize(mat3(GlhckModel) * GlhckNormal);"
"  GlhckFUV0 = GlhckMaterial.TextureOffset + (GlhckUV0 * GlhckMaterial.TextureScale);"
"  GlhckFSC0 = ScaleMatrix * GlhckLight.Projection * GlhckLight.View * GlhckFVertexWorld;"
"  gl_Position = GlhckProjection * GlhckFVertexView;"
"}\n"

"-- GLhck.Text.Vertex\n"
"void main() {"
"  GlhckFUV0 = GlhckMaterial.TextureOffset + (GlhckUV0 * GlhckMaterial.TextureScale);"
"  gl_Position = GlhckOrthographic * vec4(GlhckVertex, 1.0);"
"}\n"

"-- GLhck.Base.Fragment\n"
"void main() {"
"  GlhckFragColor = texture2D(GlhckTexture0, GlhckFUV0) * GlhckMaterial.Diffuse/255.0;"
"}\n"

"-- GLhck.Color.Fragment\n"
"void main() {"
"  GlhckFragColor = GlhckMaterial.Diffuse/255.0;"
"}\n"

"-- GLhck.Base.Lighting.Fragment\n"
"void main() {"
"  vec4 Diffuse = texture2D(GlhckTexture0, GlhckFUV0) * GlhckMaterial.Diffuse/255.0;"
"  GlhckFragColor = clamp(vec4(glhckLighting(Diffuse.rgb), Diffuse.a), 0.0, 1.0);"
"}\n"

"-- GLhck.Color.Lighting.Fragment\n"
"void main() {"
"  vec4 Diffuse = GlhckMaterial.Diffuse/255.0;"
"  GlhckFragColor = clamp(vec4(glhckLighting(Diffuse.rgb), Diffuse.a), 0.0, 1.0);"
"}\n"

"-- GLhck.Text.Fragment\n"
"void main() {"
"  GlhckFragColor = GlhckMaterial.Diffuse/255.0 * texture2D(GlhckTexture0, GlhckFUV0).a;"
"}\n";

/* state flags */
enum {
   GL_STATE_DEPTH       = 1,
   GL_STATE_CULL        = 2,
   GL_STATE_BLEND       = 4,
   GL_STATE_TEXTURE     = 8,
   GL_STATE_DRAW_AABB   = 16,
   GL_STATE_DRAW_OBB    = 32,
   GL_STATE_WIREFRAME   = 64,
   GL_STATE_LIGHTING    = 128,
};

/* internal shaders */
enum {
   GL_SHADER_BASE,
   GL_SHADER_COLOR,
   GL_SHADER_BASE_LIGHTING,
   GL_SHADER_COLOR_LIGHTING,
   GL_SHADER_TEXT,
   GL_SHADER_LAST
};

/* global data */
typedef struct __OpenGLstate {
   GLchar attrib[GLHCK_ATTRIB_COUNT];
   GLuint flags;
   glhckBlendingMode blenda, blendb;
   glhckCullFaceType cullFace;
} __OpenGLstate;

typedef struct __OpenGLrender {
   struct __OpenGLstate state;
   GLsizei indexCount;
   kmMat4 projection;
   kmMat4 view;
   kmMat4 orthographic;
   glhckShader *shader[GL_SHADER_LAST];
   glhckHwBuffer *sharedUBO;
} __OpenGLrender;
static __OpenGLrender _OpenGL;

/* check if we have certain draw state active */
#define GL_HAS_STATE(x) (_OpenGL.state.flags & x)

/* ---- Render API ---- */

/* \brief set time to renderer */
static void rTime(float time)
{
   CALL(2, "%f", time);

   /* update time on shared UBO */
   if (_OpenGL.sharedUBO)
      glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckTime", sizeof(float), &time);
}

/* \brief set projection matrix */
static void rSetProjection(const kmMat4 *m)
{
   CALL(2, "%p", m);

   /* update projection on shared UBO */
   if (_OpenGL.sharedUBO)
      glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckProjection", sizeof(kmMat4), &m[0]);

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

/* \brief set view matrix */
static void rSetView(const kmMat4 *m)
{
   CALL(2, "%p", m);

   /* update view on shared UBO */
   if (_OpenGL.sharedUBO)
      glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckView", sizeof(kmMat4), &m[0]);

   if (m != &_OpenGL.view)
      memcpy(&_OpenGL.view, m, sizeof(kmMat4));
}

/* \brief get current view */
static const kmMat4* rGetView(void)
{
   TRACE(2);
   RET(2, "%p", &_OpenGL.view);
   return &_OpenGL.view;
}

/* \brief set orthographic projection for 2D drawing */
static void rSetOrthographic(const kmMat4 *m)
{
   CALL(2, "%p", m);

   /* update projection on shared UBO */
   if (_OpenGL.sharedUBO)
      glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckOrthographic", sizeof(kmMat4), &m[0]);

   if (m != &_OpenGL.orthographic)
      memcpy(&_OpenGL.orthographic, m, sizeof(kmMat4));
}

/* \brief resize viewport */
static void rViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   kmMat4 ortho;
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
   assert(width > 0 && height > 0);

   /* set viewport */
   GL_CALL(glViewport(x, y, width, height));

   /* create orthographic projection matrix */
   kmMat4OrthographicProjection(&ortho, 0, width, height, 0, -1, 1);
   rSetOrthographic(&ortho);
}

/* \brief bind light to renderer */
static void rLightBind(glhckLight *light)
{
   glhckCamera *camera;
   glhckObject *object;

   if (!_OpenGL.sharedUBO)
      return;

   if ((light == GLHCKRD()->light && !light->object->view.update) || !light)
      return;

   camera = GLHCKRD()->camera;
   object = glhckLightGetObject(light);
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Position", sizeof(kmVec3), (void*)glhckObjectGetPosition(object));
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Target", sizeof(kmVec3), (void*)glhckObjectGetTarget(object));
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Cutout", sizeof(kmVec2), (void*)&light->cutout);
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Atten", sizeof(kmVec3), (void*)&light->atten);
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Diffuse", sizeof(GLfloat)*3, &((GLfloat[]){
            object->material.diffuse.r,
            object->material.diffuse.g,
            object->material.diffuse.b}));
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.PointLight", sizeof(GLfloat), (float*)&light->pointLightFactor);
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Near", sizeof(GLfloat), (float*)&camera->view.near);
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Far", sizeof(GLfloat), (float*)&camera->view.far);
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.Projection", sizeof(kmMat4), (void*)glhckCameraGetProjectionMatrix(camera));
   glhckHwBufferFillUniform(_OpenGL.sharedUBO, "GlhckLight.View", sizeof(kmMat4), (void*)glhckCameraGetViewMatrix(camera));
}

/* \brief link program from 2 compiled shader objects */
static unsigned int rProgramLink(GLuint vertexShader, GLuint fragmentShader)
{
   GLuint obj = 0;
   GLchar *log = NULL;
   GLsizei logSize;
   CALL(0, "%u, %u", vertexShader, fragmentShader);

   /* create program object */
   if (!(obj = glCreateProgram()))
      goto fail;

   /* bind locations */
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_VERTEX,  "GlhckVertex"));
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_NORMAL,  "GlhckNormal"));
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_COLOR,   "GlhckColor"));
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_TEXTURE, "GlhckUV0"));

   /* link the shaders to program */
   GL_CALL(glAttachShader(obj, vertexShader));
   GL_CALL(glAttachShader(obj, fragmentShader));
   GL_CALL(glLinkProgram(obj));

   /* attach glhck UBO to every shader */
   glhProgramAttachUniformBuffer(obj, "GlhckUBO", 0);

   /* dump the log incase of error */
   GL_CALL(glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &logSize));
   if (logSize > 1) {
      log = malloc(logSize);
      GL_CALL(glGetProgramInfoLog(obj, logSize, NULL, log));
      printf("-- %u,%u --\n%s^^ %u,%u ^^\n",
            vertexShader, fragmentShader, log, vertexShader, fragmentShader);
      if (_glhckStrupstr(log, "error")) goto fail;
      free(log);
   }

   RET(0, "%u", obj);
   return obj;

fail:
   IFDO(free, log);
   if (obj) GL_CALL(glDeleteProgram(obj));
   RET(0, "%d", 0);
   return 0;
}

/* \brief compile shader object from effect key and optional contents */
static GLuint rShaderCompile(glhckShaderType type, const GLchar *effectKey, const GLchar *contentsFromMemory)
{
   GLuint obj = 0;
   const GLchar *contents;
   GLchar *log = NULL;
   GLsizei logSize;
   CALL(0, "%s, %s", effectKey, contentsFromMemory);

   /* if no effect key given, compile against internal shaders */
   if (!effectKey) {
      contentsFromMemory = _glhckBaseShader;
      effectKey = (type==GLHCK_VERTEX_SHADER?".GLhck.Base.Vertex":".GLhck.Base.Fragment");
   }

   /* wrangle effect key */
   if (!(contents = glswGetShader(effectKey, contentsFromMemory))) {
      DEBUG(GLHCK_DBG_ERROR, "%s", glswGetError());
      goto fail;
   }

   /* create shader object */
   if (!(obj = glCreateShader(glhShaderTypeForGlhckType(type))))
      goto fail;

   /* feed the shader the contents */
   GL_CALL(glShaderSource(obj, 1, &contents, NULL));
   GL_CALL(glCompileShader(obj));

   /* dump the log incase of error */
   GL_CALL(glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &logSize));
   if (logSize > 1) {
      log = malloc(logSize);
      GL_CALL(glGetShaderInfoLog(obj, logSize, NULL, log));
      printf("-- %s --\n%s^^ %s ^^\n", effectKey, log, effectKey);
      if (_glhckStrupstr(log, "error")) goto fail;
      free(log);
   }

   RET(0, "%u", obj);
   return obj;

fail:
   IFDO(free, log);
   if (obj) GL_CALL(glDeleteShader(obj));
   RET(0, "%d", 0);
   return 0;
}

/* helper for checking state changes */
#define GL_STATE_CHANGED(x) (_OpenGL.state.flags & x) != (old.flags & x)

/* \brief set needed state from object data */
static inline void rMaterialState(const _glhckObject *object)
{
   GLuint i;
   __OpenGLstate old   = _OpenGL.state;
   _OpenGL.state.flags = 0; /* reset this state */

   /* always draw vertices */
   _OpenGL.state.attrib[GLHCK_ATTRIB_VERTEX] = 1;

   /* enable normals always for now */
   _OpenGL.state.attrib[GLHCK_ATTRIB_NORMAL] = glhckVertexTypeHasNormal(object->geometry->vertexType);

   /* need color? */
   _OpenGL.state.attrib[GLHCK_ATTRIB_COLOR] = (glhckVertexTypeHasColor(object->geometry->vertexType) &&
         object->material.flags & GLHCK_MATERIAL_COLOR);

   /* need texture? */
   _OpenGL.state.attrib[GLHCK_ATTRIB_TEXTURE] = object->material.texture?1:0;
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

   /* cull face */
   _OpenGL.state.cullFace = GLHCKRP()->cullFace;

   /* blending modes */
   _OpenGL.state.blenda = object->material.blenda;
   _OpenGL.state.blendb = object->material.blendb;

   /* lighting */
   _OpenGL.state.flags |=
      (object->material.flags & GLHCK_MATERIAL_LIGHTING && GLHCKRD()->light)?GL_STATE_LIGHTING:0;

   /* disabled pass bits override the above.
    * it means that we don't want something for this render pass. */
   if (!(GLHCKRP()->flags & GLHCK_PASS_DEPTH))
      _OpenGL.state.flags &= ~GL_STATE_DEPTH;
   if (!(GLHCKRP()->flags & GLHCK_PASS_CULL))
      _OpenGL.state.flags &= ~GL_STATE_CULL;
   if (!(GLHCKRP()->flags & GLHCK_PASS_BLEND))
      _OpenGL.state.flags &= ~GL_STATE_BLEND;
   if (!(GLHCKRP()->flags & GLHCK_PASS_TEXTURE))
      _OpenGL.state.flags &= ~GL_STATE_TEXTURE;
   if (!(GLHCKRP()->flags & GLHCK_PASS_OBB))
      _OpenGL.state.flags &= ~GL_STATE_DRAW_OBB;
   if (!(GLHCKRP()->flags & GLHCK_PASS_AABB))
      _OpenGL.state.flags &= ~GL_STATE_DRAW_AABB;
   if (!(GLHCKRP()->flags & GLHCK_PASS_WIREFRAME))
      _OpenGL.state.flags &= ~GL_STATE_WIREFRAME;
   if (!(GLHCKRP()->flags & GLHCK_PASS_LIGHTING))
      _OpenGL.state.flags &= ~GL_STATE_LIGHTING;

   /* pass blend override */
   if (GLHCKRP()->blenda != GLHCK_ZERO || GLHCKRP()->blendb != GLHCK_ZERO) {
      _OpenGL.state.flags |= GL_STATE_BLEND;
      _OpenGL.state.blenda = GLHCKRP()->blenda;
      _OpenGL.state.blendb = GLHCKRP()->blendb;
   }

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
         glhCullFace(_OpenGL.state.cullFace);
      } else {
         GL_CALL(glDisable(GL_CULL_FACE));
      }
   } else if (GL_HAS_STATE(GL_STATE_CULL)) {
      if (_OpenGL.state.cullFace != old.cullFace) {
         glhCullFace(_OpenGL.state.cullFace);
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

   /* check attribs */
   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i) {
      if (_OpenGL.state.attrib[i] != old.attrib[i]) {
         if (_OpenGL.state.attrib[i]) {
            GL_CALL(glEnableVertexAttribArray(i));
         } else {
            GL_CALL(glDisableVertexAttribArray(i));
         }
      }
   }

   /* bind texture if used */
   if (GL_HAS_STATE(GL_STATE_TEXTURE))
      glhckTextureBind(object->material.texture);
   else if (GL_STATE_CHANGED(GL_STATE_TEXTURE))
      glhckTextureUnbind(GLHCK_TEXTURE_2D);
}

#undef GL_STATE_CHANGED

/* helper macro for passing geometry */
#define geometryV2ToOpenGL(vprec, nprec, tprec, type, tunion)                          \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_VERTEX])                                      \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 2, vprec, 0,                  \
               sizeof(type), &geometry->vertices.tunion[0].vertex));                   \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_NORMAL])                                      \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_NORMAL, 3, nprec, (nprec!=GL_FLOAT),  \
               sizeof(type), &geometry->vertices.tunion[0].normal));                   \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_TEXTURE])                                     \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_TEXTURE, 2, tprec, (tprec!=GL_FLOAT), \
               sizeof(type), &geometry->vertices.tunion[0].coord));                    \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_COLOR])                                       \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, 1,        \
               sizeof(type), &geometry->vertices.tunion[0].color));

#define geometryV3ToOpenGL(vprec, nprec, tprec, type, tunion)                          \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_VERTEX])                                      \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, vprec, 0,                  \
             sizeof(type), &geometry->vertices.tunion[0].vertex));                     \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_NORMAL])                                      \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_NORMAL, 3, nprec, (nprec!=GL_FLOAT),  \
               sizeof(type), &geometry->vertices.tunion[0].normal));                   \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_TEXTURE])                                     \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_TEXTURE, 2, tprec, (tprec!=GL_FLOAT), \
               sizeof(type), &geometry->vertices.tunion[0].coord));                    \
   if (_OpenGL.state.attrib[GLHCK_ATTRIB_COLOR])                                       \
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, 1,        \
               sizeof(type), &geometry->vertices.tunion[0].color));

/* \brief pass interleaved vertex data to OpenGL nicely. */
static inline void rGeometryPointer(const glhckGeometry *geometry)
{
   // printf("%s dd) : %u\n", glhckVertexTypeString(geometry->vertexType), geometry->vertexCount, geometry->textureRange);

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

      default:break;
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

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && _OpenGL.state.attrib[i]) {
         GL_CALL(glDisableVertexAttribArray(i));
      }

   kmMat4 identity;
   kmMat4Identity(&identity);
   glhckShaderBind(_OpenGL.shader[GL_SHADER_COLOR]);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&identity);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1, &((GLfloat[]){255,0,0,255}));

   GL_CALL(glLineWidth(4));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glLineWidth(1));

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && _OpenGL.state.attrib[i]) {
         GL_CALL(glEnableVertexAttribArray(i));
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

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && _OpenGL.state.attrib[i]) {
         GL_CALL(glDisableVertexAttribArray(i));
      }

   glhckShaderBind(_OpenGL.shader[GL_SHADER_COLOR]);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&object->view.matrix);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1, &((GLfloat[]){0,255,0,255}));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && _OpenGL.state.attrib[i]) {
         GL_CALL(glEnableVertexAttribArray(i));
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

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && _OpenGL.state.attrib[i]) {
         GL_CALL(glDisableVertexAttribArray(i));
      }

   kmMat4 identity;
   kmMat4Identity(&identity);
   glhckShaderBind(_OpenGL.shader[GL_SHADER_COLOR]);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&identity);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1, &((GLfloat[]){0,0,255,255}));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && _OpenGL.state.attrib[i]) {
         GL_CALL(glEnableVertexAttribArray(i));
      }
}

/* \brief begin object render */
static inline void rObjectStart(const _glhckObject *object)
{
   rMaterialState(object);

   /* pick shader */
   if (GLHCKRP()->shader) {
      glhckShaderBind(GLHCKRP()->shader);
   } else if (object->material.shader) {
      glhckShaderBind(object->material.shader);
   } else {
      if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
         if (GL_HAS_STATE(GL_STATE_LIGHTING)) {
            glhckShaderBind(_OpenGL.shader[GL_SHADER_BASE_LIGHTING]);
         } else {
            glhckShaderBind(_OpenGL.shader[GL_SHADER_BASE]);
         }
      } else {
         if (GL_HAS_STATE(GL_STATE_LIGHTING)) {
            glhckShaderBind(_OpenGL.shader[GL_SHADER_COLOR_LIGHTING]);
         } else {
            glhckShaderBind(_OpenGL.shader[GL_SHADER_COLOR]);
         }
      }
   }

   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&object->view.matrix);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1,
         &((GLfloat[]){
            object->material.diffuse.r,
            object->material.diffuse.g,
            object->material.diffuse.b,
            object->material.diffuse.a}));
}

/* \brief end object render */
static inline void rObjectEnd(const _glhckObject *object)
{
   glhckGeometryType type = object->geometry->type;

   /* switch to wireframe if requested */
   if (GL_HAS_STATE(GL_STATE_WIREFRAME)) {
      type = (object->geometry->type==GLHCK_TRIANGLES ? GLHCK_LINES:GLHCK_LINE_STRIP);
   }

   /* draw geometry */
   glhGeometryRender(object->geometry, type);

   /* draw axis-aligned bounding box, if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_AABB))
      rAABBRender(object);

   /* draw oriented bounding box, if requested */
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

/* \brief render text */
static inline void rTextRender(const _glhckText *text)
{
   glhckTexture *old;
   __GLHCKtextTexture *texture;
   CALL(2, "%p", text);

   /* set states */
   if (!GL_HAS_STATE(GL_STATE_CULL)) {
      _OpenGL.state.flags |= GL_STATE_CULL;
      _OpenGL.state.cullFace = GLHCK_CULL_BACK;
      GL_CALL(glEnable(GL_CULL_FACE));
      GL_CALL(glCullFace(GL_BACK));
   } else if (_OpenGL.state.cullFace != GLHCK_CULL_BACK) {
      GL_CALL(glCullFace(GL_BACK));
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
      _OpenGL.state.attrib[GLHCK_ATTRIB_TEXTURE] = 1;
      GL_CALL(glEnableVertexAttribArray(GLHCK_ATTRIB_TEXTURE));
   }

   if (!_OpenGL.state.attrib[GLHCK_ATTRIB_VERTEX]) {
      _OpenGL.state.attrib[GLHCK_ATTRIB_VERTEX] = 1;
      GL_CALL(glEnableVertexAttribArray(GLHCK_ATTRIB_VERTEX));
   }

   if (_OpenGL.state.attrib[GLHCK_ATTRIB_NORMAL]) {
      _OpenGL.state.attrib[GLHCK_ATTRIB_NORMAL] = 0;
      GL_CALL(glDisableVertexAttribArray(GLHCK_ATTRIB_NORMAL));
   }

   if (_OpenGL.state.attrib[GLHCK_ATTRIB_COLOR]) {
      _OpenGL.state.attrib[GLHCK_ATTRIB_COLOR] = 0;
      GL_CALL(glDisableVertexAttribArray(GLHCK_ATTRIB_COLOR));
   }

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glDisable(GL_DEPTH_TEST));
   }

   if (text->shader) glhckShaderBind(text->shader);
   else glhckShaderBind(_OpenGL.shader[GL_SHADER_TEXT]);
   glhckShaderSetUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1,
         &((GLfloat[]){text->color.r, text->color.g, text->color.b, text->color.a}));

   /* store old texture */
   old = GLHCKRD()->texture[GLHCK_TEXTURE_2D];

   for (texture = text->tcache; texture;
        texture = texture->next) {
      if (!texture->geometry.vertexCount)
         continue;
      GL_CALL(glBindTexture(GL_TEXTURE_2D, texture->object));
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 2, (GLHCK_TEXT_FLOAT_PRECISION?GL_FLOAT:GL_SHORT), 0,
               (GLHCK_TEXT_FLOAT_PRECISION?sizeof(glhckVertexData2f):sizeof(glhckVertexData2s)),
               &texture->geometry.vertexData[0].vertex));
      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_TEXTURE, 2, (GLHCK_TEXT_FLOAT_PRECISION?GL_FLOAT:GL_SHORT),
               (GLHCK_TEXT_FLOAT_PRECISION?0:1),
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

   if (_OpenGL.state.cullFace != GLHCK_CULL_BACK) {
      glhCullFace(_OpenGL.state.cullFace);
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
   int i;
   TRACE(0);

   /* free shaders */
   if (_GLHCKlibrary.world.slist) {
      for (i = 0; i != GL_SHADER_LAST; ++i) {
         NULLDO(glhckShaderFree, _OpenGL.shader[i]);
      }
   }

   /* free hw buffers */
   if (_GLHCKlibrary.world.hlist) {
      NULLDO(glhckHwBufferFree, _OpenGL.sharedUBO);
   }

   /* shutdown shader wrangler */
   glswShutdown();

   /* this tells library that we are no longer alive. */
   GLHCK_RENDER_TERMINATE(RENDER_NAME);
}

/* ---- Main ---- */

/* \brief renderer main function */
void _glhckRenderOpenGL(void)
{
   int i;
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

   /* init shader wrangler */
   if (!glswInit())
      goto fail;

   /* default path &&  extension */
   glswSetPath("shaders/", ".glsl");

   /* default GLhck shader directivies */
   glswAddDirectiveToken("GLhck", "#version 140");

   /* vertex directivies */
   glswAddDirectiveToken("Vertex",
         "in vec3 GlhckVertex;"
         "in vec3 GlhckNormal;"
         "in vec4 GlhckColor;"
         "in vec2 GlhckUV0;"
         "uniform mat4 GlhckModel;"
         "out vec4 GlhckFVertexWorld;"
         "out vec4 GlhckFVertexView;"
         "out vec3 GlhckFNormalWorld;"
         "out vec2 GlhckFUV0;"
         "out vec4 GlhckFSC0;");

   /* fragment directivies */
   glswAddDirectiveToken("Fragment",
         "in vec4 GlhckFVertexWorld;"
         "in vec4 GlhckFVertexView;"
         "in vec3 GlhckFNormalWorld;"
         "in vec2 GlhckFUV0;"
         "in vec4 GlhckFSC0;"
         "out vec4 GlhckFragColor;");

   /* vertex and fragment directivies */
   glswAddDirectiveToken("GLhck",
         "struct glhckLight {"
         "  vec3 Position;"
         "  vec3 Target;"
         "  vec2 Cutout;"
         "  vec3 Atten;"
         "  vec3 Diffuse;"
         "  float PointLight;"
         "  float Near;"
         "  float Far;"
         "  mat4 Projection;"
         "  mat4 View;"
         "};"
         "uniform GlhckUBO {"
         "  float GlhckTime;"
         "  mat4 GlhckProjection;"
         "  mat4 GlhckOrthographic;"
         "  mat4 GlhckView;"
         "  glhckLight GlhckLight;"
         "};"
         "struct glhckMaterial {"
         "  vec3 Ambient;"
         "  vec4 Diffuse;"
         "  vec3 Specular;"
         "  float Shininess;"
         "  vec2 TextureOffset;"
         "  vec2 TextureScale;"
         "};"
         "uniform glhckMaterial GlhckMaterial ="
         "  glhckMaterial(vec3(1,1,1),"
         "                vec4(255,255,255,255),"
         "                vec3(200,200,200),0.9,"
         "                vec2(0,0),vec2(1,1));"
         "uniform sampler2D GlhckTexture0;");

   /* lighting functions */
   glswAddDirectiveToken("Lighting",
         "vec3 glhckLighting(vec3 Diffuse) {"
         "  vec3 Color    = Diffuse * GlhckMaterial.Ambient/255.0;"
         "  vec3 LDiffuse = GlhckLight.Diffuse/255.0;"
         "  vec3 LAtten   = GlhckLight.Atten*0.01;"
         "  vec2 LCutout  = GlhckLight.Cutout;"
         "  vec3 Specular = Diffuse * GlhckMaterial.Specular/255.0;"
         "  vec3 Normal   = normalize(GlhckFNormalWorld);"
         "  vec3 ViewVec  = normalize(-GlhckFVertexView.xyz);"
         "  vec3 LightVec = normalize(GlhckLight.Position - GlhckFVertexWorld.xyz);"
         "  vec3 LightDir = normalize(GlhckLight.Target   - GlhckFVertexWorld.xyz);"
         ""
         "  float L = clamp(dot(Normal, LightVec), 0.0, 1.0);"
         "  float Spotlight = max(-dot(LightVec, LightDir), 0.0);"
         "  float SpotlightFade = clamp((LCutout.x - Spotlight) / (LCutout.x - LCutout.y), 0.0, 1.0);"
         "  Spotlight = clamp(pow(Spotlight * SpotlightFade, 1.0), GlhckLight.PointLight, 1.0);"
         "  vec3  R = normalize(-reflect(LightVec, Normal));"
         "  float S = pow(max(dot(R, ViewVec), 0.0), GlhckMaterial.Shininess);"
         "  float D = distance(GlhckFVertexWorld.xyz, GlhckLight.Position);"
         "  float A = 1.0 / (LAtten.x + (LAtten.y * D) + (LAtten.z * D * D));"
         "  Color += (Diffuse * L + (Specular * S)) * LDiffuse * A * Spotlight;"
         "  return Color;"
         "}");

   /* unpacking functions */
   glswAddDirectiveToken("Unpacking",
         "float glhckUnpack(vec4 color) {"
         "  const vec4 bitShifts = vec4(1.0, 1.0/255.0, 1.0/(255.0 * 255.0), 1.0/(255.0 * 255.0 * 255.0));"
         "  return dot(color, bitShifts);"
         "}"
         "float glhckUnpackHalf(vec2 color) {"
         "  return color.x + (color.y / 255.0);"
         "}");

   /* packing functions */
   glswAddDirectiveToken("Packing",
         "vec4 glhckPack(float depth) {"
         "  const vec4 bias = vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);"
         "  float r = depth;"
         "  float g = fract(r * 255.0);"
         "  float b = fract(g * 255.0);"
         "  float a = fract(b * 255.0);"
         "  vec4 color = vec4(r, g, b, a);"
         "  return color - (color.yzww * bias);"
         "}"
         "vec2 glhckPackHalf(float depth) {"
         "  const vec2 bias = vec2(1.0/255.0, 0.0);"
         "  vec2 color = vec2(depth, fract(depth * 255.0));"
         "  return color - (color.yy * bias);"
         "}");

   /* shadow functions */
   glswAddDirectiveToken("ShadowMapping",
         "float glhckChebychevInequality(vec2 Moments, float Depth) {"
         "  if (Depth <= Moments.x)"
         "     return 1.0;"
         ""
         "  float Variance = Moments.y - (Moments.x * Moments.x);"
         "  Variance = max(Variance, 0.02);"
         "  float D = Depth - Moments.x;"
         "  return Variance/(Variance + D * D);"
         "}"
         ""
         "float glhckShadow(sampler2D ShadowMap, float Darkness) {"
         "  float LinearScale = 1.0/(GlhckLight.Far - GlhckLight.Near);"
         "  vec3 ShadowCoord = GlhckFSC0.xyz/GlhckFSC0.w;"
         "  ShadowCoord.z = length(GlhckFVertexView) * LinearScale;"
         "  vec4 Texel = texture2D(ShadowMap, ShadowCoord.xy);"
         "  vec2 Moments = vec2(glhckUnpackHalf(Texel.xy), glhckUnpackHalf(Texel.zw));"
         "  float Shadow = glhckChebychevInequality(Moments, ShadowCoord.z);"
         "  return Darkness-Shadow;"
         "}");

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

   /* lights */
   GLHCK_RENDER_FUNC(lightBind, rLightBind);

   /* renderbuffer objects */
   GLHCK_RENDER_FUNC(renderbufferGenerate, glGenRenderbuffers);
   GLHCK_RENDER_FUNC(renderbufferDelete, glDeleteRenderbuffers);
   GLHCK_RENDER_FUNC(renderbufferBind, glhRenderbufferBind);
   GLHCK_RENDER_FUNC(renderbufferStorage, glhRenderbufferStorage);

   /* framebuffer objects */
   GLHCK_RENDER_FUNC(framebufferGenerate, glGenFramebuffers);
   GLHCK_RENDER_FUNC(framebufferDelete, glDeleteFramebuffers);
   GLHCK_RENDER_FUNC(framebufferBind, glhFramebufferBind);
   GLHCK_RENDER_FUNC(framebufferTexture, glhFramebufferTexture);
   GLHCK_RENDER_FUNC(framebufferRenderbuffer, glhFramebufferRenderbuffer);

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
   GLHCK_RENDER_FUNC(programBind, glUseProgram);
   GLHCK_RENDER_FUNC(programLink, rProgramLink);
   GLHCK_RENDER_FUNC(programDelete, glDeleteProgram);
   GLHCK_RENDER_FUNC(programSetUniform, glhProgramSetUniform);
   GLHCK_RENDER_FUNC(programUniformBufferList, glhProgramUniformBufferList);
   GLHCK_RENDER_FUNC(programAttributeList, glhProgramAttributeList);
   GLHCK_RENDER_FUNC(programUniformList, glhProgramUniformList);
   GLHCK_RENDER_FUNC(programAttachUniformBuffer, glhProgramAttachUniformBuffer);
   GLHCK_RENDER_FUNC(shaderCompile, rShaderCompile);
   GLHCK_RENDER_FUNC(shaderDelete, glDeleteShader);
   GLHCK_RENDER_FUNC(shadersPath, glswSetPath);
   GLHCK_RENDER_FUNC(shadersDirectiveToken, glswAddDirectiveToken);

   /* drawing functions */
   GLHCK_RENDER_FUNC(setProjection, rSetProjection);
   GLHCK_RENDER_FUNC(getProjection, rGetProjection);
   GLHCK_RENDER_FUNC(setView, rSetView);
   GLHCK_RENDER_FUNC(getView, rGetView);
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

   /* create shared uniform buffer objects */
   if (!(_OpenGL.sharedUBO = glhckHwBufferNew()))
      goto fail;

   /* compile internal shaders */
   _OpenGL.shader[GL_SHADER_BASE] = glhckShaderNew(NULL, NULL, _glhckBaseShader);
   _OpenGL.shader[GL_SHADER_COLOR] = glhckShaderNew(NULL, ".GLhck.Color.Fragment", _glhckBaseShader);
   _OpenGL.shader[GL_SHADER_BASE_LIGHTING] = glhckShaderNew(NULL, ".GLhck.Base.Lighting.Fragment", _glhckBaseShader);
   _OpenGL.shader[GL_SHADER_COLOR_LIGHTING] = glhckShaderNew(NULL, ".GLhck.Color.Lighting.Fragment", _glhckBaseShader);
   _OpenGL.shader[GL_SHADER_TEXT] = glhckShaderNew(".GLhck.Text.Vertex", ".GLhck.Text.Fragment", _glhckBaseShader);

   for (i = 0; i != GL_SHADER_LAST; ++i)
      if (!_OpenGL.shader[i]) goto fail;

   /* initialize the UBO automaticlaly from shader */
   glhckHwBufferCreateUniformBufferFromShader(_OpenGL.sharedUBO, _OpenGL.shader[GL_SHADER_BASE], "GlhckUBO", GLHCK_BUFFER_DYNAMIC_DRAW);
   glhckHwBufferBindRange(_OpenGL.sharedUBO, 0, 0, _OpenGL.sharedUBO->size);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

   /* ok */
   return;

fail:
   renderTerminate();
   return;
}

/* vim: set ts=8 sw=3 tw=0 :*/
