#include <glhck/glhck.h>
#include "trace.h"
#include <limits.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER

#include "renderers/helpers/stub.h"
#include "renderers/renderer.h"

static int constructor(struct glhckRendererInfo *info)
{
   CALL(0, "%p", info);

   info->features = (glhckRendererFeatures){
      .driver = GLHCK_DRIVER_SOFTWARE,
      .texture = {
         .maxTextureSize = INT_MAX,
         .maxRenderbufferSize = INT_MAX,
         .hasNativeNpotSupport = 1,
      },
   };

   info->api = (struct glhckRendererAPI){
      .viewport = stubViewport,
      .projectionMatrix = stubProjectionMatrix,
      .modelMatrix = stubModelMatrix,
      .textureMatrix = stubTextureMatrix,
      .time = stubTime,
      .clearColor = stubClearColor,
      .clear = stubClear,
      .color = stubColor,
      .vertexPointer = stubVertexPointer,
      .coordPointer = stubCoordPointer,
      .normalPointer = stubNormalPointer,
      .drawArrays = stubDrawArrays,
      .drawElements = stubDrawElements,
      .bufferGetPixels = stubBufferGetPixels,
      .textureGenerate = stubGenerate,
      .textureDelete = stubDelete,
      .textureBind = stubTextureBind,
      .textureActive = stubTextureActive,
      .textureFill = stubTextureFill,
      .textureImage = stubTextureImage,
      .textureParameter = stubTextureParameter,
      .textureGenerateMipmap = stubTextureGenerateMipmap,
      .renderbufferGenerate = stubGenerate,
      .renderbufferDelete = stubDelete,
      .renderbufferBind = stubRenderbufferBind,
      .renderbufferStorage = stubRenderbufferStorage,
      .framebufferGenerate = stubGenerate,
      .framebufferDelete = stubDelete,
      .framebufferBind = stubFramebufferBind,
      .framebufferTexture = stubFramebufferTexture,
      .framebufferRenderbuffer = stubFramebufferRenderbuffer,
      .hwBufferGenerate = stubGenerate,
      .hwBufferDelete = stubDelete,
      .hwBufferBind = stubHwBufferBind,
      .hwBufferCreate = stubHwBufferCreate,
      .hwBufferFill = stubHwBufferFill,
      .hwBufferBindBase = stubHwBufferBindBase,
      .hwBufferBindRange = stubHwBufferBindRange,
      .hwBufferMap = stubHwBufferMap,
      .hwBufferUnmap = stubHwBufferUnmap,
      .programBind = stubProgramBind,
      .programLink = stubProgramLink,
      .programDelete = stubProgramDelete,
      .shaderCompile = stubShaderCompile,
      .shaderDelete = stubShaderDelete,
      .programUniformBufferPool = stubProgramUniformBufferPool,
      .programAttributePool = stubProgramAttributePool,
      .programUniformPool = stubProgramUniformPool,
      .programUniform = stubProgramUniform,
      .programAttachUniformBuffer = stubProgramAttachUniformBuffer,
      .shadersPath = stubShadersPath,
      .shadersDirectiveToken = stubShadersDirectiveToken
   };

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

GLHCKAPI const char* glhckRendererRegister(struct glhckRendererInfo *info)
{
   CALL(0, "%p", info);

   info->constructor = constructor;
   info->destructor = NULL;

   info->context = (glhckRendererContextInfo){
      .type = GLHCK_CONTEXT_SOFTWARE,
   };

   RET(0, "%s", "glhck-renderer-stub");
   return "glhck-renderer-stub";
}

/* vim: set ts=8 sw=3 tw=0 :*/
