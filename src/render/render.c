#include "../internal.h"
#include "render.h"
#include <assert.h> /* for assert */

#ifndef __STRING
#  define __STRING(x) #x
#endif

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER

/* \brief internal glhck flip matrix
 * every model matrix is multiplied with this when glhckRenderFlip(1) is used */
const kmMat4 _glhckFlipMatrix = {
   .mat = {
      1.0f,  0.0f, 0.0f, 0.0f,
      0.0f, -1.0f, 0.0f, 0.0f,
      0.0f,  0.0f, 1.0f, 0.0f,
      0.0f,  0.0f, 0.0f, 1.0f,
   }
};

/* \brief is renderer initialized? */
int _glhckRenderInitialized(void)
{
   return (GLHCKR()->name?1:0);
}

/* \brief macro and function for checking render api calls */
#define GLHCK_API_CHECK(x) \
if (!GLHCKRA()->x) DEBUG(GLHCK_DBG_ERROR, "[%s] \1missing render API function: %s", glhckRenderName(), __STRING(x))
void _glhckRenderCheckApi(void)
{
   GLHCK_API_CHECK(time);
   GLHCK_API_CHECK(terminate);
   GLHCK_API_CHECK(viewport);
   GLHCK_API_CHECK(setProjection);
   GLHCK_API_CHECK(setView);
   GLHCK_API_CHECK(setOrthographic);
   GLHCK_API_CHECK(clearColor);
   GLHCK_API_CHECK(clear);
   GLHCK_API_CHECK(objectRender);
   GLHCK_API_CHECK(textRender);
   GLHCK_API_CHECK(frustumRender);
   GLHCK_API_CHECK(bufferGetPixels);
   GLHCK_API_CHECK(textureGenerate);
   GLHCK_API_CHECK(textureDelete);
   GLHCK_API_CHECK(textureBind);
   GLHCK_API_CHECK(textureActive);
   GLHCK_API_CHECK(textureFill);
   GLHCK_API_CHECK(textureImage);
   GLHCK_API_CHECK(textureParameter);
   GLHCK_API_CHECK(renderbufferGenerate);
   GLHCK_API_CHECK(renderbufferDelete);
   GLHCK_API_CHECK(renderbufferBind);
   GLHCK_API_CHECK(renderbufferStorage);
   GLHCK_API_CHECK(framebufferGenerate);
   GLHCK_API_CHECK(framebufferDelete);
   GLHCK_API_CHECK(framebufferBind);
   GLHCK_API_CHECK(framebufferTexture);
   GLHCK_API_CHECK(framebufferRenderbuffer);
   GLHCK_API_CHECK(hwBufferGenerate);
   GLHCK_API_CHECK(hwBufferDelete);
   GLHCK_API_CHECK(hwBufferBind);
   GLHCK_API_CHECK(hwBufferCreate);
   GLHCK_API_CHECK(hwBufferFill);
   GLHCK_API_CHECK(hwBufferBindBase);
   GLHCK_API_CHECK(hwBufferBindRange);
   GLHCK_API_CHECK(hwBufferMap);
   GLHCK_API_CHECK(hwBufferUnmap);
   GLHCK_API_CHECK(programLink);
   GLHCK_API_CHECK(programDelete);
   GLHCK_API_CHECK(programUniformBufferList);
   GLHCK_API_CHECK(programAttributeList);
   GLHCK_API_CHECK(programUniformList);
   GLHCK_API_CHECK(programUniform);
   GLHCK_API_CHECK(programAttachUniformBuffer);
   GLHCK_API_CHECK(shaderCompile);
   GLHCK_API_CHECK(shaderDelete);
   GLHCK_API_CHECK(shadersPath);
   GLHCK_API_CHECK(shadersDirectiveToken);
}
#undef GLHCK_API_CHECK

/* \brief builds and sends the default projection to renderer */
void _glhckRenderDefaultProjection(int width, int height)
{
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);
   glhckRenderProjection2D(width, height, -100.0f, 100.0f);
}

/***
 * public api
 ***/

/* \brief get current renderer name */
GLHCKAPI const char* glhckRenderName(void)
{
   GLHCK_INITIALIZED();
   TRACE(0);
   RET(0, "%s", GLHCKR()->name);
   return GLHCKR()->name;
}

/* \brief return detected driver type */
GLHCKAPI glhckDriverType glhckRenderGetDriver(void)
{
   GLHCK_INITIALIZED();
   TRACE(0);
   RET(0, "%u", GLHCKR()->driver);
   return GLHCKR()->driver;
}

/* \brief get renderer features */
GLHCKAPI const glhckRenderFeatures* glhckRenderGetFeatures(void)
{
   GLHCK_INITIALIZED();
   TRACE(0);
   RET(0, "%p", &GLHCKR()->features);
   return &GLHCKR()->features;
}

/* \brief resize render viewport internally */
GLHCKAPI void glhckRenderResize(int width, int height)
{
   GLHCK_INITIALIZED();
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);
   if (!_glhckRenderInitialized()) return;

   /* nothing to resize */
   if (GLHCKR()->width == width && GLHCKR()->height == height)
      return;

   /* update all cameras */
   _glhckCameraWorldUpdate(width, height);

   /* update on library last, so functions know the old values */
   GLHCKR()->width  = width;
   GLHCKR()->height = height;
}

/* \brief set renderer's viewport */
GLHCKAPI void glhckRenderViewport(const glhckRect *viewport)
{
   kmMat4 ortho;
   GLHCK_INITIALIZED();
   CALL(1, RECTS, RECT(viewport));
   assert(viewport->x >= 0 && viewport->y >= 0 && viewport->w > 0 && viewport->h > 0);
   if (!_glhckRenderInitialized()) return;

   /* set viewport on render */
   GLHCKRA()->viewport(viewport->x, viewport->y, viewport->w, viewport->h);
   memcpy(&GLHCKRP()->viewport, viewport, sizeof(glhckRect));

   /* update orthographic matrix */
   kmMat4OrthographicProjection(&ortho, viewport->x, viewport->w, viewport->h, viewport->y, -1.0f, 1.0f);
   GLHCKRA()->setOrthographic(&ortho);
   memcpy(&GLHCKRD()->view.orthographic, &ortho, sizeof(kmMat4));
}

/* \brief set renderer's viewport (int) */
GLHCKAPI void glhckRenderViewporti(int x, int y, int width, int height)
{
   glhckRect viewport = {x, y, width, height};
   glhckRenderViewport(&viewport);
}

/* \brief push current render state to stack */
GLHCKAPI void glhckRenderStatePush(void)
{
   __GLHCKrenderState *state;
   GLHCK_INITIALIZED();
   TRACE(2);

   if (!(state = _glhckMalloc(sizeof(__GLHCKrenderState))))
      return;

   memcpy(&state->pass, &GLHCKR()->pass, sizeof(__GLHCKrenderPass));
   memcpy(&state->view, &GLHCKRD()->view, sizeof(__GLHCKrenderView));
   state->width = GLHCKR()->width; state->height = GLHCKR()->height;
   state->next = GLHCKR()->stack;
   GLHCKR()->stack = state;
}

/* \brief push 2D drawing state */
GLHCKAPI void glhckRenderStatePush2D(int width, int height, kmScalar near, kmScalar far)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   glhckRenderStatePush();
   glhckRenderPass(glhckRenderGetPass() & ~GLHCK_PASS_DEPTH);
   glhckRenderProjection2D(width, height, near, far);
}

/* \brief pop render state from stack */
GLHCKAPI void glhckRenderStatePop(void)
{
   __GLHCKrenderState *state, *newState;

   if (!(state = GLHCKR()->stack))
      return;

   glhckRenderResize(state->width, state->height);
   memcpy(&GLHCKR()->pass, &state->pass, sizeof(__GLHCKrenderPass));
   glhckRenderClearColor(&state->pass.clearColor);
   glhckRenderViewport(&state->pass.viewport);

   glhckRenderProjection(&state->view.projection);
   glhckRenderView(&state->view.view);
   GLHCKRA()->setOrthographic(&state->view.orthographic);
   memcpy(&GLHCKRD()->view.orthographic, &state->view.orthographic, sizeof(kmMat4));
   glhckRenderFlip(state->view.flippedProjection);

   newState = (state?state->next:NULL);
   IFDO(_glhckFree, state);
   GLHCKR()->stack = newState;
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
   GLHCK_INITIALIZED();
   CALL(2, "%u", flags);
   GLHCKRP()->flags = flags;
}

/* \brief get current render pass flags */
GLHCKAPI unsigned int glhckRenderGetPass(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%u", GLHCKRP()->flags);
   return GLHCKRP()->flags;
}

/* \brief give current program time to glhck */
GLHCKAPI void glhckRenderTime(float time)
{
   GLHCK_INITIALIZED();
   CALL(2, "%f", time);
   if (!_glhckRenderInitialized()) return;
   GLHCKRA()->time(time);
}

/* \brief set clear color to render */
GLHCKAPI void glhckRenderClearColor(const glhckColorb *color)
{
   GLHCK_INITIALIZED();
   CALL(2, "%p", color);
   if (!_glhckRenderInitialized()) return;
   GLHCKRA()->clearColor(color);
   memcpy(&GLHCKRP()->clearColor, color, sizeof(glhckColorb));
}

/* \brief set clear color to render */
GLHCKAPI void glhckRenderClearColorb(char r, char g, char b, char a)
{
   glhckColorb color = {r,g,b,a};
   glhckRenderClearColor(&color);
}

/* \brief return current clear color */
GLHCKAPI const glhckColorb* glhckRenderGetClearColor(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%p", &GLHCKRP()->clearColor);
   return &GLHCKRP()->clearColor;
}

/* \brief clear scene */
GLHCKAPI void glhckRenderClear(unsigned int bufferBits)
{
   GLHCK_INITIALIZED();
   CALL(2, "%u", bufferBits);
   if (!_glhckRenderInitialized()) return;

#if EMSCRIPTEN
   /* when there is no framebuffers bound assume we are in
    * main loop don't do clear since webgl does clear itself.
    * XXX: document this behaviour */
   int i;
   for (i = 0; i < GLHCK_FRAMEBUFFER_TYPE_LAST && !GLHCKRD()->framebuffer[i]; ++i);
   if (i == GLHCK_FRAMEBUFFER_TYPE_LAST) return;
#endif

   GLHCKRA()->clear(bufferBits);
}

/* \brief set render pass front face orientation */
GLHCKAPI void glhckRenderFrontFace(glhckFaceOrientation orientation)
{
   GLHCK_INITIALIZED();
   CALL(2, "%d", orientation);
   GLHCKRP()->frontFace = orientation;
}

/* \brief return current render pass front face */
GLHCKAPI glhckFaceOrientation glhckRenderGetFrontFace(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%u", GLHCKRP()->frontFace);
   return GLHCKRP()->frontFace;
}

/* \brief set render pass cull face side */
GLHCKAPI void glhckRenderCullFace(glhckCullFaceType face)
{
   GLHCK_INITIALIZED();
   CALL(2, "%d", face);
   GLHCKRP()->cullFace = face;
}

/* \brief return current render pass cull face */
GLHCKAPI glhckCullFaceType glhckRenderGetCullFace(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%u", GLHCKRP()->cullFace);
   return GLHCKRP()->cullFace;
}

/* \brief set render pass blend func that overrides object specific */
GLHCKAPI void glhckRenderBlendFunc(glhckBlendingMode blenda, glhckBlendingMode blendb)
{
   GLHCK_INITIALIZED();
   CALL(2, "%d, %d", blenda, blendb);
   GLHCKRP()->blenda = blenda;
   GLHCKRP()->blendb = blendb;
}

/* \brief get current blend func for render pass */
GLHCKAPI void glhckRenderGetBlendFunc(glhckBlendingMode *blenda, glhckBlendingMode *blendb)
{
   GLHCK_INITIALIZED();
   CALL(2, "%p, %p", blenda, blendb);
   if (blenda) *blenda = GLHCKRP()->blenda;
   if (blendb) *blendb = GLHCKRP()->blendb;
}

/* \brief set shader for render pass */
GLHCKAPI void glhckRenderPassShader(glhckShader *shader)
{
   GLHCK_INITIALIZED();
   CALL(2, "%p", shader);
   if (GLHCKRP()->shader) glhckShaderFree(GLHCKRP()->shader);
   if (shader) GLHCKRP()->shader = glhckShaderRef(shader);
   else GLHCKRP()->shader = NULL;
}

/* \brief get current shader for render pass */
GLHCKAPI glhckShader* glhckRenderPassGetShader(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%p", GLHCKRP()->shader);
   return GLHCKRP()->shader;
}

/* \brief set vertically flipped rendering mode */
GLHCKAPI void glhckRenderFlip(int flip)
{
   GLHCK_INITIALIZED();
   CALL(2, "%d", flip);
   GLHCKRD()->view.flippedProjection = flip;
}

/* \brief get current flipping */
GLHCKAPI int glhckRenderGetFlip(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   return GLHCKRD()->view.flippedProjection;
}

/* \brief set projection matrix */
GLHCKAPI void glhckRenderProjection(const kmMat4 *mat)
{
   GLHCK_INITIALIZED();
   CALL(2, "%p", mat);
   if (!_glhckRenderInitialized()) return;
   GLHCKRA()->setProjection(mat);
   memcpy(&GLHCKRD()->view.projection, mat, sizeof(kmMat4));
}

/* \brief set projection matrix, and identity view matrix */
GLHCKAPI void glhckRenderProjectionOnly(const kmMat4 *mat)
{
   kmMat4 identity;
   GLHCK_INITIALIZED();
   CALL(2, "%p", mat);
   kmMat4Identity(&identity);
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
   kmMat4 ortho;
   GLHCK_INITIALIZED();
   CALL(2, "%d, %d, %f, %f", width, height, near, far);
   assert(width > 0 && height > 0);
   kmMat4OrthographicProjection(&ortho, 0, (kmScalar)width, (kmScalar)height, 0, near, far);
   glhckRenderProjectionOnly(&ortho);
   glhckRenderFlip(1);
}

/* \brief return projection matrix */
GLHCKAPI const kmMat4* glhckRenderGetProjection(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%p", &GLHCKRD()->view.projection);
   return &GLHCKRD()->view.projection;
}

/* \brief set view matrix */
GLHCKAPI void glhckRenderView(const kmMat4 *mat)
{
   GLHCK_INITIALIZED();
   CALL(2, "%p", mat);
   if (!_glhckRenderInitialized()) return;
   GLHCKRA()->setView(mat);
   memcpy(&GLHCKRD()->view.view, mat, sizeof(kmMat4));
}

/* \brief return view matrix */
GLHCKAPI const kmMat4* glhckRenderGetView(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%p", &GLHCKRD()->view.view);
   return &GLHCKRD()->view.view;
}

/* \brief output queued objects */
GLHCKAPI void glhckRenderPrintObjectQueue(void)
{
   unsigned int i;
   __GLHCKobjectQueue *objects;
   GLHCK_INITIALIZED();

   objects = &GLHCKRD()->objects;
   _glhckPuts("\n--- Object Queue ---");
   for (i = 0; i != objects->count; ++i)
      _glhckPrintf("%u. %p", i, objects->queue[i]);
   _glhckPuts("--------------------");
   _glhckPrintf("count/alloc: %u/%u", objects->count, objects->allocated);
   _glhckPuts("--------------------\n");
}

 /* \brief output queued textures */
GLHCKAPI void glhckRenderPrintTextureQueue(void)
{
   unsigned int i;
   __GLHCKtextureQueue *textures;
   GLHCK_INITIALIZED();

   textures = &GLHCKRD()->textures;
   _glhckPuts("\n--- Texture Queue ---");
   for (i = 0; i != textures->count; ++i)
      _glhckPrintf("%u. %p", i, textures->queue[i]);
   _glhckPuts("---------------------");
   _glhckPrintf("count/alloc: %u/%u", textures->count, textures->allocated);
   _glhckPuts("--------------------\n");
}

/* \brief render scene */
GLHCKAPI void glhckRender(void)
{
   unsigned int ti, oi, ts, os, tc, oc;
   char kt;
   glhckTexture *t;
   glhckObject *o;
   __GLHCKobjectQueue *objects;
   __GLHCKtextureQueue *textures;
   GLHCK_INITIALIZED();
   TRACE(2);

   /* can't render */
   if (!glhckInitialized() || !_glhckRenderInitialized())
      return;

   objects  = &GLHCKRD()->objects;
   textures = &GLHCKRD()->textures;

   /* store counts for enumeration, +1 for untexture objects */
   tc = textures->count+1;
   oc = objects->count;

   /* nothing to draw */
   if (!oc)
      return;

   /* draw in sorted texture order */
   for (ti = 0, ts = 0; ti != tc; ++ti) {
      if (ti < tc-1) {
         if (!(t = textures->queue[ti])) continue;
      } else t = NULL; /* untextured object */

      for (oi = 0, os = 0, kt = 0; oi != oc; ++oi) {
         if (!(o = objects->queue[oi])) continue;

         if (o->material) {
            /* don't draw if not same texture or opaque,
             * opaque objects are drawn last */
            if (o->material->texture != t ||
                  (o->material->blenda != GLHCK_ZERO || o->material->blendb != GLHCK_ZERO)) {
               if (o->material->texture == t) kt = 1; /* don't remove texture from queue */
               if (os != oi) objects->queue[oi] = NULL;
               objects->queue[os++] = o;
               continue;
            }
         } else if (t) {
            /* no material, but texture requested */
            if (os != oi) objects->queue[oi] = NULL;
            objects->queue[os++] = o;
            continue;
         }

         /* render object */
         glhckObjectRender(o);
         glhckObjectFree(o); /* referenced on draw call */
         objects->queue[oi] = NULL;
         --objects->count;
      }

      /* check if we need texture again or not */
      if (kt) {
         if (ts != ti) textures->queue[ti] = NULL;
         textures->queue[ts++] = t;
      } else {
         if (t) {
            glhckTextureFree(t); /* ref is increased on draw call! */
            textures->queue[ti] = NULL;
            --textures->count;
         }
      }
   }

   /* store counts for enumeration, +1 for untextured objects */
   tc = textures->count+1;
   oc = objects->count;

   /* FIXME: shift queue here */
   if (oc) {
   }

   /* draw opaque objects next,
    * FIXME: this should not be done in texture order,
    * instead draw from farthest to nearest. (I hate opaque objects) */
   for (ti = 0; ti != tc && oc; ++ti) {
      if (ti < tc-1) {
         if (!(t = textures->queue[ti])) continue;
      } else t = NULL; /* untextured object */

      for (oi = 0, os = 0; oi != oc; ++oi) {
         if (!(o = objects->queue[oi])) continue;

         if (o->material) {
            /* don't draw if not same texture */
            if (o->material->texture != t) {
               if (os != oi) objects->queue[oi] = NULL;
               objects->queue[os++] = o;
               continue;
            }
         } else if (t) {
            if (os != oi) objects->queue[oi] = NULL;
            objects->queue[os++] = o;
            continue;
         }

         /* render object */
         glhckObjectRender(o);
         glhckObjectFree(o); /* referenced on draw call */
         objects->queue[oi] = NULL;
         --objects->count;
      }

      /* this texture is done for */
      if (t) {
         glhckTextureFree(t); /* ref is increased on draw call! */
         textures->queue[ti] = NULL;
         --textures->count;
      }

      /* no texture, time to break */
      if (!t) break;
   }

   /* FIXME: shift queue here if leftovers */

   /* good we got no leftovers \o/ */
   if (objects->count) {
      /* something was left un-drawn :o? */
      for (oi = 0; oi != objects->count; ++oi) glhckObjectFree(objects->queue[oi]);
      memset(objects->queue, 0, objects->count * sizeof(glhckObject*));
      objects->count = 0;
   }

   if (textures->count) {
      /* something was left un-drawn :o? */
      DEBUG(GLHCK_DBG_CRAP, "COUNT UNLEFT %u", textures->count);
      for (ti = 0; ti != textures->count; ++ti) glhckTextureFree(textures->queue[ti]);
      memset(textures->queue, 0, textures->count * sizeof(glhckTexture*));
      textures->count = 0;
   }
}

/* vim: set ts=8 sw=3 tw=0 :*/
