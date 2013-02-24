#ifndef __glhck_render_h__
#define __glhck_render_h__

/* helper macros */
#define GLHCK_RENDER_INIT(x)        GLHCKR()->name  = x
#define GLHCK_RENDER_TERMINATE(x)   GLHCKR()->name  = NULL
#define GLHCK_RENDER_FUNC(x,y)      GLHCKRA()->x    = y

/* renderers */
void _glhckRenderOpenGLFixedPipeline(void);
void _glhckRenderOpenGL(void);

#endif /* __glhck_render_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
