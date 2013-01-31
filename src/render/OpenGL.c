#include "../internal.h"
#include "render.h"
#include <limits.h>
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
"-- Vertex\n"
"#version 140\n"
"in vec3 Vertex;"
"in vec3 Normal;"
"in vec2 UV0;"
"in vec4 Color;"
"uniform float Time;"
"uniform mat4 Projection;"
"uniform mat4 Model;"
"out vec3 fNormal;"
"out vec2 fUV0;"
"out vec4 fColor;"
"void main() {"
"  fNormal = Normal;"
"  fUV0 = UV0;"
"  fColor = Color;"
"  gl_Position = Projection * Model * vec4(Vertex, 1.0);"
"}\n"
"-- Fragment\n"
"#version 140\n"
"in vec3 fNormal;"
"in vec2 fUV0;"
"in vec4 fColor;"
"uniform float Time;"
"uniform sampler2D Texture0;"
"uniform vec4 MaterialColor;"
"out vec4 FragColor;"
"void main() {"
"  FragColor = texture2D(Texture0, fUV0) * MaterialColor/255.0;"
"}\n";

static const char *_glhckTextShader =
"-- Vertex\n"
"#version 140\n"
"in vec3 Vertex;"
"in vec2 UV0;"
"uniform mat4 Projection;"
"out vec2 fUV0;"
"void main() {"
"  fUV0 = UV0;"
"  gl_Position = Projection * vec4(Vertex, 1.0);"
"}\n"
"-- Fragment\n"
"#version 140\n"
"in vec2 fUV0;"
"uniform sampler2D Texture0;"
"uniform vec4 MaterialColor;"
"out vec4 FragColor;"
"void main() {"
"  FragColor = MaterialColor/255.0 * texture2D(Texture0, fUV0).a;"
"}\n";

static const char *_glhckColorShader =
"-- Vertex\n"
"#version 140\n"
"in vec3 Vertex;"
"uniform mat4 Projection;"
"uniform mat4 Model;"
"void main() {"
"  gl_Position = Projection * Model * vec4(Vertex, 1.0);"
"}\n"
"-- Fragment\n"
"#version 140\n"
"uniform vec4 MaterialColor;"
"out vec4 FragColor;"
"void main() {"
"  FragColor = MaterialColor/255.0;"
"}\n";

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
   glhckShader *baseShader;
   glhckShader *textShader;
   glhckShader *colorShader;
} __OpenGLrender;
static __OpenGLrender _OpenGL;

/* check if we have certain draw state active */
#define GL_HAS_STATE(x) (_OpenGL.state.flags & x)

/* ---- Render API ---- */

/* \brief set projection matrix */
static void rSetProjection(const kmMat4 *m)
{
   CALL(2, "%p", m);
   glhckShaderSetUniform(GLHCKRD()->shader, "Projection", 1, (GLfloat*)&m[0]);

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
static void rViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
   assert(width > 0 && height > 0);

   /* set viewport */
   GL_CALL(glViewport(x, y, width, height));

   /* create orthographic projection matrix */
   kmMat4OrthographicProjection(&_OpenGL.orthographic, 0, width, height, 0, -1, 1);
}

/* \brief get attribute list from program */
static _glhckShaderAttribute* rProgramAttributeList(GLuint obj)
{
   GLenum type;
   GLchar name[255];
   GLint count, i;
   GLsizei size, length;
   _glhckShaderAttribute *attributes = NULL, *a, *an;
   CALL(0, "%u", obj);

   /* get attribute count */
   GL_CALL(glGetProgramiv(obj, GL_ACTIVE_ATTRIBUTES, &count));
   if (!count) goto no_attributes;

   for (i = 0; i != count; ++i) {
      /* get attribute information */
      GL_CALL(glGetActiveAttrib(obj, i, sizeof(name)-1, &length, &size, &type, name));

      /* allocate new attribute slot */
      for (a = attributes; a && a->next; a = a->next);
      if (a) a = a->next    = _glhckCalloc(1, sizeof(_glhckShaderAttribute));
      else   a = attributes = _glhckCalloc(1, sizeof(_glhckShaderAttribute));
      if (!a || !(a->name = _glhckMalloc(length+1)))
         goto fail;

      /* assign information */
      memcpy(a->name, name, length+1);
      a->typeName = glhShaderVariableNameForOpenGLConstant(type);
      a->location = glGetAttribLocation(obj, a->name);
      a->type = glhGlhckShaderVariableTypeForGLType(type);
      a->size = size;
   }

   RET(0, "%p", attributes);
   return attributes;

fail:
no_attributes:
   if (attributes) {
      for (a = attributes; a; a = an) {
         an = a->next;
         _glhckFree(a->name);
         _glhckFree(a);
      }
   }
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get uniform list from program */
static _glhckShaderUniform* rProgramUniformList(GLuint obj)
{
   GLenum type;
   GLchar name[255];
   GLint count, i;
   GLsizei length, size;
   _glhckShaderUniform *uniforms = NULL, *u, *un;
   CALL(0, "%u", obj);

   /* get uniform count */
   GL_CALL(glGetProgramiv(obj, GL_ACTIVE_UNIFORMS, &count));
   if (!count) goto no_uniforms;

   for (i = 0; i != count; ++i) {
      /* get uniform information */
      GL_CALL(glGetActiveUniform(obj, i, sizeof(name)-1, &length, &size, &type, name));

      /* allocate new uniform slot */
      for (u = uniforms; u && u->next; u = u->next);
      if (u) u = u->next  = _glhckCalloc(1, sizeof(_glhckShaderUniform));
      else   u = uniforms = _glhckCalloc(1, sizeof(_glhckShaderUniform));
      if (!u || !(u->name = _glhckMalloc(length+1)))
      goto fail;

      /* assign information */
      memcpy(u->name, name, length+1);
      u->typeName = glhShaderVariableNameForOpenGLConstant(type);
      u->location = glGetUniformLocation(obj, u->name);
      u->type = glhGlhckShaderVariableTypeForGLType(type);
      u->size = size;
   }

   RET(0, "%p", uniforms);
   return uniforms;

fail:
no_uniforms:
   if (uniforms) {
      for (u = uniforms; u; u = un) {
         un = u->next;
         _glhckFree(u);
      }
   }
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief set shader uniform */
static void rProgramSetUniform(GLuint obj, _glhckShaderUniform *uniform, GLsizei count, GLvoid *value)
{
   CALL(0, "%u", obj);

   /* automatically figure out the data type */
   switch (uniform->type) {
      case GLHCK_SHADER_FLOAT:
         GL_CALL(glUniform1fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_VEC2:
         GL_CALL(glUniform2fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_VEC3:
         GL_CALL(glUniform3fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_VEC4:
         GL_CALL(glUniform4fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_DOUBLE:
         GL_CALL(glUniform1dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_VEC2:
         GL_CALL(glUniform2dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_VEC3:
         GL_CALL(glUniform3dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_VEC4:
         GL_CALL(glUniform4dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_INT:
      case GLHCK_SHADER_BOOL:
      case GLHCK_SHADER_SAMPLER_1D:
      case GLHCK_SHADER_SAMPLER_2D:
      case GLHCK_SHADER_SAMPLER_3D:
      case GLHCK_SHADER_SAMPLER_CUBE:
      case GLHCK_SHADER_SAMPLER_1D_SHADOW:
      case GLHCK_SHADER_SAMPLER_2D_SHADOW:
      case GLHCK_SHADER_SAMPLER_1D_ARRAY:
      case GLHCK_SHADER_SAMPLER_2D_ARRAY:
      case GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW:
      case GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW:
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE:
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_SAMPLER_CUBE_SHADOW:
      case GLHCK_SHADER_SAMPLER_BUFFER:
      case GLHCK_SHADER_SAMPLER_2D_RECT:
      case GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW:
      case GLHCK_SHADER_INT_SAMPLER_1D:
      case GLHCK_SHADER_INT_SAMPLER_2D:
      case GLHCK_SHADER_INT_SAMPLER_3D:
      case GLHCK_SHADER_INT_SAMPLER_CUBE:
      case GLHCK_SHADER_INT_SAMPLER_1D_ARRAY:
      case GLHCK_SHADER_INT_SAMPLER_2D_ARRAY:
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE:
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_INT_SAMPLER_BUFFER:
      case GLHCK_SHADER_INT_SAMPLER_2D_RECT:
      case GLHCK_SHADER_IMAGE_1D:
      case GLHCK_SHADER_IMAGE_2D:
      case GLHCK_SHADER_IMAGE_3D:
      case GLHCK_SHADER_IMAGE_2D_RECT:
      case GLHCK_SHADER_IMAGE_CUBE:
      case GLHCK_SHADER_IMAGE_BUFFER:
      case GLHCK_SHADER_IMAGE_1D_ARRAY:
      case GLHCK_SHADER_IMAGE_2D_ARRAY:
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE:
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_INT_IMAGE_1D:
      case GLHCK_SHADER_INT_IMAGE_2D:
      case GLHCK_SHADER_INT_IMAGE_3D:
      case GLHCK_SHADER_INT_IMAGE_2D_RECT:
      case GLHCK_SHADER_INT_IMAGE_CUBE:
      case GLHCK_SHADER_INT_IMAGE_BUFFER:
      case GLHCK_SHADER_INT_IMAGE_1D_ARRAY:
      case GLHCK_SHADER_INT_IMAGE_2D_ARRAY:
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE:
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
         GL_CALL(glUniform1iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_INT_VEC2:
      case GLHCK_SHADER_BOOL_VEC2:
         GL_CALL(glUniform2iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_INT_VEC3:
      case GLHCK_SHADER_BOOL_VEC3:
         GL_CALL(glUniform3iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_INT_VEC4:
      case GLHCK_SHADER_BOOL_VEC4:
         GL_CALL(glUniform4iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER:
         GL_CALL(glUniform1uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT_VEC2:
         GL_CALL(glUniform2uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT_VEC3:
         GL_CALL(glUniform3uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT_VEC4:
         GL_CALL(glUniform4uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT2:
         GL_CALL(glUniformMatrix2fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT3:
         GL_CALL(glUniformMatrix3fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT4:
         GL_CALL(glUniformMatrix4fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT2x3:
         GL_CALL(glUniformMatrix2x3fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT2x4:
         GL_CALL(glUniformMatrix2x4fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT3x2:
         GL_CALL(glUniformMatrix3x2fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT3x4:
         GL_CALL(glUniformMatrix3x4fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT4x2:
         GL_CALL(glUniformMatrix4x2fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT4x3:
         GL_CALL(glUniformMatrix4x3fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT2:
         GL_CALL(glUniformMatrix2dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT3:
         GL_CALL(glUniformMatrix3dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT4:
         GL_CALL(glUniformMatrix4dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT2x3:
         GL_CALL(glUniformMatrix2x3dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT2x4:
         GL_CALL(glUniformMatrix2x4dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT3x2:
         GL_CALL(glUniformMatrix3x2dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT3x4:
         GL_CALL(glUniformMatrix3x4dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT4x2:
         GL_CALL(glUniformMatrix4x2dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT4x3:
         GL_CALL(glUniformMatrix4x3dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      default:
         DEBUG(GLHCK_DBG_ERROR, "uniform type not implemented.");
         break;
   }
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
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_VERTEX, "Vertex"));
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_NORMAL, "Normal"));
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_TEXTURE, "UV0"));
   GL_CALL(glBindAttribLocation(obj, GLHCK_ATTRIB_COLOR, "Color"));

   /* link the shaders to program */
   GL_CALL(glAttachShader(obj, vertexShader));
   GL_CALL(glAttachShader(obj, fragmentShader));
   GL_CALL(glLinkProgram(obj));

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
   assert(effectKey);

   /* wrangle effect key */
   if (!(contents = glswGetShader(effectKey, contentsFromMemory)))
      goto fail;

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
      (object->material.blenda != GLHCK_ZERO && object->material.blendb != GLHCK_ZERO)?GL_STATE_BLEND:0;

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
}

#undef GL_STATE_CHANGED

/* helper macro for passing geometry */
#define geometryV2ToOpenGL(vprec, tprec, type, tunion)                              \
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 2, vprec, 0,                  \
            sizeof(type), &geometry->vertices.tunion[0].vertex));                   \
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_TEXTURE, 2, tprec, (tprec!=GL_FLOAT), \
            sizeof(type), &geometry->vertices.tunion[0].coord));                    \
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, 1,        \
            sizeof(type), &geometry->vertices.tunion[0].color));

#define geometryV3ToOpenGL(vprec, tprec, type, tunion)                              \
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, vprec, 0,                  \
            sizeof(type), &geometry->vertices.tunion[0].vertex));                   \
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_NORMAL, 3, vprec, (vprec!=GL_FLOAT),  \
            sizeof(type), &geometry->vertices.tunion[0].normal));                   \
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_TEXTURE, 2, tprec, (tprec!=GL_FLOAT), \
            sizeof(type), &geometry->vertices.tunion[0].coord));                    \
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, 1, \
            sizeof(type), &geometry->vertices.tunion[0].color));

/* \brief pass interleaved vertex data to OpenGL nicely. */
static inline void rGeometryPointer(const glhckGeometry *geometry)
{
   // printf("%s dd) : %u\n", glhckVertexTypeString(geometry->vertexType), geometry->vertexCount, geometry->textureRange);

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
   glhckShaderBind(_OpenGL.colorShader);
   glhckShaderSetUniform(GLHCKRD()->shader, "Projection", 1, (GLfloat*)&_OpenGL.projection);
   glhckShaderSetUniform(GLHCKRD()->shader, "Model", 1, (GLfloat*)&identity);
   glhckShaderSetUniform(GLHCKRD()->shader, "MaterialColor", 1, &((GLfloat[]){255,0,0,255}));

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

   glhckShaderBind(_OpenGL.colorShader);
   glhckShaderSetUniform(GLHCKRD()->shader, "Projection", 1, (GLfloat*)&_OpenGL.projection);
   glhckShaderSetUniform(GLHCKRD()->shader, "Model", 1, (GLfloat*)&object->view.matrix);
   glhckShaderSetUniform(GLHCKRD()->shader, "MaterialColor", 1, &((GLfloat[]){0,255,0,255}));
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
   glhckShaderBind(_OpenGL.colorShader);
   glhckShaderSetUniform(GLHCKRD()->shader, "Projection", 1, (GLfloat*)&_OpenGL.projection);
   glhckShaderSetUniform(GLHCKRD()->shader, "Model", 1, (GLfloat*)&identity);
   glhckShaderSetUniform(GLHCKRD()->shader, "MaterialColor", 1, &((GLfloat[]){0,0,255,255}));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && _OpenGL.state.attrib[i]) {
         GL_CALL(glEnableVertexAttribArray(i));
      }
}

/* \brief begin object render */
static inline void rObjectStart(const _glhckObject *object) {
   /* set object's model */
   glhckShaderBind(_OpenGL.baseShader);
   glhckShaderSetUniform(GLHCKRD()->shader, "Projection", 1, (GLfloat*)&_OpenGL.projection);
   glhckShaderSetUniform(GLHCKRD()->shader, "Model", 1, (GLfloat*)&object->view.matrix);
   glhckShaderSetUniform(GLHCKRD()->shader, "MaterialColor", 1,
         &((GLfloat[]){
            object->material.color.r,
            object->material.color.g,
            object->material.color.b,
            object->material.color.a}));

   /* set states and draw */
   rMaterialState(object);
}

/* \brief end object render */
static inline void rObjectEnd(const _glhckObject *object) {
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
      _OpenGL.state.attrib[GLHCK_ATTRIB_TEXTURE] = 1;
      GL_CALL(glEnableVertexAttribArray(GLHCK_ATTRIB_TEXTURE));
   }

   if (!_OpenGL.state.attrib[GLHCK_ATTRIB_VERTEX]) {
      _OpenGL.state.attrib[GLHCK_ATTRIB_VERTEX] = 1;
      GL_CALL(glEnableVertexAttribArray(GLHCK_ATTRIB_VERTEX));
   }

   if (!_OpenGL.state.attrib[GLHCK_ATTRIB_NORMAL]) {
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

   glhckShaderBind(_OpenGL.textShader);
   glhckShaderSetUniform(GLHCKRD()->shader, "Projection", 1, (GLfloat*)&_OpenGL.orthographic);
   glhckShaderSetUniform(GLHCKRD()->shader, "MaterialColor", 1,
         &((GLfloat[]){text->color.r, text->color.g, text->color.b, text->color.a}));

   for (texture = text->tcache; texture;
        texture = texture->next) {
      if (!texture->geometry.vertexCount)
         continue;
      glhTextureBind(GL_TEXTURE_2D, texture->object);
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

   /* restore glhck texture state */
   glhTextureBindRestore();

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

   /* free shaders */
   NULLDO(glhckShaderFree, _OpenGL.baseShader);
   NULLDO(glhckShaderFree, _OpenGL.textShader);
   NULLDO(glhckShaderFree, _OpenGL.colorShader);

   /* shutdown shader wrangler */
   glswShutdown();

   /* this tells library that we are no longer alive. */
   GLHCK_RENDER_TERMINATE(RENDER_NAME);
}

/* OpenGL bindings */
DECLARE_GL_GEN_FUNC(rGlGenTextures, glGenTextures)
DECLARE_GL_GEN_FUNC(rGlDeleteTextures, glDeleteTextures)
DECLARE_GL_BIND_FUNC(rGlBindTexture, glBindTexture(GL_TEXTURE_2D, object))

DECLARE_GL_GEN_FUNC(rGlGenFramebuffers, glGenFramebuffers);
DECLARE_GL_GEN_FUNC(rGlDeleteFramebuffers, glDeleteFramebuffers);
DECLARE_GL_BIND_FUNC(rGlBindFramebuffer, glBindFramebuffer(GL_FRAMEBUFFER, object))

/* ---- Main ---- */

/* \brief renderer main function */
void _glhckRenderOpenGL(void)
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

   /* init shader wrangler */
   if (!glswInit())
      goto fail;

   /* reset global data */
   if (renderInit() != RETURN_OK)
      goto fail;

   /* register api functions */

   /* textures */
   GLHCK_RENDER_FUNC(textureGenerate, rGlGenTextures);
   GLHCK_RENDER_FUNC(textureDelete, rGlDeleteTextures);
   GLHCK_RENDER_FUNC(textureBind, rGlBindTexture);
   GLHCK_RENDER_FUNC(textureCreate, glhTextureCreate);
   GLHCK_RENDER_FUNC(textureFill, glhTextureFill);

   /* framebuffer objects */
   GLHCK_RENDER_FUNC(framebufferGenerate, rGlGenFramebuffers);
   GLHCK_RENDER_FUNC(framebufferDelete, rGlDeleteFramebuffers);
   GLHCK_RENDER_FUNC(framebufferBind, rGlBindFramebuffer);
   GLHCK_RENDER_FUNC(framebufferLinkWithTexture, glhFramebufferLinkWithTexture);

   /* shader objects */
   GLHCK_RENDER_FUNC(programBind, glUseProgram);
   GLHCK_RENDER_FUNC(programLink, rProgramLink);
   GLHCK_RENDER_FUNC(programDelete, glDeleteProgram);
   GLHCK_RENDER_FUNC(programSetUniform, rProgramSetUniform);
   GLHCK_RENDER_FUNC(programAttributeList, rProgramAttributeList);
   GLHCK_RENDER_FUNC(programUniformList, rProgramUniformList);
   GLHCK_RENDER_FUNC(shaderCompile, rShaderCompile);
   GLHCK_RENDER_FUNC(shaderDelete, glDeleteShader);

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
   GLHCK_RENDER_FUNC(viewport, rViewport);
   GLHCK_RENDER_FUNC(terminate, renderTerminate);

   /* parameters */
   GLHCK_RENDER_FUNC(getIntegerv, glhGetIntegerv);

   /* compile base shader */
   _OpenGL.baseShader = glhckShaderNew("Base.Vertex", "Base.Fragment", _glhckBaseShader);
   if (!_OpenGL.baseShader)
      goto fail;

   /* compile text shader */
   _OpenGL.textShader = glhckShaderNew("Text.Vertex", "Text.Fragment", _glhckTextShader);
   if (!_OpenGL.textShader)
      goto fail;

   /* compile color shader */
   _OpenGL.colorShader = glhckShaderNew("Color.Vertex", "Color.Fragment", _glhckColorShader);
   if (!_OpenGL.colorShader)
      goto fail;

   /* set uniforms */
   glhckShaderSetUniform(_OpenGL.baseShader, "Texture0", 1, &((GLint[]){0}));
   glhckShaderSetUniform(_OpenGL.baseShader, "MaterialColor", 1, &((GLfloat[]){255,255,255,255}));
   glhckShaderSetUniform(_OpenGL.textShader, "Texture0", 1, &((GLint[]){0}));
   glhckShaderSetUniform(_OpenGL.textShader, "MaterialColor", 1, &((GLfloat[]){255,255,255,255}));
   glhckShaderSetUniform(_OpenGL.colorShader, "MaterialColor", 1, &((GLfloat[]){255,255,255,255}));

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
