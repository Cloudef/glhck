#ifndef __glhck_common_h__
#define __glhck_common_h__

/* call prioritory for free calls on reference counted objects.
 * use these on CALL() and RET(), function calls on reference counted object's free function. */
#define FREE_CALL_PRIO(o) (o?o->refCounter==1?0:3:0)
#define FREE_RET_PRIO(o)  (o?3:0)

/* if exists then perform function and set NULL
 * used mainly to shorten if (x) free(x); x = NULL; */
#define IFDO(f, x) { if (x) f(x); x = NULL; }

/* perform function and set NULL (no checks)
 * used mainly to shorten free(x); x = NULL; */
#define NULLDO(f, x) { f(x); x = NULL; }

/*** format macros ***/

#define RECT(r)   (r)?(r)->x:-1, (r)?(r)->y:-1, (r)?(r)->w:-1, (r)?(r)->h:-1
#define RECTS     "rect[%f, %f, %f, %f]"
#define COLB(c)   (c)?(c)->r:0, (c)?(c)->g:0, (c)?(c)->b:0, (c)?(c)->a:0
#define COLBS     "colb[%d, %d, %d, %d]"
#define VEC2(v)   (v)?(v)->x:-1, (v)?(v)->y:-1
#define VEC2S     "vec2[%f, %f]"
#define VEC3(v)   (v)?(v)->x:-1, (v)?(v)->y:-1, (v)?(v)->z:-1
#define VEC3S     "vec3[%f, %f, %f]"
#define VEC4(v)   (v)?(v)->x:-1, (v)?(v)->y:-1, (v)?(v)->z:-1, (v)?(v)->w:-1
#define VEC4S     "vec3[%f, %f, %f, %f]"

/*** image macros ***/

#define IMAGE_DIMENSIONS_OK(w, h) \
   (((w) > 0) && ((h) > 0) &&     \
    ((unsigned long long)(w) * (unsigned long long)(h) <= (1ULL << 29) - 1))

/*** vector macros ***/

/* assign 2D v2 vector to v1 vector regardless
 * of datatype */
#define glhckSetV2(v1, v2) \
   (v1)->x = (v2)->x;      \
   (v1)->y = (v2)->y

/* assign 3D v2 vector to v1 vector regardless
 * of datatype */
#define glhckSetV3(v1, v2) \
   glhckSetV2(v1, v2);     \
   (v1)->z = (v2)->z

/* assign the max units from vectors to v1 */
#define glhckMaxV2(v1, v2) \
   if ((v1)->x < (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y < (v2)->y) (v1)->y = (v2)->y
#define glhckMaxV3(v1, v2) \
   glhckMaxV2(v1, v2);     \
   if ((v1)->z < (v2)->z) (v1)->z = (v2)->z

/* assign the min units from vectors to v1 */
#define glhckMinV2(v1, v2) \
   if ((v1)->x > (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y > (v2)->y) (v1)->y = (v2)->y
#define glhckMinV3(v1, v2) \
   glhckMinV2(v1, v2);     \
   if ((v1)->z > (v2)->z) (v1)->z = (v2)->z

#endif /* __glhck_common_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
