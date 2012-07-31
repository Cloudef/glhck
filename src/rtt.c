#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_RTT

/* \brief create new rtt */
GLHCKAPI glhckRtt* glhckRttNew(int width, int height, glhckRttMode mode)
{
   _glhckRtt *rtt;
   _glhckTexture *texture = NULL;
   CALL(0, "%d, %d, %d", width, height, mode);

   if (!(rtt = _glhckMalloc(sizeof(_glhckRtt))))
      goto fail;

   if (!(texture = glhckTextureNew(NULL, 0)))
      goto fail;

   /* init */
   memset(rtt, 0, sizeof(_glhckRtt));
   if (!(glhckTextureCreate(texture, NULL, width, height,
         mode==GLHCK_RTT_RGB?GLHCK_RGB:GLHCK_RGBA, 0)))
      goto fail;

   _GLHCKlibrary.render.api.generateFramebuffers(1, &rtt->object);
   if (!rtt->object)
      goto fail;

   if (_GLHCKlibrary.render.api.linkFramebufferWithTexture(rtt->object,
            texture->object, GLHCK_COLOR_ATTACHMENT)
         != RETURN_OK)
      goto fail;

   /* assign the texture */
   rtt->texture = texture;

   /* increase reference */
   rtt->refCounter++;

   /* insert to world */
   _glhckWorldInsert(rlist, rtt, _glhckRtt*);

   RET(0, "%p", rtt);
   return rtt;

fail:
   IFDO(_glhckFree, rtt);
   IFDO(glhckTextureFree, texture);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief free rtt object */
GLHCKAPI short glhckRttFree(glhckRtt *rtt)
{
   CALL(FREE_CALL_PRIO(rtt), "%p", rtt);
   assert(rtt);

   /* there is still references to this rtt alive */
   if (--rtt->refCounter != 0) goto success;

   /* free fbo object */
   _GLHCKlibrary.render.api.deleteFramebuffers(1, &rtt->object);
   IFDO(glhckTextureFree, rtt->texture);

   /* remove from world */
   _glhckWorldRemove(rlist, rtt, _glhckRtt*);

   /* free */
   NULLDO(_glhckFree, rtt);

success:
   RET(FREE_RET_PRIO(rtt), "%d", rtt?rtt->refCounter:0);
   return rtt?rtt->refCounter:0;
}

/* \brief fill rtt's texture data with current pixels */
GLHCKAPI int glhckRttFillData(glhckRtt *rtt)
{
   unsigned char *data;
   CALL(1, "%p", rtt);
   assert(rtt);

   /* texture will take ownership of this data,
    * so let's alloc it explictly for TEXTURE channel */
   data = __glhckMalloc(GLHCK_CHANNEL_TEXTURE,
                       rtt->texture->width    *
                       rtt->texture->height   *
      _glhckNumChannels(rtt->texture->format) *
                       sizeof(unsigned char));

   if (!data)
      goto fail;

   _GLHCKlibrary.render.api.getPixels(0, 0,
         rtt->texture->width, rtt->texture->height,
         rtt->texture->format, data);

   _glhckTextureSetData(rtt->texture, data);

   RET(1, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(1, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief return rtt's texture */
GLHCKAPI glhckTexture* glhckRttGetTexture(glhckRtt *rtt)
{
   CALL(1, "%p", rtt);
   assert(rtt);

   RET(1, "%p", rtt->texture);
   return rtt->texture;
}

/* \brief start rendering to rtt */
GLHCKAPI void glhckRttBegin(glhckRtt *rtt)
{
   CALL(2, "%p", rtt);
   assert(rtt);
   _GLHCKlibrary.render.api.bindFramebuffer(rtt->object);
}

/* \brief end rendering to rtt */
GLHCKAPI void glhckRttEnd(glhckRtt *rtt)
{
   CALL(2, "%p", rtt);
   assert(rtt);
   _GLHCKlibrary.render.api.bindFramebuffer(0);
}

/* vim: set ts=8 sw=3 tw=0 :*/
