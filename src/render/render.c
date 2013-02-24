#include "../internal.h"
#include "render.h"
#include <assert.h> /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER

/* \brief is renderer initialized? */
int _glhckRenderInitialized(void)
{
   return (glhckRenderName()?1:0);
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
   GLHCK_API_CHECK(programSetUniform);
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
   kmMat4 projection;
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);

   glhckRenderViewport(0, 0, width, height);
   kmMat4PerspectiveProjection(&projection, 35.0f,
         (float)width/(float)height, 0.1f, 100.0f);
   glhckRenderProjectionOnly(&projection);
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
GLHCKAPI void glhckRenderViewport(int x, int y, int width, int height)
{
   kmMat4 ortho;
   GLHCK_INITIALIZED();
   CALL(1, "%d, %d, %d, %d", x, y, width, height);
   assert(x >= 0 && y >= 0 && width > 0 && height > 0);
   if (!_glhckRenderInitialized()) return;

   /* set viewport on render */
   GLHCKRA()->viewport(x, y, width, height);

   /* update orthographic matrix */
   kmMat4OrthographicProjection(&ortho, x, width, height, y, -1, 1);
   GLHCKRA()->setOrthographic(&ortho);
   memcpy(&GLHCKRD()->view.orthographic, &ortho, sizeof(kmMat4));
}

/* \brief set render pass flags */
GLHCKAPI void glhckRenderPassFlags(unsigned int flags)
{
   GLHCK_INITIALIZED();
   CALL(2, "%u", flags);
   if (flags & GLHCK_PASS_DEFAULTS) {
      flags  = GLHCK_PASS_DEPTH;
      flags |= GLHCK_PASS_CULL;
      flags |= GLHCK_PASS_BLEND;
      flags |= GLHCK_PASS_TEXTURE;
      flags |= GLHCK_PASS_OBB;
      flags |= GLHCK_PASS_AABB;
      flags |= GLHCK_PASS_WIREFRAME;
      flags |= GLHCK_PASS_LIGHTING;
   }
   GLHCKRP()->flags = flags;
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
   memcpy(&GLHCKRD()->clearColor, color, sizeof(glhckColorb));
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
   RET(2, "%p", &GLHCKRD()->clearColor);
   return &GLHCKRD()->clearColor;
}

/* \brief clear scene */
GLHCKAPI void glhckRenderClear(unsigned int bufferBits)
{
   GLHCK_INITIALIZED();
   CALL(2, "%u", bufferBits);
   if (!_glhckRenderInitialized()) return;
   GLHCKRA()->clear(bufferBits);
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
   GLHCKRP()->shader = glhckShaderRef(shader);
}

/* \brief get current shader for render pass */
GLHCKAPI glhckShader* glhckRenderPassGetShader(void)
{
   GLHCK_INITIALIZED();
   TRACE(2);
   RET(2, "%p", GLHCKRP()->shader);
   return GLHCKRP()->shader;
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
   _glhckTexture *t;
   _glhckObject *o;
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

         /* don't draw if not same texture or opaque,
          * opaque objects are drawn last */
         if (o->material.texture != t ||
            (o->material.blenda != GLHCK_ZERO || o->material.blendb != GLHCK_ZERO)) {
            if (o->material.texture == t) kt = 1; /* don't remove texture from queue */
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

         /* don't draw if not same texture */
         if (o->material.texture != t) {
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
      memset(objects->queue, 0, objects->count * sizeof(_glhckObject*));
      objects->count = 0;
   }

   if (textures->count) {
      /* something was left un-drawn :o? */
      DEBUG(GLHCK_DBG_CRAP, "COUNT UNLEFT %u", textures->count);
      for (ti = 0; ti != textures->count; ++ti) glhckTextureFree(textures->queue[ti]);
      memset(textures->queue, 0, textures->count * sizeof(_glhckTexture*));
      textures->count = 0;
   }
}

/* vim: set ts=8 sw=3 tw=0 :*/
