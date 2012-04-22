#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_RTT

/* \brief create new rtt */
GLHCKAPI glhckRtt* glhckRttNew(int width, int height, glhckRttMode mode)
{
   _glhckRtt *rtt;
   _glhckTexture *texture = NULL;
   CALL("%d", mode);

   if (!(rtt = _glhckMalloc(sizeof(_glhckRtt))))
      goto fail;

   if (!(texture = glhckTextureNew(NULL, 0)))
      goto fail;

   /* init */
   memset(rtt, 0, sizeof(_glhckRtt));
   glhckTextureCreate(texture, NULL, width, height,
         mode==GLHCK_RTT_RGB?3:
         mode==GLHCK_RTT_RGBA?4:4, 0);

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

   RET("%p", rtt);
   return rtt;

fail:
   if (rtt) _glhckFree(rtt);
   if (texture) glhckTextureFree(texture);
   RET("%p", NULL);
   return NULL;
}

/* \brief free rtt object */
GLHCKAPI short glhckRttFree(glhckRtt *rtt)
{
   CALL("%p", rtt);
   assert(rtt);

   /* there is still references to this rtt alive */
   if (--rtt->refCounter != 0) goto success;

   /* free fbo object */
   _GLHCKlibrary.render.api.deleteFramebuffers(1, &rtt->object);

   if (rtt->texture)
      glhckTextureFree(rtt->texture);

   _glhckFree(rtt);
   rtt = NULL;

success:
   RET("%d", rtt?rtt->refCounter:0);
   return rtt?rtt->refCounter:0;
}

/* \brief start rendering to rtt */
GLHCKAPI void glhckRttBegin(glhckRtt *rtt)
{
   assert(rtt);
   _GLHCKlibrary.render.api.bindFramebuffer(rtt->object);
}

/* \brief end rendering to rtt */
GLHCKAPI void glhckRttEnd(glhckRtt *rtt)
{
   assert(rtt);
   _GLHCKlibrary.render.api.bindFramebuffer(0);

   /* TODO: copy the frame data to texture */
}
