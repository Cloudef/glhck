#ifndef __glhck_render_h__
#define __glhck_render_h__

#include <kazmath/kazmath.h>
#include "renderer.h"

void _glhckRenderAPI(const struct glhckRendererAPI *api);
const struct glhckRendererAPI* _glhckRenderGetAPI(void);
const kmMat4* _glhckRenderGetFlipMatrix(void);
void _glhckRenderDefaultProjection(int width, int height);
void _glhckRenderCheckApi(void);

#endif /* __glhck_render_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
