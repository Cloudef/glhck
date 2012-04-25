#ifndef __render_h__
#define __render_h__

/* helper macros */
#define GLHCK_RENDER_INIT(x)        _GLHCKlibrary.render.name  = x
#define GLHCK_RENDER_TERMINATE(x)   _GLHCKlibrary.render.name  = NULL
#define GLHCK_RENDER_FUNC(x,y)      _GLHCKlibrary.render.api.x = y

/* renderers */
void _glhckRenderOpenGL(void);

#endif /* __render_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
