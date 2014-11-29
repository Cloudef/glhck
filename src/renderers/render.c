#include "render.h"
#include "renderer.h"
#include "types/geometry.h"
#include "types/object.h"
#include "types/view.h"

#include <glhck/glhck.h>
#include <assert.h> /* for assert */
#include <string.h>

#include "trace.h"
#include "system/tls.h"
#include "pool/pool.h"

#include "types/object.h"
#include "types/camera.h"
#include "types/texture.h"
#include "types/text/text.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER

/* \brief internal glhck flip matrix
 * every model matrix is multiplied with this when glhckRenderFlip(1) is used */
static const kmMat4 flipMatrix = {
   .mat = {
      1.0f,  0.0f, 0.0f, 0.0f,
      0.0f, -1.0f, 0.0f, 0.0f,
      0.0f,  0.0f, 1.0f, 0.0f,
      0.0f,  0.0f, 0.0f, 1.0f,
   }
};

static const kmMat4 identity = {
   .mat = {
      1.0f,  0.0f, 0.0f, 0.0f,
      0.0f,  1.0f, 0.0f, 0.0f,
      0.0f,  0.0f, 1.0f, 0.0f,
      0.0f,  0.0f, 0.0f, 1.0f,
   }
};

static _GLHCK_TLS chckIterPool *rstates;
static _GLHCK_TLS const struct glhckRendererAPI *rapi;

enum {
   STATE_DEPTH          = 1<<0,
   STATE_CULL           = 1<<1,
   STATE_BLEND          = 1<<2,
   STATE_TEXTURE        = 1<<3,
   STATE_OVERDRAW       = 1<<4,
   STATE_DRAW_AABB      = 1<<5,
   STATE_DRAW_OBB       = 1<<6,
   STATE_DRAW_SKELETON  = 1<<7,
   STATE_DRAW_WIREFRAME = 1<<8,
};

#if 0
static _GLHCK_TLS struct {
   unsigned int drawCount;
   unsigned int stateFlags;
} rdraw;
#endif

#define HAS_STATE(x) (rdraw.stateFlags & x)

static _GLHCK_TLS struct glhckRenderDisplay {
   int width, height;
} rdisplay;

static _GLHCK_TLS struct glhckRenderView {
   kmMat4 projection, view, orthographic;
   char flippedProjection;
} rview;

static _GLHCK_TLS struct glhckRenderPass {
   glhckRect viewport;
   glhckHandle shader;
   unsigned int blenda, blendb;
   unsigned int flags;
   glhckCullFaceType cullFace;
   glhckFaceOrientation frontFace;
   glhckColor clearColor;
} rpass;

struct glhckRenderState {
   struct glhckRenderView view;
   struct glhckRenderPass pass;
   struct glhckRenderDisplay display;
};

void _glhckRenderAPI(const struct glhckRendererAPI *api)
{
   rapi = api;
}

const struct glhckRendererAPI* _glhckRenderGetAPI(void)
{
   return rapi;
}

const kmMat4* _glhckRenderGetFlipMatrix(void)
{
   return &flipMatrix;
}

/* \brief builds and sends the default projection to renderer */
void _glhckRenderDefaultProjection(int width, int height)
{
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);
   glhckRenderProjection2D(width, height, -1000.0f, 1000.0f);
}

GLHCKAPI int glhckRenderGetWidth(void)
{
   return rdisplay.width;
}

GLHCKAPI int glhckRenderGetHeight(void)
{
   return rdisplay.height;
}

/* \brief resize render viewport internally */
GLHCKAPI void glhckRenderResize(int width, int height)
{
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);

   if (!rapi)
      return;

   /* nothing to resize */
   if (rdisplay.width == width && rdisplay.height == height)
      return;

   /* update all cameras */
   _glhckCameraWorldUpdate(width, height, rdisplay.width, rdisplay.height);

   /* update on library last, so functions know the old values */
   rdisplay.width  = width;
   rdisplay.height = height;
}

/* \brief set renderer's viewport */
GLHCKAPI void glhckRenderViewport(const glhckRect *viewport)
{
   CALL(1, "%p", viewport);
   assert(viewport->x >= 0 && viewport->y >= 0 && viewport->w > 0 && viewport->h > 0);

   if (!rapi)
      return;

   memcpy(&rpass.viewport, viewport, sizeof(glhckRect));
   rapi->viewport(viewport->x, viewport->y, viewport->w, viewport->h);

   kmMat4 ortho;
   kmMat4OrthographicProjection(&ortho, viewport->x, viewport->w, viewport->h, viewport->y, -1.0f, 1.0f);
   memcpy(&rview.orthographic, &ortho, sizeof(kmMat4));
}

/* \brief set renderer's viewport (int) */
GLHCKAPI void glhckRenderViewporti(int x, int y, int width, int height)
{
   const glhckRect viewport = { x, y, width, height };
   glhckRenderViewport(&viewport);
}

/* \brief push current render state to stack */
GLHCKAPI void glhckRenderStatePush(void)
{
   TRACE(2);

   if (!rstates && !(rstates = chckIterPoolNew(32, 1, sizeof(struct glhckRenderState))))
      return;

   struct glhckRenderState *state;
   if (!(state = chckIterPoolAdd(rstates, NULL, NULL)))
      return;

   memcpy(&state->pass, &rpass, sizeof(struct glhckRenderPass));
   memcpy(&state->view, &rview, sizeof(struct glhckRenderView));
   memcpy(&state->display, &rdisplay, sizeof(struct glhckRenderDisplay));
}

/* \brief push 2D drawing state */
GLHCKAPI void glhckRenderStatePush2D(int width, int height, kmScalar near, kmScalar far)
{
   TRACE(2);
   glhckRenderStatePush();
   glhckRenderPass(glhckRenderGetPass() & ~GLHCK_PASS_DEPTH);
   glhckRenderProjection2D(width, height, near, far);
}

/* \brief pop render state from stack */
GLHCKAPI void glhckRenderStatePop(void)
{
   struct glhckRenderState *state;
   if (!(state = chckIterPoolGetLast(rstates)))
      return;

   glhckRenderResize(state->display.width, state->display.height);
   memcpy(&rpass, &state->pass, sizeof(struct glhckRenderPass));
   glhckRenderClearColor(state->pass.clearColor);
   glhckRenderViewport(&state->pass.viewport);

   glhckRenderProjection(&state->view.projection);
   glhckRenderView(&state->view.view);
   memcpy(&rview.orthographic, &state->view.orthographic, sizeof(kmMat4));
   glhckRenderFlip(state->view.flippedProjection);

   chckIterPoolRemove(rstates, chckIterPoolCount(rstates) - 1);
}

/* \brief default render pass flags */
GLHCKAPI unsigned int glhckRenderPassDefaults(void)
{
   return (GLHCK_PASS_DEPTH          |
           GLHCK_PASS_CULL           |
           GLHCK_PASS_BLEND          |
           GLHCK_PASS_TEXTURE        |
           GLHCK_PASS_DRAW_OBB       |
           GLHCK_PASS_DRAW_AABB      |
           GLHCK_PASS_DRAW_SKELETON  |
           GLHCK_PASS_DRAW_WIREFRAME |
           GLHCK_PASS_LIGHTING);
}

/* \brief set render pass flags */
GLHCKAPI void glhckRenderPass(unsigned int flags)
{
   CALL(2, "%u", flags);
   rpass.flags = flags;
}

/* \brief get current render pass flags */
GLHCKAPI unsigned int glhckRenderGetPass(void)
{
   TRACE(2);
   RET(2, "%u", rpass.flags);
   return rpass.flags;
}

/* \brief give current program time to glhck */
GLHCKAPI void glhckRenderTime(float time)
{
   CALL(2, "%f", time);

   if (!rapi)
      return;

   rapi->time(time);
}

/* \brief set clear color to render */
GLHCKAPI void glhckRenderClearColor(const glhckColor color)
{
   CALL(2, "%u", color);

   if (!rapi)
      return;

   rpass.clearColor = color;
   rapi->clearColor(color);
}

/* \brief set clear color to render */
GLHCKAPI void glhckRenderClearColoru(const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a)
{
   glhckRenderClearColor((a | (b << 8) | (g << 16) | (r << 24)));
}

/* \brief return current clear color */
GLHCKAPI glhckColor glhckRenderGetClearColor(void)
{
   TRACE(2);
   RET(2, "%u", rpass.clearColor);
   return rpass.clearColor;
}

/* \brief clear scene */
GLHCKAPI void glhckRenderClear(unsigned int bufferBits)
{
   CALL(2, "%u", bufferBits);

   if (!rapi)
      return;

#if EMSCRIPTEN
   /* when there is no framebuffers bound assume we are in
    * main loop don't do clear since webgl does clear itself.
    * XXX: document this behaviour */
   int i;
   for (i = 0; i < GLHCK_FRAMEBUFFER_TYPE_LAST && !GLHCKRD()->framebuffer[i]; ++i);
   if (i == GLHCK_FRAMEBUFFER_TYPE_LAST) return;
#endif

   rapi->clear(bufferBits);
}

/* \brief set render pass front face orientation */
GLHCKAPI void glhckRenderFrontFace(glhckFaceOrientation orientation)
{
   CALL(2, "%d", orientation);
   rpass.frontFace = orientation;
}

/* \brief return current render pass front face */
GLHCKAPI glhckFaceOrientation glhckRenderGetFrontFace(void)
{
   TRACE(2);
   RET(2, "%u", rpass.frontFace);
   return rpass.frontFace;
}

/* \brief set render pass cull face side */
GLHCKAPI void glhckRenderCullFace(glhckCullFaceType face)
{
   CALL(2, "%d", face);
   rpass.cullFace = face;
}

/* \brief return current render pass cull face */
GLHCKAPI glhckCullFaceType glhckRenderGetCullFace(void)
{
   TRACE(2);
   RET(2, "%u", rpass.cullFace);
   return rpass.cullFace;
}

/* \brief set render pass blend func that overrides object specific */
GLHCKAPI void glhckRenderBlendFunc(glhckBlendingMode blenda, glhckBlendingMode blendb)
{
   CALL(2, "%d, %d", blenda, blendb);
   rpass.blenda = blenda;
   rpass.blendb = blendb;
}

/* \brief get current blend func for render pass */
GLHCKAPI void glhckRenderGetBlendFunc(glhckBlendingMode *blenda, glhckBlendingMode *blendb)
{
   CALL(2, "%p, %p", blenda, blendb);
   if (blenda) *blenda = rpass.blenda;
   if (blendb) *blendb = rpass.blendb;
}

/* \brief set shader for render pass */
GLHCKAPI void glhckRenderPassShader(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));

#if 0
   if (rpass.shader)
      glhckShaderFree(rpass.shader);
#endif

   if (handle)
      glhckHandleRef(handle);

   rpass.shader = handle;
}

/* \brief get current shader for render pass */
GLHCKAPI glhckHandle glhckRenderPassGetShader(void)
{
   TRACE(2);
   RET(2, "%s", glhckHandleRepr(rpass.shader));
   return rpass.shader;
}

/* \brief set vertically flipped rendering mode */
GLHCKAPI void glhckRenderFlip(int flip)
{
   CALL(2, "%d", flip);
   rview.flippedProjection = flip;
}

/* \brief get current flipping */
GLHCKAPI int glhckRenderGetFlip(void)
{
   TRACE(2);
   return rview.flippedProjection;
}

/* \brief set projection matrix */
GLHCKAPI void glhckRenderProjection(const kmMat4 *mat)
{
   CALL(2, "%p", mat);

   if (!rapi)
      return;

   memcpy(&rview.projection, mat, sizeof(kmMat4));
}

/* \brief set projection matrix, and identity view matrix */
GLHCKAPI void glhckRenderProjectionOnly(const kmMat4 *mat)
{
   CALL(2, "%p", mat);

   glhckRenderProjection(mat);
   glhckRenderView(&identity);
}

/* \brief set 2D projection matrix, and identity view matrx
 * 2D projection matrix sets screen coordinates where (0,0) == TOP LEFT (W,H) == BOTTOM RIGHT
 *
 *  TEXT    WORLD/3D
 *   -y        y
 * -x | x   -x | x
 *    y       -y
 *
 */
GLHCKAPI void glhckRenderProjection2D(int width, int height, kmScalar near, kmScalar far)
{
   CALL(2, "%d, %d, %f, %f", width, height, near, far);
   assert(width > 0 && height > 0);

   kmMat4 ortho;
   kmMat4OrthographicProjection(&ortho, 0, (kmScalar)width, (kmScalar)height, 0, near, far);
   glhckRenderProjectionOnly(&ortho);
   glhckRenderFlip(1);
}

/* \brief return projection matrix */
GLHCKAPI const kmMat4* glhckRenderGetProjection(void)
{
   TRACE(2);
   RET(2, "%p", &rview.projection);
   return &rview.projection;
}

/* \brief set view matrix */
GLHCKAPI void glhckRenderView(const kmMat4 *mat)
{
   CALL(2, "%p", mat);

   if (!rapi)
      return;

   memcpy(&rview.view, mat, sizeof(kmMat4));
}

/* \brief return view matrix */
GLHCKAPI const kmMat4* glhckRenderGetView(void)
{
   TRACE(2);
   RET(2, "%p", &rview.view);
   return &rview.view;
}

GLHCKAPI void glhckRenderObjectMany(const glhckHandle *handles, const size_t memb)
{
   _glhckObjectUpdateMatrixMany(handles, memb);

   rapi->blendFunc(GLHCK_ONE, GLHCK_ZERO);
   rapi->projectionMatrix(&rview.projection);

   for (size_t i = 0; i < memb; ++i) {
      const glhckHandle geometry = glhckObjectGetGeometry(handles[i]);
      const glhckGeometry *data = glhckGeometryGetStruct(geometry);

      const glhckHandle material = glhckObjectGetMaterial(handles[i]);
      if (material) {
         const glhckHandle texture = glhckMaterialGetTexture(material);
         if (texture) {
            glhckTextureBind(texture);
            rapi->blendFunc(GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);

            kmMat4 textureMatrix;
            const kmVec3 *internal = _glhckTextureGetInternalScale(texture);
            kmMat4Scaling(&textureMatrix, internal->x / (kmScalar)data->textureRange, internal->y / (kmScalar)data->textureRange, 1.0f);
            rapi->textureMatrix(&textureMatrix);
         }

         rapi->color(glhckMaterialGetDiffuse(material));
      }

      if (!material || !glhckMaterialGetTexture(material)) {
         glhckTextureUnbind(GLHCK_TEXTURE_2D);
      }

      const glhckHandle view = glhckObjectGetView(handles[i]);

      kmMat4 mv;
      if (rview.flippedProjection) {
         kmMat4 flipped;
         kmMat4Multiply(&flipped, glhckViewGetMatrix(view), &flipMatrix);
         kmMat4Multiply(&mv, &rview.view, &flipped);
      } else {
         kmMat4Multiply(&mv, &rview.view, glhckViewGetMatrix(view));
      }
      rapi->modelMatrix(&mv);

      const struct glhckVertexType *vt = _glhckGeometryGetVertexType(data->vertexType);
      rapi->vertexPointer(vt->memb[0], vt->dataType[0], vt->size, data->vertices + vt->offset[0]);
      // rapi->normalPointer(vt->dataType[1], vt->size, data->vertices + vt->offset[1]);
      rapi->coordPointer(vt->memb[2], vt->dataType[2], vt->size, data->vertices + vt->offset[2]);
      // rapi->colorPointer(vt->memb[3], vt->dataType[3], vt->size, data->vertices + vt->offset[3]);

      rapi->depthTest(glhckObjectGetDepth(handles[i]));

      if (data->indices) {
         const struct glhckIndexType *it = _glhckGeometryGetIndexType(data->indexType);
         rapi->drawElements(data->type, data->indexCount, it->dataType, data->indices);
      } else {
         rapi->drawArrays(data->type, 0, data->vertexCount);
      }
   }
}

GLHCKAPI void glhckRenderObject(const glhckHandle handle)
{
   glhckRenderObjectMany(&handle, 1);
}

GLHCKAPI void glhckRenderText(const glhckHandle handle)
{
   rapi->depthTest(0);
   rapi->blendFunc(GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);
   rapi->projectionMatrix(&rview.orthographic);
   rapi->modelMatrix(&identity);
   rapi->color(glhckTextGetColor(handle));

   const struct glhckTextTexture *t;
   for (chckPoolIndex iter = 0; (t = _glhckTextGetTextTexture(handle, iter)); ++iter) {
      if (!t->geometry.vertexCount)
         continue;

      glhckTextureBind(t->texture);

      kmMat4 textureMatrix;
      const kmVec3 *internal = _glhckTextureGetInternalScale(t->texture);
      kmMat4Scaling(&textureMatrix, internal->x / GLHCK_TEXT_TEXTURE_RANGE, internal->y / GLHCK_TEXT_TEXTURE_RANGE, 1.0f);
      rapi->textureMatrix(&textureMatrix);

      rapi->vertexPointer(2, GLHCK_TEXT_PRECISION, sizeof(GLHCK_TEXT_VTYPE), &t->geometry.vertexData[0].vertex);
      rapi->coordPointer(2, GLHCK_TEXT_PRECISION, sizeof(GLHCK_TEXT_VTYPE), &t->geometry.vertexData[0].coord);
      rapi->drawArrays(GLHCK_TEXT_GTYPE, 0, t->geometry.vertexCount);
   }
}

/* vim: set ts=8 sw=3 tw=0 :*/
