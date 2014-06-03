#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define GLHCK_ATTRIB_COUNT 4

#define OPENGL2
#include "include/glad/glad.h"
#include "renderers/helpers/opengl.h"
#include "renderers/renderer.h"

#if 0

static const glhckColorb overdrawColor = {25,25,25,255};

static const GLenum _glhckAttribName[] = {
   GL_VERTEX_ARRAY,
   GL_NORMAL_ARRAY,
   GL_TEXTURE_COORD_ARRAY,
   GL_COLOR_ARRAY,
};

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

/* global data */
typedef struct __OpenGLstate {
   GLchar attrib[GLHCK_ATTRIB_COUNT];
   GLuint flags;
   GLuint blenda, blendb;
   glhckCullFaceType cullFace;
   glhckFaceOrientation frontFace;
} __OpenGLstate;

typedef struct __OpenGLrender {
   struct __OpenGLstate state;
} __OpenGLrender;

/* typecast the glhck's render pointer where we allocate our context */
#define GLPOINTER() ((__OpenGLrender*)GLHCKR()->renderPointer)

/* check if we have certain draw state active */
#define GL_HAS_STATE(x) (GLPOINTER()->state.flags & x)

/* \brief load matrix */
static void rLoadMatrix(const kmMat4 *mat)
{
#if USE_DOUBLE_PRECISION
   GL_CALL(glLoadMatrixd((GLdouble*)mat));
#else
   GL_CALL(glLoadMatrixf((GLfloat*)mat));
#endif
}

/* \brief multiplies current matrix with given matrix */
static void rMultMatrix(const kmMat4 *mat)
{
#if USE_DOUBLE_PRECISION
   GL_CALL(glMultMatrixd((GLdouble*)mat));
#else
   GL_CALL(glMultMatrixf((GLfloat*)mat));
#endif
}

/* ---- Render API ---- */

/* \brief bind light */
static void rLightBind(glhckLight *light)
{
   /* stub in fixed pipeline */
   (void)light;
}

/* \brief we use this function instead in fixed pipeline in rObjectStart */
static void rLightSetup(glhckLight *light)
{
   glhckObject *object;

   if (!light)
      return;

   object = glhckLightGetObject(light);
   kmVec3 diffuse = { 255, 255, 255 }; // FIXME: Get real diffuse
   const kmVec3 *pos = glhckObjectGetPosition(object);

   /* position is affected by model view matrix */
   rLoadMatrix(&GLHCKRD()->view.view);
   GL_CALL(glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat[]){pos->x, pos->y, pos->z, glhckLightGetPointLightFactor(light)}));
   GL_CALL(glLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, (GLfloat*)&glhckLightGetAtten(light)->x));
   GL_CALL(glLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, (GLfloat*)&glhckLightGetAtten(light)->y));
   GL_CALL(glLightfv(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, (GLfloat*)&glhckLightGetAtten(light)->z));
   GL_CALL(glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, (GLfloat*)glhckObjectGetTarget(object)));
#if 0
   GL_CALL(glLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, (GLfloat*)&glhckLightGetCutout(light)->x));
   GL_CALL(glLightfv(GL_LIGHT0, GL_SPOT_EXPONENT, (GLfloat*)&glhckLightGetCutout(light)->y));
#endif
   GL_CALL(glLightfv(GL_LIGHT0, GL_DIFFUSE, ((GLfloat[]){
               (GLfloat)diffuse.x/255.0f,
               (GLfloat)diffuse.y/255.0f,
               (GLfloat)diffuse.z/255.0f,
               (GLfloat)1.0f})));
}

/* \brief set projection matrix */
static void rSetProjection(const kmMat4 *m)
{
   CALL(2, "%p", m);
   GL_CALL(glMatrixMode(GL_PROJECTION));
   rLoadMatrix(m);
}

/* \brief set view matrix */
static void rSetView(const kmMat4 *m)
{
   CALL(2, "%p", m);
   (void)m;
   /* do nothing, we multiply against model */
}

/* \brief set orthographic matrix */
static void rSetOrthographic(const kmMat4 *m)
{
   CALL(2, "%p", m);
   (void)m;
   /* do nothing, ... */
}

/* \brief resize viewport */
static void rViewport(int x, int y, int width, int height)
{
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
   assert(width > 0 && height > 0);
   GL_CALL(glViewport(x, y, width, height));
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
   GL_CALL(glVertexPointer(type->memb[0], glhckDataTypeToGL[type->dataType[0]], type->size, geometry->vertices + type->offset[0]));
   GL_CALL(glNormalPointer(glhckDataTypeToGL[type->dataType[1]], type->size, geometry->vertices + type->offset[1]));
   GL_CALL(glTexCoordPointer(type->memb[2], glhckDataTypeToGL[type->dataType[2]], type->size, geometry->vertices + type->offset[2]));
   GL_CALL(glColorPointer(type->memb[3], glhckDataTypeToGL[type->dataType[3]], type->size, geometry->vertices + type->offset[3]));
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

   /* disable stuff if enabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glDisable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (GLPOINTER()->state.attrib[i]) {
         GL_CALL(glDisableClientState(_glhckAttribName[i]));
      }

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   rLoadMatrix(&GLHCKRD()->view.view);

   GL_CALL(glLineWidth(4));
   GL_CALL(glColor4ub(255, 0, 0, 255));
   GL_CALL(glVertexPointer(3, GL_FLOAT, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glColor4ub(255, 255, 255, 255));
   GL_CALL(glLineWidth(1));

   /* re enable stuff we disabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glEnable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (GLPOINTER()->state.attrib[i]) {
         GL_CALL(glEnableClientState(_glhckAttribName[i]));
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

   /* disable stuff if enabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glDisable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (GLPOINTER()->state.attrib[i]) {
         GL_CALL(glDisableClientState(_glhckAttribName[i]));
      }

   GL_CALL(glColor4ub(0, 255, 0, 255));
   GL_CALL(glVertexPointer(3, GL_FLOAT, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glColor4ub(255, 255, 255, 255));

   /* re enable stuff we disabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glEnable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (GLPOINTER()->state.attrib[i]) {
         GL_CALL(glEnableClientState(_glhckAttribName[i]));
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

   /* disable stuff if enabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glDisable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (GLPOINTER()->state.attrib[i]) {
         GL_CALL(glDisableClientState(_glhckAttribName[i]));
      }

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   rLoadMatrix(&GLHCKRD()->view.view);

   GL_CALL(glColor4ub(0, 0, 255, 255));
   GL_CALL(glVertexPointer(3, GL_FLOAT, 0, &points[0]));
   GL_CALL(glDrawArrays(GL_LINES, 0, 24));
   GL_CALL(glColor4ub(255, 255, 255, 255));

   /* back to modelView matrix */
   rMultMatrix(&object->view.matrix);

   /* re enable stuff we disabled */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glEnable(GL_TEXTURE_2D));
   }
   for (i = 1; i != GLHCK_ATTRIB_COUNT; ++i)
      if (GLPOINTER()->state.attrib[i]) {
         GL_CALL(glEnableClientState(_glhckAttribName[i]));
      }
}

/* helper for checking state changes */
#define GL_STATE_CHANGED(x) (GLPOINTER()->state.flags & x) != (old.flags & x)

/* \brief begin object render */
static void rObjectStart(const glhckObject *object) {
   GLuint i;
   __OpenGLstate old = GLPOINTER()->state;
   GLPOINTER()->state.flags = 0; /* reset this state */

   /* upload material parameters */
   if (GL_HAS_STATE(GL_STATE_LIGHTING) && object->material) {
      GL_CALL(glMaterialfv(GL_FRONT, GL_AMBIENT, ((GLfloat[]){
               (GLfloat)object->material->ambient.r/255.0f,
               (GLfloat)object->material->ambient.g/255.0f,
               (GLfloat)object->material->ambient.b/255.0f,
               (GLfloat)object->material->ambient.a/255.0f})));

      GL_CALL(glMaterialfv(GL_FRONT, GL_DIFFUSE, ((GLfloat[]){
               (GLfloat)object->material->diffuse.r/255.0f,
               (GLfloat)object->material->diffuse.g/255.0f,
               (GLfloat)object->material->diffuse.b/255.0f,
               (GLfloat)object->material->diffuse.a/255.0f})));

      GL_CALL(glMaterialfv(GL_FRONT, GL_SPECULAR, ((GLfloat[]){
               (GLfloat)object->material->specular.r/255.0f,
               (GLfloat)object->material->specular.g/255.0f,
               (GLfloat)object->material->specular.b/255.0f,
               (GLfloat)object->material->specular.a/255.0f})));

      GL_CALL(glMaterialf(GL_FRONT, GL_SHININESS, object->material->shininess));
   }

   /* set states and draw */
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

   /* check texture */
   if (GL_STATE_CHANGED(GL_STATE_TEXTURE)) {
      if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
         GL_CALL(glEnable(GL_TEXTURE_2D));
      } else {
         GL_CALL(glDisable(GL_TEXTURE_2D));
      }
   }

   /* check lighting */
   if (GL_STATE_CHANGED(GL_STATE_LIGHTING)) {
      if (GL_HAS_STATE(GL_STATE_LIGHTING)) {
         GL_CALL(glEnable(GL_LIGHTING));
         GL_CALL(glEnable(GL_LIGHT0));
      } else {
         GL_CALL(glDisable(GL_LIGHT0));
         GL_CALL(glDisable(GL_LIGHTING));
      }
   }

   /* check attribs */
   for (i = 0; i != GLHCK_ATTRIB_COUNT; ++i) {
      if (GLPOINTER()->state.attrib[i] != old.attrib[i]) {
         if (GLPOINTER()->state.attrib[i]) {
            GL_CALL(glEnableClientState(_glhckAttribName[i]));
         } else {
            GL_CALL(glDisableClientState(_glhckAttribName[i]));
         }
      }
   }

   /* bind texture if used */
   if (GL_HAS_STATE(GL_STATE_TEXTURE)) {
      GL_CALL(glMatrixMode(GL_TEXTURE));
      GL_CALL(glLoadIdentity());

      GLfloat rotation = 0.0f;
      if (object->material) rotation = object->material->textureRotation;
      GL_CALL(glTranslatef(0.5f, 0.5f, 0.0f));
      GL_CALL(glRotatef(rotation, 0, 0, 1));
      GL_CALL(glTranslatef(-0.5f, -0.5f, 0.0f));

      kmVec2 offset = {0,0};
      if (object->material) memcpy(&offset, &object->material->textureOffset, sizeof(kmVec2));
      GL_CALL(glTranslatef(offset.x, offset.y, 1.0f));

      kmVec2 scale = {1,1};
      if (object->material) {
         memcpy(&scale, &object->material->textureScale, sizeof(kmVec2));
         if (object->material->texture) {
            scale.x *= object->material->texture->internalScale.x;
            scale.y *= object->material->texture->internalScale.y;
         }
      }

      /* set texture coords according to how geometry wants them */
      GL_CALL(glScalef((GLfloat)scale.x/object->geometry->textureRange, (GLfloat)scale.y/object->geometry->textureRange, 1.0f));

      glhckTextureBind(object->material->texture);
   } else if (GL_STATE_CHANGED(GL_STATE_TEXTURE)) {
      glhckTextureUnbind(GLHCK_TEXTURE_2D);
   }

   /* reset color */
   glhckColorb diffuse = {255,255,255,255};
   if (object->material) memcpy(&diffuse, &object->material->diffuse, sizeof(glhckColorb));
   if (GL_HAS_STATE(GL_STATE_OVERDRAW)) memcpy(&diffuse, &overdrawColor, sizeof(glhckColorb));
   GL_CALL(glColor4ub(diffuse.r, diffuse.g, diffuse.b, diffuse.a));

   /* load view matrix */
   GL_CALL(glMatrixMode(GL_MODELVIEW));
   rLightSetup(GLHCKRD()->light);
   rLoadMatrix(&GLHCKRD()->view.view);
   rMultMatrix(&object->view.matrix);
}

#undef GL_STATE_CHANGED

/* \brief end object render */
static void rObjectEnd(const glhckObject *object) {
   glhckGeometryType type = object->geometry->type;

   /* switch to wireframe if requested */
   if (GL_HAS_STATE(GL_STATE_DRAW_WIREFRAME)) {
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
}

/* \brief render single 3d object */
static void rObjectRender(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object->geometry->vertexCount && object->geometry->vertices);
   rObjectStart(object);
   rGeometryPointer(object->geometry);
   rObjectEnd(object);
}

/* \brief draw text */
static void rTextRender(const glhckText *text)
{
   CALL(2, "%p", text);

   /* set states */
   if (GL_HAS_STATE(GL_STATE_LIGHTING)) {
      GL_CALL(glDisable(GL_LIGHTING));
   }

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
         GL_CALL(glEnable(GL_TEXTURE_2D));
         GL_CALL(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
      }
   } else {
      glhckTextureUnbind(GLHCK_TEXTURE_2D);
   }

   if (!GLPOINTER()->state.attrib[GLHCK_ATTRIB_VERTEX]) {
      GLPOINTER()->state.attrib[GLHCK_ATTRIB_VERTEX] = 1;
      GL_CALL(glEnableClientState(GL_VERTEX_ARRAY));
   }

   if (GLPOINTER()->state.attrib[GLHCK_ATTRIB_NORMAL]) {
      GLPOINTER()->state.attrib[GLHCK_ATTRIB_NORMAL] = 0;
      GL_CALL(glDisableClientState(GL_NORMAL_ARRAY));
   }

   if (GLPOINTER()->state.attrib[GLHCK_ATTRIB_COLOR]) {
      GLPOINTER()->state.attrib[GLHCK_ATTRIB_COLOR] = 0;
      GL_CALL(glDisableClientState(GL_COLOR_ARRAY));
   }

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glDisable(GL_DEPTH_TEST));
   }

   /* set 2d projection */
   GL_CALL(glMatrixMode(GL_PROJECTION));
   rLoadMatrix(&GLHCKRD()->view.orthographic);

   GL_CALL(glMatrixMode(GL_TEXTURE));
   GL_CALL(glLoadIdentity());
   GL_CALL(glScalef((GLfloat)1.0f/text->textureRange, (GLfloat)1.0f/text->textureRange, 1.0f));

   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadIdentity());

   glhckColorb diffuse = text->color;
   if (GL_HAS_STATE(GL_STATE_OVERDRAW)) memcpy(&diffuse, &overdrawColor, sizeof(glhckColorb));
   GL_CALL(glColor4ub(diffuse.r, diffuse.b, diffuse.g, diffuse.a));

   __GLHCKtextTexture *t;
   GL_CALL(glMatrixMode(GL_TEXTURE));
   for (chckPoolIndex iter = 0; (t = chckPoolIter(text->textures, &iter));) {
      if (!t->geometry.vertexCount)
         continue;

      if (GL_HAS_STATE(GL_STATE_TEXTURE))
         glhckTextureBind(t->texture);

      GL_CALL(glLoadIdentity());
      GL_CALL(glScalef(t->texture->internalScale.x, t->texture->internalScale.y, 1.0f));
      GL_CALL(glVertexPointer(2, (GLHCK_TEXT_FLOAT_PRECISION ? GL_FLOAT : GL_SHORT),
            (GLHCK_TEXT_FLOAT_PRECISION ? sizeof(glhckVertexData2f) : sizeof(glhckVertexData2s)),
            &t->geometry.vertexData[0].vertex));
      GL_CALL(glTexCoordPointer(2, (GLHCK_TEXT_FLOAT_PRECISION ? GL_FLOAT : GL_SHORT),
            (GLHCK_TEXT_FLOAT_PRECISION ? sizeof(glhckVertexData2f) : sizeof(glhckVertexData2s)),
            &t->geometry.vertexData[0].coord));
      GL_CALL(glDrawArrays((GLHCK_TRISTRIP ? GL_TRIANGLE_STRIP : GL_TRIANGLES),
               0, t->geometry.vertexCount));
   }

   if (GLPOINTER()->state.frontFace != GLHCK_CCW) {
      glhFrontFace(GLPOINTER()->state.frontFace);
   }

   if (GL_HAS_STATE(GL_STATE_DEPTH)) {
      GL_CALL(glEnable(GL_DEPTH_TEST));
   }

   if (GL_HAS_STATE(GL_STATE_LIGHTING)) {
      GL_CALL(glEnable(GL_LIGHTING));
   }

   /* reset projection */
   GL_CALL(glMatrixMode(GL_PROJECTION));
   rLoadMatrix(&GLHCKRD()->view.projection);
}

/* ---- Initialization ---- */

/* \brief init renderers global state */
static int renderInit(void)
{
   TRACE(0);

   /* init render's context */
   if (!(GLHCKR()->renderPointer = _glhckCalloc(1, sizeof(__OpenGLrender))))
      goto fail;

   /* setup OpenGL debug output */
   glhSetupDebugOutput();

   /* save from some headache */
   GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT,1));

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief terminate renderer */
static void renderTerminate(void)
{
   TRACE(0);

   /* free our render structure */
   IFDO(_glhckFree, GLHCKR()->renderPointer);

   /* this tells library that we are no longer alive. */
   GLHCK_RENDER_TERMINATE(RENDER_NAME);
}

/* stub shader functions */


/* ---- Main ---- */

#endif

static void projectionMatrix(const kmMat4 *mat)
{
   GL_CALL(glMatrixMode(GL_PROJECTION));
   GL_CALL(glLoadMatrixf((GLfloat*)mat));
}

static void modelMatrix(const kmMat4 *mat)
{
   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadMatrixf((GLfloat*)mat));
}

static void textureMatrix(const kmMat4 *mat)
{
   GL_CALL(glMatrixMode(GL_TEXTURE));
   GL_CALL(glLoadMatrixf((GLfloat*)mat));
}

static void vertexPointer(GLint size, glhckDataType type, GLint stride, const GLvoid *pointer)
{
   GL_CALL(glVertexPointer(size, glhckDataTypeToGL[type], stride, pointer));
}

static void coordPointer(GLint size, glhckDataType type, GLint stride, const GLvoid *pointer)
{
   GL_CALL(glTexCoordPointer(size, glhckDataTypeToGL[type], stride, pointer));
}

static void drawArrays(glhckGeometryType type, GLint first, GLint count)
{
   GL_CALL(glDrawArrays(glhckGeometryTypeToGL[type], first, count));
}

static void drawElements(glhckGeometryType type, GLint count, glhckDataType dataType, const GLvoid *pointer)
{
   GL_CALL(glDrawElements(glhckGeometryTypeToGL[type], count, glhckDataTypeToGL[dataType], pointer));
}

static void blendFunc(glhckBlendingMode blenda, glhckBlendingMode blendb)
{
   GL_CALL(glBlendFunc(glhckBlendingModeToGL[blenda], glhckBlendingModeToGL[blendb]));
}

static void color(const glhckColor color)
{
   unsigned char fr = (color & 0xFF000000) >> 24;
   unsigned char fg = (color & 0x00FF0000) >> 16;
   unsigned char fb = (color & 0x0000FF00) >> 8;
   unsigned char fa = (color & 0x000000FF);
   GL_CALL(glColor4ub(fr, fg, fb, fa));
}

static int constructor(struct glhckRendererInfo *info)
{
   CALL(0, "%p", info);

   if (!gladLoadGL())
      goto fail;

   GL_CALL(glDisable(GL_LIGHTING));
   GL_CALL(glEnable(GL_TEXTURE_2D));
   GL_CALL(glEnable(GL_BLEND));
   GL_CALL(glEnableClientState(GL_VERTEX_ARRAY));
   GL_CALL(glEnableClientState(GL_TEXTURE_COORD_ARRAY));

   glhRendererFeatures(&info->features);

   info->api = (struct glhckRendererAPI){
      .viewport = glViewport,
      .projectionMatrix = projectionMatrix,
      .modelMatrix = modelMatrix,
      .textureMatrix = textureMatrix,
      .vertexPointer = vertexPointer,
      .coordPointer = coordPointer,
      .drawArrays = drawArrays,
      .drawElements = drawElements,
      // .setView = rSetView,
      // .time = rTime,
      .clearColor = glhClearColor,
      .clear = glhClear,
      .color = color,
      .blendFunc = blendFunc,
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
      .major = 2,
      .minor = 1,
      .coreProfile = 0,
      .forwardCompatible = 0,
   };

   RET(0, "%s", "glhck-renderer-opengl2");
   return "glhck-renderer-opengl2";
}

/* vim: set ts=8 sw=3 tw=0 :*/
