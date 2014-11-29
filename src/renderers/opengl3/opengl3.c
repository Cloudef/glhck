#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define GLHCK_ATTRIB_COUNT GLHCK_ATTRIB_LAST

#define OPENGL3
#include "include/glad/glad.h"
#include "renderers/helpers/opengl.h"
#include "renderers/renderer.h"
#include "glsw.h"

#if 0
/* This is the base shader for glhck */
static const char *_glhckBaseShader =
"-- GLhck.Base.Vertex\n"
"const mat4 ScaleMatrix ="
"mat4(0.5, 0.0, 0.0, 0.0,"
"     0.0, 0.5, 0.0, 0.0,"
"     0.0, 0.0, 0.5, 0.0,"
"     0.5, 0.5, 0.5, 1.0);"
""
"vec2 vec2RotateBy(vec2 invec, float degrees, vec2 center) {"
"  float radians = degrees * 0.017453;"
"  float cs = cos(radians), sn = sin(radians);"
"  vec2 tmp = invec - center;"
"  float x = tmp.x * cs - tmp.y * sn;"
"  float y = tmp.x * sn + tmp.y * cs;"
"  return vec2(x, y) + center;"
"}\n"
""
"void main() {"
"  vec2 rotated = vec2RotateBy(GlhckUV0, GlhckMaterial.TextureRotation, vec2(0.5, 0.5));"
"  GlhckFVertexWorld = GlhckModel * vec4(GlhckVertex, 1.0);"
"  GlhckFVertexView = GlhckView * GlhckFVertexWorld;"
"  GlhckFNormalWorld = normalize(mat3(GlhckModel) * GlhckNormal);"
"  GlhckFUV0 = GlhckMaterial.TextureOffset + (rotated * GlhckMaterial.TextureScale);"
"  GlhckFSC0 = ScaleMatrix * GlhckLight.Projection * GlhckLight.View * GlhckFVertexWorld;"
"  gl_Position = GlhckProjection * GlhckFVertexView;"
"}\n"

"-- GLhck.Text.Vertex\n"
"void main() {"
"  GlhckFUV0 = GlhckUV0 * GlhckMaterial.TextureScale;"
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
"  vec4 Diffuse = texture2D(GlhckTexture0, GlhckFUV0);"
"  GlhckFragColor = GlhckMaterial.Diffuse/255.0 * Diffuse.aaaa;"
"}\n";

static const glhckColorb overdrawColor = {25,25,25,255};

/* state flags */
enum {
   GL_STATE_DEPTH          = 1<<0,
   GL_STATE_CULL           = 1<<1,
   GL_STATE_BLEND          = 1<<2,
   GL_STATE_TEXTURE        = 1<<3,
   GL_STATE_DRAW_AABB      = 1<<4,
   GL_STATE_DRAW_OBB       = 1<<5,
   GL_STATE_DRAW_SKELETON  = 1<<6,
   GL_STATE_DRAW_WIREFRAME = 1<<7,
   GL_STATE_LIGHTING       = 1<<8,
   GL_STATE_OVERDRAW       = 1<<9,
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
   glhckFaceOrientation frontFace;
} __OpenGLstate;

typedef struct __OpenGLrender {
   struct __OpenGLstate state;
   glhckShader *shader[GL_SHADER_LAST];
   glhckHwBuffer *sharedUBO;
} __OpenGLrender;

/* typecast the glhck's render pointer where we allocate our context */
#define GLPOINTER() ((__OpenGLrender*)GLHCKR()->renderPointer)

/* check if we have certain draw state active */
#define GL_HAS_STATE(x) (GLPOINTER()->state.flags & x)

/* ---- Render API ---- */

/* \brief set time to renderer */
static void rTime(float time)
{
   CALL(2, "%f", time);
   if (GLPOINTER()->sharedUBO)
      glhckHwBufferFillUniform(GLPOINTER()->sharedUBO, "GlhckTime", sizeof(float), &time);
}

/* \brief set projection matrix */
static void rSetProjection(const kmMat4 *m)
{
   CALL(2, "%p", m);
   if (GLPOINTER()->sharedUBO)
      glhckHwBufferFillUniform(GLPOINTER()->sharedUBO, "GlhckProjection", sizeof(kmMat4), &m[0]);
}

/* \brief set view matrix */
static void rSetView(const kmMat4 *m)
{
   CALL(2, "%p", m);
   if (GLPOINTER()->sharedUBO)
      glhckHwBufferFillUniform(GLPOINTER()->sharedUBO, "GlhckView", sizeof(kmMat4), &m[0]);
}

/* \brief set orthographic projection for 2D drawing */
static void rSetOrthographic(const kmMat4 *m)
{
   CALL(2, "%p", m);
   if (GLPOINTER()->sharedUBO)
      glhckHwBufferFillUniform(GLPOINTER()->sharedUBO, "GlhckOrthographic", sizeof(kmMat4), &m[0]);
}

/* \brief resize viewport */
static void rViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
   assert(width > 0 && height > 0);
   GL_CALL(glViewport(x, y, width, height));
}

/* \brief link program from 2 compiled shader objects */
static GLuint rProgramLink(GLuint vertexShader, GLuint fragmentShader)
{
   GLuint obj = 0;
   GLchar *log = NULL;
   GLsizei logSize;
   GLint status;
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
      log = _glhckMalloc(logSize);
      GL_CALL(glGetProgramInfoLog(obj, logSize, NULL, log));
      printf("-- %u,%u --\n%s^^ %u,%u ^^\n",
            vertexShader, fragmentShader, log, vertexShader, fragmentShader);
      if (_glhckStrupstr(log, "error")) goto fail;
      _glhckFree(log);
   }

   /* linking failed */
   GL_CALL(glGetProgramiv(obj, GL_LINK_STATUS, &status));
   if (status == GL_FALSE) goto fail;

   RET(0, "%u", obj);
   return obj;

fail:
   IFDO(_glhckFree, log);
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
   if (!(obj = glCreateShader(glhckShaderTypeToGL[type])))
      goto fail;

   /* feed the shader the contents */
   GL_CALL(glShaderSource(obj, 1, &contents, NULL));
   GL_CALL(glCompileShader(obj));

   /* dump the log incase of error */
   GL_CALL(glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &logSize));
   if (logSize > 1) {
      log = _glhckMalloc(logSize);
      GL_CALL(glGetShaderInfoLog(obj, logSize, NULL, log));
      printf("-- %s --\n%s^^ %s ^^\n", effectKey, log, effectKey);
      if (_glhckStrupstr(log, "error")) goto fail;
      _glhckFree(log);
   }

   RET(0, "%u", obj);
   return obj;

fail:
   IFDO(_glhckFree, log);
   if (obj) GL_CALL(glDeleteShader(obj));
   RET(0, "%d", 0);
   return 0;
}

/* \brief set needed starte from object data */
static void rObjectState(const glhckObject *object)
{
   __GLHCKvertexType *type = GLHCKVT(object->geometry->vertexType);

   /* need vertices? */
   GLPOINTER()->state.attrib[GLHCK_ATTRIB_VERTEX] = type->memb[0];

   /* need normal? */
   GLPOINTER()->state.attrib[GLHCK_ATTRIB_NORMAL] = type->memb[1];

   /* need color? */
   GLPOINTER()->state.attrib[GLHCK_ATTRIB_COLOR] =
      ((object->flags & GLHCK_OBJECT_VERTEX_COLOR) && type->memb[3]);

   /* depth? */
   GLPOINTER()->state.flags |=
      (object->flags & GLHCK_OBJECT_DEPTH)?GL_STATE_DEPTH:0;

   /* cull? */
   GLPOINTER()->state.flags |=
      (object->flags & GLHCK_OBJECT_CULL)?GL_STATE_CULL:0;

   /* aabb? */
   GLPOINTER()->state.flags |=
      (object->flags & GLHCK_OBJECT_DRAW_AABB)?GL_STATE_DRAW_AABB:0;

   /* obb? */
   GLPOINTER()->state.flags |=
      (object->flags & GLHCK_OBJECT_DRAW_OBB)?GL_STATE_DRAW_OBB:0;

   /* skeleton? */
   GLPOINTER()->state.flags |=
      (object->flags & GLHCK_OBJECT_DRAW_SKELETON)?GL_STATE_DRAW_SKELETON:0;

   /* wire? */
   GLPOINTER()->state.flags |=
      (object->flags & GLHCK_OBJECT_DRAW_WIREFRAME)?GL_STATE_DRAW_WIREFRAME:0;
}

/* \brief set needed state from material data */
static void rMaterialState(const glhckMaterial *material)
{
   /* need texture? */
   GLPOINTER()->state.flags |= (material->texture?GL_STATE_TEXTURE:0);

   /* alpha? */
   GLPOINTER()->state.flags |=
      (material->blenda != GLHCK_ZERO || material->blendb != GLHCK_ZERO)?GL_STATE_BLEND:0;

   /* blending modes */
   GLPOINTER()->state.blenda = material->blenda;
   GLPOINTER()->state.blendb = material->blendb;

   /* lighting */
   GLPOINTER()->state.flags |=
      (material->flags & GLHCK_MATERIAL_LIGHTING && GLHCKRD()->light)?GL_STATE_LIGHTING:0;
}

/* \brief set needed state from pass data */
static void rPassState(void)
{
   /* front face */
   GLPOINTER()->state.frontFace = GLHCKRP()->frontFace;

   /* cull face */
   GLPOINTER()->state.cullFace = GLHCKRP()->cullFace;

   /* disabled pass bits override the above.
    * it means that we don't want something for this render pass. */
   if (!(GLHCKRP()->flags & GLHCK_PASS_DEPTH))
      GLPOINTER()->state.flags &= ~GL_STATE_DEPTH;
   if (!(GLHCKRP()->flags & GLHCK_PASS_CULL))
      GLPOINTER()->state.flags &= ~GL_STATE_CULL;
   if (!(GLHCKRP()->flags & GLHCK_PASS_BLEND))
      GLPOINTER()->state.flags &= ~GL_STATE_BLEND;
   if (!(GLHCKRP()->flags & GLHCK_PASS_TEXTURE))
      GLPOINTER()->state.flags &= ~GL_STATE_TEXTURE;
   if (!(GLHCKRP()->flags & GLHCK_PASS_DRAW_OBB))
      GLPOINTER()->state.flags &= ~GL_STATE_DRAW_OBB;
   if (!(GLHCKRP()->flags & GLHCK_PASS_DRAW_AABB))
      GLPOINTER()->state.flags &= ~GL_STATE_DRAW_AABB;
   if (!(GLHCKRP()->flags & GLHCK_PASS_DRAW_SKELETON))
      GLPOINTER()->state.flags &= ~GL_STATE_DRAW_SKELETON;
   if (!(GLHCKRP()->flags & GLHCK_PASS_DRAW_WIREFRAME))
      GLPOINTER()->state.flags &= ~GL_STATE_DRAW_WIREFRAME;
   if (!(GLHCKRP()->flags & GLHCK_PASS_LIGHTING))
      GLPOINTER()->state.flags &= ~GL_STATE_LIGHTING;

   /* pass blend override */
   if (GLHCKRP()->blenda != GLHCK_ZERO || GLHCKRP()->blendb != GLHCK_ZERO) {
      GLPOINTER()->state.flags |= GL_STATE_BLEND;
      GLPOINTER()->state.blenda = GLHCKRP()->blenda;
      GLPOINTER()->state.blendb = GLHCKRP()->blendb;
   }

   /* overdraw state */
   if (!(GLHCKRP()->flags & GLHCK_PASS_OVERDRAW)) {
      GLPOINTER()->state.flags &= ~GL_STATE_OVERDRAW;
   } else {
      GLPOINTER()->state.flags &= ~GL_STATE_TEXTURE;
      GLPOINTER()->state.flags &= ~GL_STATE_LIGHTING;
      GLPOINTER()->state.flags |= GL_STATE_OVERDRAW;
      GLPOINTER()->state.flags |= GL_STATE_BLEND;
      GLPOINTER()->state.blenda = GLHCK_ONE;
      GLPOINTER()->state.blendb = GLHCK_ONE;
   }

   GLPOINTER()->state.attrib[GLHCK_ATTRIB_TEXTURE] = (GLPOINTER()->state.flags & GL_STATE_TEXTURE);
}

/* \brief pass interleaved vertex data to OpenGL nicely. */
static void rGeometryPointer(const glhckGeometry *geometry)
{
   __GLHCKvertexType *type = GLHCKVT(geometry->vertexType);
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, type->memb[0],
            glhckDataTypeToGL[type->dataType[0]], type->normalized[0], type->size, geometry->vertices + type->offset[0]));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_NORMAL, type->memb[1],
            glhckDataTypeToGL[type->dataType[1]], type->normalized[1], type->size, geometry->vertices + type->offset[1]));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_TEXTURE, type->memb[2],
            glhckDataTypeToGL[type->dataType[2]], type->normalized[2], type->size, geometry->vertices + type->offset[2]));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_COLOR, type->memb[3],
            glhckDataTypeToGL[type->dataType[3]], type->normalized[3], type->size, geometry->vertices + type->offset[3]));
}

/* \brief render frustum */
static void rFrustumRender(glhckFrustum *frustum)
{
   GLuint i = 0;
   kmVec3 *corners = frustum->corners;
   const GLfloat points[] = {
                      corners[0].x, corners[0].y, corners[0].z,
                      corners[1].x, corners[1].y, corners[1].z,
                      corners[1].x, corners[1].y, corners[1].z,
                      corners[2].x, corners[2].y, corners[2].z,
                      corners[2].x, corners[2].y, corners[2].z,
                      corners[3].x, corners[3].y, corners[3].z,
                      corners[3].x, corners[3].y, corners[3].z,
                      corners[0].x, corners[0].y, corners[0].z,

                      corners[4].x, corners[4].y, corners[4].z,
                      corners[5].x, corners[5].y, corners[5].z,
                      corners[5].x, corners[5].y, corners[5].z,
                      corners[6].x, corners[6].y, corners[6].z,
                      corners[6].x, corners[6].y, corners[6].z,
                      corners[7].x, corners[7].y, corners[7].z,
                      corners[7].x, corners[7].y, corners[7].z,
                      corners[4].x, corners[4].y, corners[4].z,

                      corners[0].x, corners[0].y, corners[0].z,
                      corners[4].x, corners[4].y, corners[4].z,
                      corners[1].x, corners[1].y, corners[1].z,
                      corners[5].x, corners[5].y, corners[5].z,
                      corners[2].x, corners[2].y, corners[2].z,
                      corners[6].x, corners[6].y, corners[6].z,
                      corners[3].x, corners[3].y, corners[3].z,
                      corners[7].x, corners[7].y, corners[7].z  };

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && GLPOINTER()->state.attrib[i]) {
         GL_CALL(glDisableVertexAttribArray(i));
      }

   kmMat4 identity;
   kmMat4Identity(&identity);
   glhckShaderBind(GLPOINTER()->shader[GL_SHADER_COLOR]);
   glhckShaderUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&identity);
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1, &((GLfloat[]){255,0,0,255}));

   GL_CALL(glLineWidth(4));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glLineWidth(1));

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && GLPOINTER()->state.attrib[i]) {
         GL_CALL(glEnableVertexAttribArray(i));
      }
}


/* \brief render object's oriented bounding box */
static void rOBBRender(const glhckObject *object)
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
      if (i != GLHCK_ATTRIB_VERTEX && GLPOINTER()->state.attrib[i]) {
         GL_CALL(glDisableVertexAttribArray(i));
      }

   glhckShaderBind(GLPOINTER()->shader[GL_SHADER_COLOR]);
   glhckShaderUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&object->view.matrix);
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1, &((GLfloat[]){0,255,0,255}));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && GLPOINTER()->state.attrib[i]) {
         GL_CALL(glEnableVertexAttribArray(i));
      }
}

/* \brief render object's axis-aligned bounding box */
static void rAABBRender(const glhckObject *object)
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
      if (i != GLHCK_ATTRIB_VERTEX && GLPOINTER()->state.attrib[i]) {
         GL_CALL(glDisableVertexAttribArray(i));
      }

   kmMat4 identity;
   kmMat4Identity(&identity);
   glhckShaderBind(GLPOINTER()->shader[GL_SHADER_COLOR]);
   glhckShaderUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&identity);
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1, &((GLfloat[]){0,0,255,255}));
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));

   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i)
      if (i != GLHCK_ATTRIB_VERTEX && GLPOINTER()->state.attrib[i]) {
         GL_CALL(glEnableVertexAttribArray(i));
      }
}

/* \brief render object's bones */
static void rBonesRender(const glhckObject *object)
{
   if (!object->skinBones || !object->geometry)
      return;

   kmVec3 *points;
   if (!(points = _glhckMalloc(chckArrayCount(object->skinBones) * sizeof(kmVec3))))
      return;

   kmMat4 bias, scale, transform;
   kmMat4Translation(&bias, object->geometry->bias.x, object->geometry->bias.y, object->geometry->bias.z);
   kmMat4Scaling(&scale, object->geometry->scale.x, object->geometry->scale.y, object->geometry->scale.z);
   kmMat4Inverse(&bias, &bias);
   kmMat4Inverse(&scale, &scale);

   glhckSkinBone *skinBone;
   for (chckArrayIndex iter = 0; (skinBone = chckArrayIter(object->skinBones, &iter));) {
      if (skinBone->bone)
         continue;

      kmMat4Multiply(&transform, &bias, &skinBone->bone->transformedMatrix);
      kmMat4Multiply(&transform, &scale, &transform);
      points[iter - 1].x = transform.mat[12];
      points[iter - 1].y = transform.mat[13];
      points[iter - 1].z = transform.mat[14];
   }

   GL_CALL(glPointSize(5.0f));
   GL_CALL(glDisable(GL_DEPTH_TEST));
   glhckShaderBind(GLPOINTER()->shader[GL_SHADER_COLOR]);
   glhckShaderUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&object->view.matrix);
   GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 3, GL_FLOAT, 0, 0, &points[0]));
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1, &((GLfloat[]){255,0,0,255}));
   GL_CALL(glDrawArrays(GL_POINTS, 0, chckArrayCount(object->skinBones)));
   GL_CALL(glEnable(GL_DEPTH_TEST));

   _glhckFree(points);
}

/* helper for checking state changes */
#define GL_STATE_CHANGED(x) ((GLPOINTER()->state.flags & x) != (old.flags & x))

/* \brief begin object render */
static void rObjectStart(const glhckObject *object)
{
   GLuint i;
   __OpenGLstate old = GLPOINTER()->state;
   GLPOINTER()->state.flags = 0; /* reset this state */

   /* figure out states */
   rObjectState(object);
   if (object->material) rMaterialState(object->material);
   rPassState();

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

   /* check winding */
   if (GLPOINTER()->state.frontFace != old.frontFace) {
      glhFrontFace(GLPOINTER()->state.frontFace);
   }

   /* check culling */
   if (GL_STATE_CHANGED(GL_STATE_CULL)) {
      if (GL_HAS_STATE(GL_STATE_CULL)) {
         GL_CALL(glEnable(GL_CULL_FACE));
         glhCullFace(GLPOINTER()->state.cullFace);
      } else {
         GL_CALL(glDisable(GL_CULL_FACE));
      }
   } else if (GL_HAS_STATE(GL_STATE_CULL)) {
      if (GLPOINTER()->state.cullFace != old.cullFace) {
         glhCullFace(GLPOINTER()->state.cullFace);
      }
   }

#if GLHCK_TRISTRIP
   /* disable culling for strip geometry
    * FIXME: Fix the stripping to get rid of this */
   if (GL_HAS_STATE(GL_STATE_CULL) &&
       object->geometry->type == GLHCK_TRIANGLE_STRIP) {
      GL_CALL(glDisable(GL_CULL_FACE));
   }
#endif

   /* check alpha */
   if (GL_STATE_CHANGED(GL_STATE_BLEND)) {
      if (GL_HAS_STATE(GL_STATE_BLEND)) {
         GL_CALL(glEnable(GL_BLEND));
         GL_CALL(glhBlendFunc(GLPOINTER()->state.blenda, GLPOINTER()->state.blendb));
      } else {
         GL_CALL(glDisable(GL_BLEND));
      }
   } else if (GL_HAS_STATE(GL_STATE_BLEND) &&
         (GLPOINTER()->state.blenda != old.blenda ||
          GLPOINTER()->state.blendb != old.blendb)) {
      GL_CALL(glhBlendFunc(GLPOINTER()->state.blenda, GLPOINTER()->state.blendb));
   }

   /* check attribs */
   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i) {
      if (GLPOINTER()->state.attrib[i] != old.attrib[i]) {
         if (GLPOINTER()->state.attrib[i]) {
            GL_CALL(glEnableVertexAttribArray(i));
         } else {
            GL_CALL(glDisableVertexAttribArray(i));
         }
      }
   }

   /* bind texture if used */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      glhckTextureBind(object->material->texture);
   } else if (GL_STATE_CHANGED(GL_STATE_TEXTURE)) {
      glhckTextureUnbind(GLHCK_TEXTURE_2D);
   }

   /* pick shader */
   if (GLHCKRP()->shader) {
      glhckShaderBind(GLHCKRP()->shader);
   } else if (object->material && object->material->shader) {
      glhckShaderBind(object->material->shader);
   } else {
      if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
         if (GL_HAS_STATE(GL_STATE_LIGHTING)) {
            glhckShaderBind(GLPOINTER()->shader[GL_SHADER_BASE_LIGHTING]);
         } else {
            glhckShaderBind(GLPOINTER()->shader[GL_SHADER_BASE]);
         }
      } else {
         if (GL_HAS_STATE(GL_STATE_LIGHTING)) {
            glhckShaderBind(GLPOINTER()->shader[GL_SHADER_COLOR_LIGHTING]);
         } else {
            glhckShaderBind(GLPOINTER()->shader[GL_SHADER_COLOR]);
         }
      }
   }

   glhckColorb diffuse = {255,255,255,255};
   if (object->material) memcpy(&diffuse, &object->material->diffuse, sizeof(glhckColorb));
   if (GL_HAS_STATE(GL_STATE_OVERDRAW)) memcpy(&diffuse, &overdrawColor, sizeof(glhckColorb));
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1,
         &((GLfloat[]){diffuse.r, diffuse.g, diffuse.b, diffuse.a}));

   glhckColorb ambient = {255,255,255,255};
   if (object->material) memcpy(&ambient, &object->material->ambient, sizeof(glhckColorb));
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Ambient", 1,
         &((GLfloat[]){ambient.r, ambient.g, ambient.b, ambient.a}));

   glhckColorb specular = {255,255,255,255};
   if (object->material) memcpy(&specular, &object->material->specular, sizeof(glhckColorb));
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Specular", 1,
         &((GLfloat[]){specular.r, specular.g, specular.b, specular.a}));

   GLfloat shininess = 0.0f;
   if (object->material) shininess = object->material->shininess;
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Shininess", 1, &shininess);

   GLfloat rotation = 0.0f;
   if (object->material) rotation = object->material->textureRotation;
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.TextureRotation", 1, &rotation);

   kmVec2 offset = {0,0};
   if (object->material) memcpy(&offset, &object->material->textureOffset, sizeof(kmVec2));
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.TextureOffset", 1, &offset);

   kmVec2 scale = {1,1};
   if (object->material) {
      memcpy(&scale, &object->material->textureScale, sizeof(kmVec2));
      if (object->material->texture) {
         scale.x *= object->material->texture->internalScale.x;
         scale.y *= object->material->texture->internalScale.y;
      }
   }
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.TextureScale", 1, &scale);

   glhckShaderUniform(GLHCKRD()->shader, "GlhckModel", 1, (GLfloat*)&object->view.matrix);
}

/* \brief end object render */
static void rObjectEnd(const glhckObject *object)
{
   glhckGeometryType type = object->geometry->type;

   /* switch to wireframe if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_WIREFRAME)) {
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

   /* draw bones, if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_SKELETON))
      rBonesRender(object);

   /* enable the culling back
    * NOTE: this is a hack */
   if (GL_HAS_STATE(GL_STATE_CULL) &&
       object->geometry->type == GLHCK_TRIANGLE_STRIP) {
      GL_CALL(glEnable(GL_CULL_FACE));
   }
}

#undef GL_STATE_CHANGED

/* \brief render single 3d object */
static void rObjectRender(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object->geometry->vertexCount != 0 && object->geometry->vertices);
   rObjectStart(object);
   rGeometryPointer(object->geometry);
   rObjectEnd(object);
}

/* \brief render text */
static void rTextRender(const glhckText *text)
{
   CALL(2, "%p", text);

   if (!GL_HAS_STATE(GL_STATE_OVERDRAW)) {
      if (!GL_HAS_STATE(GL_STATE_BLEND)) {
         GLPOINTER()->state.flags |= GL_STATE_BLEND;
         GLPOINTER()->state.blenda = GLHCK_SRC_ALPHA;
         GLPOINTER()->state.blendb = GLHCK_ONE_MINUS_SRC_ALPHA;
         GL_CALL(glEnable(GL_BLEND));
         GL_CALL(glhBlendFunc(GLPOINTER()->state.blenda, GLPOINTER()->state.blendb));
      } else if (GLPOINTER()->state.blenda != GLHCK_SRC_ALPHA ||
            GLPOINTER()->state.blendb != GLHCK_ONE_MINUS_SRC_ALPHA) {
         GLPOINTER()->state.blenda = GLHCK_SRC_ALPHA;
         GLPOINTER()->state.blendb = GLHCK_ONE_MINUS_SRC_ALPHA;
         GL_CALL(glhBlendFunc(GLPOINTER()->state.blenda, GLPOINTER()->state.blendb));
      }
   }

   if (GLPOINTER()->state.frontFace != GLHCK_CCW) {
      glhFrontFace(GLHCK_CCW);
   }

   if (!GL_HAS_STATE(GL_STATE_OVERDRAW)) {
      if (!GL_HAS_STATE(GL_STATE_TEXTURE)) {
         GLPOINTER()->state.flags |= GL_STATE_TEXTURE;
         GLPOINTER()->state.attrib[GLHCK_ATTRIB_TEXTURE] = 1;
         GL_CALL(glEnableVertexAttribArray(GLHCK_ATTRIB_TEXTURE));
      }
   } else {
      glhckTextureUnbind(GLHCK_TEXTURE_2D);
   }

   if (!GLPOINTER()->state.attrib[GLHCK_ATTRIB_VERTEX]) {
      GLPOINTER()->state.attrib[GLHCK_ATTRIB_VERTEX] = 1;
      GL_CALL(glEnableVertexAttribArray(GLHCK_ATTRIB_VERTEX));
   }

   if (GLPOINTER()->state.attrib[GLHCK_ATTRIB_NORMAL]) {
      GLPOINTER()->state.attrib[GLHCK_ATTRIB_NORMAL] = 0;
      GL_CALL(glDisableVertexAttribArray(GLHCK_ATTRIB_NORMAL));
   }

   if (GLPOINTER()->state.attrib[GLHCK_ATTRIB_COLOR]) {
      GLPOINTER()->state.attrib[GLHCK_ATTRIB_COLOR] = 0;
      GL_CALL(glDisableVertexAttribArray(GLHCK_ATTRIB_COLOR));
   }

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glDisable(GL_DEPTH_TEST));
   }

   if (text->shader) glhckShaderBind(text->shader);
   else glhckShaderBind(GLPOINTER()->shader[GL_SHADER_TEXT]);

   glhckColorb diffuse = text->color;
   if (GL_HAS_STATE(GL_STATE_OVERDRAW)) memcpy(&diffuse, &overdrawColor, sizeof(glhckColorb));
   glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.Diffuse", 1,
         &((GLfloat[]){diffuse.r, diffuse.g, diffuse.b, diffuse.a}));

   __GLHCKtextTexture *t;
   for (chckPoolIndex iter = 0; (t = chckPoolIter(text->textures, &iter));) {
      if (!t->geometry.vertexCount)
         continue;

      if (GL_HAS_STATE(GL_STATE_TEXTURE))
         glhckTextureBind(t->texture);

      glhckShaderUniform(GLHCKRD()->shader, "GlhckMaterial.TextureScale", 1, &t->texture->internalScale);

      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_VERTEX, 2, (GLHCK_TEXT_FLOAT_PRECISION ? GL_FLOAT : GL_SHORT), 0,
               (GLHCK_TEXT_FLOAT_PRECISION ? sizeof(glhckVertexData2f) : sizeof(glhckVertexData2s)),
               &t->geometry.vertexData[0].vertex));

      GL_CALL(glVertexAttribPointer(GLHCK_ATTRIB_TEXTURE, 2, (GLHCK_TEXT_FLOAT_PRECISION ? GL_FLOAT : GL_SHORT),
               (GLHCK_TEXT_FLOAT_PRECISION ? 0 : 1),
               (GLHCK_TEXT_FLOAT_PRECISION ? sizeof(glhckVertexData2f) : sizeof(glhckVertexData2s)),
               &t->geometry.vertexData[0].coord));

      GL_CALL(glDrawArrays((GLHCK_TRISTRIP ? GL_TRIANGLE_STRIP : GL_TRIANGLES), 0, t->geometry.vertexCount));
   }

   if (GLPOINTER()->state.frontFace != GLHCK_CCW)
      glhFrontFace(GLPOINTER()->state.frontFace);

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glEnable(GL_DEPTH_TEST));
   }
}

static int constructor(void)
{
   int i;
   TRACE(0);

   /* init render's context */
   if (!(GLHCKR()->renderPointer = _glhckCalloc(1, sizeof(__OpenGLrender))))
      goto fail;

   /* setup OpenGL debug output */
   glhSetupDebugOutput();

   /* init shader wrangler */
   if (!glswInit())
      goto fail;

   /* default path &&  extension */
   glswSetPath("shaders/", ".glsl");

   /* default GLhck shader directivies */
   glswAddDirectiveToken("GLhck", "#version 140");

   /* NOTE: uniform constructors crash on fglrx, don't use them.
    * At least until the cause is sorted out. */

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
         "  float TextureRotation;"
         "  vec2 TextureOffset;"
         "  vec2 TextureScale;"
         "};"
         "uniform glhckMaterial GlhckMaterial;"
         "uniform sampler2D GlhckTexture0;");

   /* lighting functions */
   glswAddDirectiveToken("Lighting",
         "vec3 glhckLighting(vec3 Diffuse) {"
         "  vec3 Color    = Diffuse * GlhckMaterial.Ambient/255.0;"
         "  vec3 LDiffuse = GlhckLight.Diffuse/255.0;"
         "  vec3 LAtten   = GlhckLight.Atten*0.001;"
         "  vec2 LCutout  = GlhckLight.Cutout;"
         "  vec3 Specular = Diffuse * GlhckMaterial.Specular/255.0;"
         "  vec3 Normal   = normalize(GlhckFNormalWorld);"
         "  vec3 LightVec = normalize(GlhckLight.Position - GlhckFVertexWorld.xyz);"
         "  vec3 LightDir = normalize(GlhckLight.Target   - GlhckFVertexWorld.xyz);"
         ""
         "  float L = clamp(dot(Normal, LightVec), 0.0, 1.0);"
         "  float Spotlight = max(-dot(LightVec, LightDir), 0.0);"
         "  float SpotlightFade = clamp((LCutout.x - Spotlight) / (LCutout.x - LCutout.y), 0.0, 1.0);"
         "  Spotlight = clamp(pow(Spotlight * SpotlightFade, 1.0), GlhckLight.PointLight, 1.0);"
         "  vec3  R = -normalize(reflect(LightVec, Normal));"
         "  float S = pow(max(dot(R, LightVec), 0.0), GlhckMaterial.Shininess);"
         "  float D = distance(GlhckFVertexWorld.xyz, GlhckLight.Position);"
         "  float A = 1.0 / (LAtten.x + (LAtten.y * D) + (LAtten.z * D * D));"
         "  Color += (Diffuse * L + Specular * S) * LDiffuse * A * Spotlight;"
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

   /* save from some headache */
   GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT,1));

   /* create shared uniform buffer objects */
   if (!(GLPOINTER()->sharedUBO = glhckHwBufferNew()))
      goto fail;

   /* compile internal shaders */
   GLPOINTER()->shader[GL_SHADER_BASE] = glhckShaderNew(NULL, NULL, _glhckBaseShader);
   GLPOINTER()->shader[GL_SHADER_COLOR] = glhckShaderNew(NULL, ".GLhck.Color.Fragment", _glhckBaseShader);
   GLPOINTER()->shader[GL_SHADER_BASE_LIGHTING] = glhckShaderNew(NULL, ".GLhck.Base.Lighting.Fragment", _glhckBaseShader);
   GLPOINTER()->shader[GL_SHADER_COLOR_LIGHTING] = glhckShaderNew(NULL, ".GLhck.Color.Lighting.Fragment", _glhckBaseShader);
   GLPOINTER()->shader[GL_SHADER_TEXT] = glhckShaderNew(".GLhck.Text.Vertex", ".GLhck.Text.Fragment", _glhckBaseShader);

   /* create UBO from shader */
   glhckHwBufferCreateUniformBufferFromShader(GLPOINTER()->sharedUBO,
         GLPOINTER()->shader[GL_SHADER_BASE], "GlhckUBO", GLHCK_BUFFER_DYNAMIC_DRAW);

   /* append UBO information from other shaders */
   for (i = 1; i != GL_SHADER_LAST; ++i) {
      if (!GLPOINTER()->shader[i]) goto fail;
      glhckHwBufferAppendInformationFromShader(GLPOINTER()->sharedUBO, GLPOINTER()->shader[i]);
   }

   DEBUG(GLHCK_DBG_CRAP, "GLHCK UBO SIZE: %d", GLPOINTER()->sharedUBO->size);
   glhckHwBufferBindRange(GLPOINTER()->sharedUBO, 0, 0, GLPOINTER()->sharedUBO->size);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

static void destructor(void)
{
   int i;
   TRACE(0);

   /* free shaders */
   if (GLHCKW()->shaders && chckArrayCount(GLHCKW()->shaders) > 0) {
      for (i = 0; i != GL_SHADER_LAST; ++i) {
         NULLDO(glhckShaderFree, GLPOINTER()->shader[i]);
      }
   }

   /* free hw buffers */
   if (GLHCKW()->hwbuffers && chckArrayCount(GLHCKW()->hwbuffers) > 0) {
      NULLDO(glhckHwBufferFree, GLPOINTER()->sharedUBO);
   }

   /* shutdown shader wrangler */
   glswShutdown();

   /* free our render structure */
   IFDO(_glhckFree, GLHCKR()->renderPointer);

   /* this tells library that we are no longer alive. */
   GLHCK_RENDER_TERMINATE(RENDER_NAME);
}
#endif

static int constructor(struct glhckRendererInfo *info)
{
   CALL(0, "%p", info);

   // XXX: not implemented yet
   goto fail;

   if (!gladLoadGL())
      goto fail;

   glhRendererFeatures(&info->features);

   info->api = (struct glhckRendererAPI){
      .viewport = glViewport,
      // .setOrthographic = rSetOrthographic,
      // .setProjection = rSetProjection,
      // .setView = rSetView,
      // .time = rTime,
      .clearColor = glhClearColor,
      .clear = glhClear,
      .bufferGetPixels = glhBufferGetPixels,
      .textureGenerate = glhTextureGenerate,
      .textureDelete = glhTextureDelete,
      .textureBind = glhTextureBind,
      .textureActive = glhTextureActive,
      .textureFill = glhTextureFill,
      .textureImage = glhTextureImage,
      .textureParameter = glhTextureParameter,
      .textureGenerateMipmap = glhTextureMipmap,
      .renderbufferGenerate = glhRenderbufferGenerate,
      .renderbufferDelete = glhRenderbufferDelete,
      .renderbufferBind = glhRenderbufferBind,
      .renderbufferStorage = glhRenderbufferStorage,
      .framebufferGenerate = glhFramebufferGenerate,
      .framebufferDelete = glhFramebufferDelete,
      .framebufferBind = glhFramebufferBind,
      .framebufferTexture = glhFramebufferTexture,
      .framebufferRenderbuffer = glhFramebufferRenderbuffer,
      .hwBufferGenerate = glhHwBufferGenerate,
      .hwBufferDelete = glhHwBufferDelete,
      .hwBufferBind = glhHwBufferBind,
      .hwBufferBindBase = glhHwBufferBindBase,
      .hwBufferBindRange = glhHwBufferBindRange,
      .hwBufferCreate = glhHwBufferCreate,
      .hwBufferFill = glhHwBufferFill,
      .hwBufferMap = glhHwBufferMap,
      .hwBufferUnmap = glhHwBufferUnmap,
      .programBind = glhProgramBind,
      // .programLink = rProgramLink,
      .programDelete = glhProgramDelete,
      // .shaderCompile = rShaderCompile,
      .shaderDelete = glhShaderDelete,
      .programUniform = glhProgramUniform,
      .programUniformBufferPool = glhProgramUniformBufferPool,
      .programAttributePool = glhProgramAttributePool,
      .programUniformPool = glhProgramUniformPool,
      .programAttachUniformBuffer = glhProgramAttachUniformBuffer,
      // .shadersPath = glswSetPath,
      // .shadersDirectiveToken = glswAddDirectiveToken
   };

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckRendererRegister(struct glhckRendererInfo *info)
{
   CALL(0, "%p", info);

   info->constructor = constructor;
   // info->destructor = destructor;

   info->context = (glhckRendererContextInfo){
      .type = GLHCK_CONTEXT_OPENGL,
      .major = 3,
      .minor = 2,
      .coreProfile = 1,
      .forwardCompatible = 1,
   };

   RET(0, "%s", "glhck-renderer-opengl3");
   return "glhck-renderer-opengl3";
}

/* vim: set ts=8 sw=3 tw=0 :*/
