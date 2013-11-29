#include "../internal.h"
#include "render.h"
#include <limits.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define RENDER_NAME "NULL Renderer"
#include "helper_stub.h"

/* \brief terminate renderer */
static void renderTerminate(void)
{
   TRACE(0);

   /* this tells library that we are no longer alive. */
   GLHCK_RENDER_TERMINATE(RENDER_NAME);
}

/* \brief renderer main function */
void _glhckRenderStub(void)
{
   glhckRenderFeatures *features;
   TRACE(0);

   /* XXX: Maybe DRIVER_STUB || DRIVER_SOFTWARE? */
   GLHCKR()->driver = GLHCK_DRIVER_OTHER;
   features = &GLHCKR()->features;
   features->texture.maxTextureSize = INT_MAX;
   features->texture.maxRenderbufferSize = INT_MAX;
   features->texture.hasNativeNpotSupport = 1;

   /* register stub api functions */

   GLHCK_RENDER_FUNC(textureGenerate, stubGenerate);
   GLHCK_RENDER_FUNC(textureDelete, stubDelete);
   GLHCK_RENDER_FUNC(textureBind, stubTextureBind);
   GLHCK_RENDER_FUNC(textureActive, stubTextureActive);
   GLHCK_RENDER_FUNC(textureFill, stubTextureFill);
   GLHCK_RENDER_FUNC(textureImage, stubTextureImage);
   GLHCK_RENDER_FUNC(textureParameter, stubTextureParameter);
   GLHCK_RENDER_FUNC(textureGenerateMipmap, stubTextureMipmap);
   GLHCK_RENDER_FUNC(lightBind, stubLightBind);
   GLHCK_RENDER_FUNC(renderbufferGenerate, stubGenerate);
   GLHCK_RENDER_FUNC(renderbufferDelete, stubDelete);
   GLHCK_RENDER_FUNC(renderbufferBind, stubRenderbufferBind);
   GLHCK_RENDER_FUNC(renderbufferStorage, stubRenderbufferStorage);
   GLHCK_RENDER_FUNC(framebufferGenerate, stubGenerate);
   GLHCK_RENDER_FUNC(framebufferDelete, stubDelete);
   GLHCK_RENDER_FUNC(framebufferBind, stubFramebufferBind);
   GLHCK_RENDER_FUNC(framebufferTexture, stubFramebufferTexture);
   GLHCK_RENDER_FUNC(framebufferRenderbuffer, stubFramebufferRenderbuffer);
   GLHCK_RENDER_FUNC(hwBufferGenerate, stubGenerate);
   GLHCK_RENDER_FUNC(hwBufferDelete, stubDelete);
   GLHCK_RENDER_FUNC(hwBufferBind, stubHwBufferBind);
   GLHCK_RENDER_FUNC(hwBufferBindBase, stubHwBufferBindBase);
   GLHCK_RENDER_FUNC(hwBufferBindRange, stubHwBufferBindRange);
   GLHCK_RENDER_FUNC(hwBufferCreate, stubHwBufferCreate);
   GLHCK_RENDER_FUNC(hwBufferFill, stubHwBufferFill);
   GLHCK_RENDER_FUNC(hwBufferMap, stubHwBufferMap);
   GLHCK_RENDER_FUNC(hwBufferUnmap, stubHwBufferUnmap);
   GLHCK_RENDER_FUNC(programBind, stubProgramBind);
   GLHCK_RENDER_FUNC(programLink, stubProgramLink);
   GLHCK_RENDER_FUNC(programDelete, stubProgramDelete);
   GLHCK_RENDER_FUNC(programUniform, stubProgramUniform);
   GLHCK_RENDER_FUNC(programUniformBufferList, stubProgramUniformBufferList);
   GLHCK_RENDER_FUNC(programAttributeList, stubProgramAttributeList);
   GLHCK_RENDER_FUNC(programUniformList, stubProgramUniformList);
   GLHCK_RENDER_FUNC(programAttachUniformBuffer, stubProgramAttachUniformBuffer);
   GLHCK_RENDER_FUNC(shaderCompile, stubShaderCompile);
   GLHCK_RENDER_FUNC(shaderDelete, stubShaderDelete);
   GLHCK_RENDER_FUNC(shadersPath, stubShadersPath);
   GLHCK_RENDER_FUNC(shadersDirectiveToken, stubShadersDirectiveToken);
   GLHCK_RENDER_FUNC(setOrthographic, stubSetOrthographic);
   GLHCK_RENDER_FUNC(setProjection, stubSetProjection);
   GLHCK_RENDER_FUNC(setView, stubSetView);
   GLHCK_RENDER_FUNC(clearColor, stubClearColor);
   GLHCK_RENDER_FUNC(clear, stubClear);
   GLHCK_RENDER_FUNC(objectRender, stubObjectRender);
   GLHCK_RENDER_FUNC(textRender, stubTextRender);
   GLHCK_RENDER_FUNC(frustumRender, stubFrustumRender);
   GLHCK_RENDER_FUNC(bufferGetPixels, stubBufferGetPixels);
   GLHCK_RENDER_FUNC(time, stubTime);
   GLHCK_RENDER_FUNC(viewport, stubViewport);

   /* common */
   GLHCK_RENDER_FUNC(terminate, renderTerminate);

   /* this also tells library that everything went OK, so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);
}

/* vim: set ts=8 sw=3 tw=0 :*/
