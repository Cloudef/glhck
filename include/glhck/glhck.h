#ifndef __glhck_h__
#define __glhck_h__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__))
#  define _WIN32
#endif

#if defined(_WIN32) && defined(_GLHCK_BUILD_DLL)
#  define GLHCKAPI __declspec(dllexport)
#elif defined(_WIN32) && defined(GLHCK_DLL)
#  if defined(__LCC__)
#     define GLHCKAPI extern
#  else
#     define GLHCKAPI __declspec(dllimport)
#  endif
#else
#  define GLHCKAPI
#endif

#ifndef size_t
#  include <stddef.h>
#endif

#include <kazmath/kazmath.h> /* glhck needs kazmath */
#include <kazmath/vec4.h> /* is not included by kazmath.h */

typedef size_t glhckHandle;
typedef unsigned int glhckFont;
typedef unsigned char glhckVertexType;
typedef unsigned char glhckIndexType;
typedef unsigned int glhckColor;

#include <glhck/glmap.h>
#include <glhck/geometry.h>

/* debugging level */
typedef enum glhckDebugLevel {
   GLHCK_DBG_ERROR,
   GLHCK_DBG_WARNING,
   GLHCK_DBG_CRAP,
} glhckDebugLevel;

/* \brief compile time features struct */
typedef struct glhckCompileFeatures {
} glhckCompileFeatures;

/* driver type */
typedef enum glhckDriverType {
   GLHCK_DRIVER_NONE,
   GLHCK_DRIVER_OTHER,
   GLHCK_DRIVER_SOFTWARE,
   GLHCK_DRIVER_MESA,
   GLHCK_DRIVER_NVIDIA,
   GLHCK_DRIVER_ATI,
   GLHCK_DRIVER_IMGTEC,
} glhckDriverType;

typedef enum glhckContextType {
   GLHCK_CONTEXT_SOFTWARE,
   GLHCK_CONTEXT_OPENGL,
} glhckContextType;

/* render pass bits */
typedef enum glhckRenderPassFlags {
   GLHCK_PASS_NONE           = 0,
   GLHCK_PASS_DEPTH          = 1<<0,
   GLHCK_PASS_CULL           = 1<<1,
   GLHCK_PASS_BLEND          = 1<<2,
   GLHCK_PASS_TEXTURE        = 1<<3,
   GLHCK_PASS_DRAW_OBB       = 1<<4,
   GLHCK_PASS_DRAW_AABB      = 1<<5,
   GLHCK_PASS_DRAW_SKELETON  = 1<<6,
   GLHCK_PASS_DRAW_WIREFRAME = 1<<7,
   GLHCK_PASS_LIGHTING       = 1<<8,
   GLHCK_PASS_OVERDRAW       = 1<<9,
} glhckRenderPassFlags;

/* \brief version render features */
typedef struct glhckRendererFeaturesVersion {
   int major, minor;
   int shaderMajor, shaderMinor;
} glhckRendererFeaturesVersion;

/* \brief texture render features */
typedef struct glhckRendererFeaturesTexture {
   int maxTextureSize;
   int maxRenderbufferSize;
   unsigned char hasNativeNpotSupport;
} glhckRendererFeaturesTexture;

/* \brief renderer features */
typedef struct glhckRendererFeatures {
   glhckDriverType driver;
   glhckRendererFeaturesVersion version;
   glhckRendererFeaturesTexture texture;
} glhckRendererFeatures;

typedef struct glhckRendererContextInfo {
   glhckContextType type;
   int major, minor;
   int coreProfile;
   int forwardCompatible;
} glhckRendererContextInfo;

/* texture parameters struct */
typedef struct glhckTextureParameters {
   float maxAnisotropy;
   float minLod, maxLod, biasLod;
   int baseLevel, maxLevel;
   glhckTextureWrap wrapS, wrapT, wrapR;
   glhckTextureFilter minFilter, magFilter;
   glhckTextureCompareMode compareMode;
   glhckCompareFunc compareFunc;
   unsigned char mipmap;
} glhckTextureParameters;

/* projection type */
typedef enum glhckProjectionType {
   GLHCK_PROJECTION_PERSPECTIVE,
   GLHCK_PROJECTION_ORTHOGRAPHIC,
} glhckProjectionType;

/* frustum planes */
typedef enum glhckFrustumPlane {
   GLHCK_FRUSTUM_PLANE_LEFT,
   GLHCK_FRUSTUM_PLANE_RIGHT,
   GLHCK_FRUSTUM_PLANE_BOTTOM,
   GLHCK_FRUSTUM_PLANE_TOP,
   GLHCK_FRUSTUM_PLANE_NEAR,
   GLHCK_FRUSTUM_PLANE_FAR,
   GLHCK_FRUSTUM_PLANE_LAST
} glhckFrustumPlane;

/* frustum corners */
typedef enum glhckFrustumCorner {
   GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_LEFT,
   GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT,
   GLHCK_FRUSTUM_CORNER_NEAR_TOP_RIGHT,
   GLHCK_FRUSTUM_CORNER_NEAR_TOP_LEFT,
   GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_LEFT,
   GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_RIGHT,
   GLHCK_FRUSTUM_CORNER_FAR_TOP_RIGHT,
   GLHCK_FRUSTUM_CORNER_FAR_TOP_LEFT,
   GLHCK_FRUSTUM_CORNER_LAST
} glhckFrustumCorner;

typedef enum glhckFrustumTestResult {
   GLHCK_FRUSTUM_OUTSIDE,
   GLHCK_FRUSTUM_INSIDE,
   GLHCK_FRUSTUM_PARTIAL
} glhckFrustumTestResult;

/* frustum struct */
typedef struct glhckFrustum {
   kmPlane planes[GLHCK_FRUSTUM_PLANE_LAST];
   kmVec3 corners[GLHCK_FRUSTUM_CORNER_LAST];
} glhckFrustum;

/* parent affection flags */
typedef enum glhckObjectAffectionFlags {
   GLHCK_AFFECT_NONE        = 0,
   GLHCK_AFFECT_TRANSLATION = 1<<0,
   GLHCK_AFFECT_ROTATION    = 1<<1,
   GLHCK_AFFECT_SCALING     = 1<<2,
} glhckObjectAffectionFlags;

/* material flags */
typedef enum glhckMaterialOptionsFlags {
   GLHCK_MATERIAL_NONE     = 0,
   GLHCK_MATERIAL_LIGHTING = 1<<0,
} glhckMaterialOptionsFlags;

/* texture compression type */
typedef enum glhckTextureCompression {
   GLHCK_COMPRESSION_NONE,
   GLHCK_COMPRESSION_DXT,
} glhckTextureCompression;

/* common datatypes */
typedef struct glhckRect {
   kmScalar x, y, w, h;
} glhckRect;

/* vector animation key */
typedef struct glhckAnimationVectorKey {
   kmVec3 vector;
   kmScalar time;
} glhckAnimationVectorKey;

/* quaternion animation key */
typedef struct glhckAnimationQuaternionKey {
   kmQuaternion quaternion;
   kmScalar time;
} glhckAnimationQuaternionKey;

/* datatype that presents bone's vertex weight */
typedef struct glhckVertexWeight {
   kmScalar weight;
   glhckImportIndexData vertexIndex;
} glhckVertexWeight;

typedef struct glhckPostProcessModelParameters {
   unsigned char animated;
   unsigned char flatten;
} glhckPostProcessModelParameters;

typedef struct glhckPostProcessImageParameters {
   glhckTextureCompression compression;
} glhckPostProcessImageParameters;

/* collision callbacks */
struct glhckCollisionInData;
struct glhckCollisionOutData;
struct _glhckCollisionHandle;
typedef void (*glhckCollisionResponseFunction)(const struct glhckCollisionOutData *collision);
typedef int (*glhckCollisionTestFunction)(const struct glhckCollisionInData *data, const glhckHandle colliderPrimitive);

/* collision input data */
typedef struct glhckCollisionInData {
   glhckCollisionTestFunction test;
   glhckCollisionResponseFunction response;
   const kmVec3 *velocity;
   void *userData;
} glhckCollisionInData;

/* collision output data */
typedef struct glhckCollisionOutData {
   const glhckHandle collisionWorld;
   const glhckHandle colliderPrimitive;
   const kmVec3 *contactPoint;
   const kmVec3 *pushVector;
   const kmVec3 *velocity;
   void *userData;
} glhckCollisionOutData;

typedef enum glhckType {
   GLHCK_TYPE_ANIMATION,
   GLHCK_TYPE_ANIMATION_NODE,
   GLHCK_TYPE_ANIMATOR,
   GLHCK_TYPE_ATLAS,
   GLHCK_TYPE_BONE,
   GLHCK_TYPE_CAMERA,
   GLHCK_TYPE_FRAMEBUFFER,
   GLHCK_TYPE_HWBUFFER,
   GLHCK_TYPE_LIGHT,
   GLHCK_TYPE_MATERIAL,
   GLHCK_TYPE_OBJECT,
   GLHCK_TYPE_RENDERBUFFER,
   GLHCK_TYPE_SHADER,
   GLHCK_TYPE_SKINBONE,
   GLHCK_TYPE_TEXT,
   GLHCK_TYPE_TEXTURE,
   GLHCK_TYPE_GEOMETRY,
   GLHCK_TYPE_VIEW,
   GLHCK_TYPE_STRING,
   GLHCK_TYPE_LIST,
   GLHCK_TYPE_LAST
} glhckType;

typedef void (*glhckDebugHookFunc)(const char *file, const int line, const char *function, const glhckDebugLevel level, const char *channel, const char *str);

/* can be called before context */
GLHCKAPI void glhckGetCompileFeatures(glhckCompileFeatures *features);

/* init && terminate */
GLHCKAPI int glhckInitialized(void);
GLHCKAPI int glhckInit(const int argc, char **argv);
GLHCKAPI void glhckTerminate(void);
GLHCKAPI void glhckFlush(void);
GLHCKAPI void glhckLogColor(int color);
GLHCKAPI void glhckSetGlobalPrecision(const glhckIndexType itype, const glhckVertexType vtype);
GLHCKAPI void glhckGetGlobalPrecision(glhckIndexType *outItype, glhckVertexType *outVtype);

/* handles */
GLHCKAPI const char* glhckHandleRepr(const glhckHandle handle);
GLHCKAPI const char* glhckHandleReprArray(const glhckHandle *handles, const size_t memb);
GLHCKAPI glhckType glhckHandleGetType(const glhckHandle handle);
GLHCKAPI unsigned int glhckHandleRef(const glhckHandle handle);
GLHCKAPI unsigned int glhckHandleRefPtr(const glhckHandle *handle);
GLHCKAPI unsigned int glhckHandleRelease(const glhckHandle handle);
GLHCKAPI unsigned int glhckHandleReleasePtr(const glhckHandle *handle);

/* display */
GLHCKAPI int glhckDisplayCreated(void);
GLHCKAPI int glhckDisplayCreate(const int width, const int height, const size_t renderer);
GLHCKAPI void glhckDisplayClose(void);
GLHCKAPI void glhckDisplayResize(const int width, const int height);

/* importer */
GLHCKAPI size_t glhckImporterCount(void);
GLHCKAPI const char* glhckImporterGetName(const size_t importer);

/* renderer */
GLHCKAPI size_t glhckRendererCount(void);
GLHCKAPI const glhckRendererContextInfo* glhckRendererGetContextInfo(const size_t renderer, const char **outName);
GLHCKAPI const glhckRendererFeatures* glhckRendererGetFeatures(void);

/* rendering */
GLHCKAPI void glhckRenderResize(const int width, const int height);
GLHCKAPI void glhckRenderViewport(const glhckRect *viewport);
GLHCKAPI void glhckRenderViewporti(const int x, const int y, const int width, const int height);
GLHCKAPI int glhckRenderGetWidth(void);
GLHCKAPI int glhckRenderGetHeight(void);
GLHCKAPI void glhckRenderStatePush(void);
GLHCKAPI void glhckRenderStatePush2D(const int width, const int height, const kmScalar near, const kmScalar far);
GLHCKAPI void glhckRenderStatePop(void);
GLHCKAPI unsigned int glhckRenderPassDefaults(void);
GLHCKAPI void glhckRenderPass(const unsigned int flags);
GLHCKAPI unsigned int glhckRenderGetPass(void);
GLHCKAPI void glhckRenderTime(const float time);
GLHCKAPI void glhckRenderClearColor(const glhckColor color);
GLHCKAPI void glhckRenderClearColoru(const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a);
GLHCKAPI glhckColor glhckRenderGetClearColor(void);
GLHCKAPI void glhckRenderClear(const unsigned int bufferBits);
GLHCKAPI void glhckRenderBlendFunc(const glhckBlendingMode blenda, const glhckBlendingMode blendb);
GLHCKAPI void glhckRenderGetBlendFunc(glhckBlendingMode *outBlenda, glhckBlendingMode *outBlendb);
GLHCKAPI void glhckRenderFrontFace(const glhckFaceOrientation orientation);
GLHCKAPI glhckFaceOrientation glhckRenderGetFrontFace(void);
GLHCKAPI void glhckRenderCullFace(const glhckCullFaceType face);
GLHCKAPI glhckCullFaceType glhckRenderGetCullFace(void);
GLHCKAPI void glhckRenderPassShader(const glhckHandle shader);
GLHCKAPI glhckHandle glhckRenderGetShader(void);
GLHCKAPI void glhckRenderFlip(const int flip);
GLHCKAPI int glhckRenderGetFlip(void);
GLHCKAPI void glhckRenderProjection(const kmMat4 *mat);
GLHCKAPI void glhckRenderProjectionOnly(const kmMat4 *mat);
GLHCKAPI void glhckRenderProjection2D(const int width, const int height, const kmScalar near, const kmScalar far);
GLHCKAPI const kmMat4* glhckRenderGetProjection(void);
GLHCKAPI void glhckRenderView(const kmMat4 *mat);
GLHCKAPI const kmMat4* glhckRenderGetView(void);
GLHCKAPI void glhckRenderWorldPosition(const kmVec3 *position);
GLHCKAPI void glhckRender(void);

/** XXX: fix with glhckRenderHandle? */
GLHCKAPI void glhckRenderObject(const glhckHandle object);
GLHCKAPI void glhckRenderText(const glhckHandle text);

/* frustum */
GLHCKAPI void glhckFrustumBuild(glhckFrustum *frustum, const kmMat4 *mvp);
GLHCKAPI void glhckFrustumRender(const glhckFrustum *frustum);
GLHCKAPI int glhckFrustumContainsPoint(const glhckFrustum *frustum, const kmVec3 *point);
GLHCKAPI kmScalar glhckFrustumContainsSphere(const glhckFrustum *frustum, const kmVec3 *point, const kmScalar radius);
GLHCKAPI glhckFrustumTestResult glhckFrustumContainsSphereEx(const glhckFrustum *frustum, const kmVec3 *point, const kmScalar radius);
GLHCKAPI int glhckFrustumContainsAABB(const glhckFrustum *frustum, const kmAABB *aabb);
GLHCKAPI glhckFrustumTestResult glhckFrustumContainsAABBEx(const glhckFrustum *frustum, const kmAABB *aabb);

/* cameras */
GLHCKAPI glhckHandle glhckCameraNew(void);
GLHCKAPI void glhckCameraUpdate(const glhckHandle camera);
GLHCKAPI void glhckCameraReset(const glhckHandle camera);
GLHCKAPI void glhckCameraProjection(const glhckHandle camera, const glhckProjectionType projectionType);
GLHCKAPI glhckProjectionType glhckCameraGetProjection(const glhckHandle camera);
GLHCKAPI const glhckFrustum* glhckCameraGetFrustum(const glhckHandle camera);
GLHCKAPI const kmMat4* glhckCameraGetViewMatrix(const glhckHandle camera);
GLHCKAPI const kmMat4* glhckCameraGetProjectionMatrix(const glhckHandle camera);
GLHCKAPI const kmMat4* glhckCameraGetVPMatrix(const glhckHandle camera);
GLHCKAPI void glhckCameraUpVector(const glhckHandle camera, const kmVec3 *upVector);
GLHCKAPI void glhckCameraFov(const glhckHandle camera, const kmScalar fov);
GLHCKAPI void glhckCameraRange(const glhckHandle camera, const kmScalar near, const kmScalar far);
GLHCKAPI void glhckCameraViewport(const glhckHandle camera, const glhckRect *viewport);
GLHCKAPI void glhckCameraViewporti(const glhckHandle camera, const int x, const int y, const int w, const int h);
GLHCKAPI const glhckRect* glhckCameraGetViewport(const glhckHandle camera);
GLHCKAPI const kmVec3* glhckCameraGetPosition(const glhckHandle camera);
GLHCKAPI void glhckCameraPosition(const glhckHandle camera, const kmVec3 *position);
GLHCKAPI void glhckCameraPositionf(const glhckHandle camera, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckCameraMove(const glhckHandle camera, const kmVec3 *move);
GLHCKAPI void glhckCameraMovef(const glhckHandle camera, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckCameraGetRotation(const glhckHandle camera);
GLHCKAPI void glhckCameraRotation(const glhckHandle camera, const kmVec3 *rotate);
GLHCKAPI void glhckCameraRotationf(const glhckHandle camera, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckCameraRotate(const glhckHandle camera, const kmVec3 *rotate);
GLHCKAPI void glhckCameraRotatef(const glhckHandle camera, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckCameraGetTarget(const glhckHandle camera);
GLHCKAPI void glhckCameraTarget(const glhckHandle camera, const kmVec3 *target);
GLHCKAPI void glhckCameraTargetf(const glhckHandle camera, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckCameraGetScale(const glhckHandle camera);
GLHCKAPI void glhckCameraScale(const glhckHandle camera, const kmVec3 *scale);
GLHCKAPI void glhckCameraScalef(const glhckHandle camera, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmRay3* glhckCameraCastRayFromPoint(const glhckHandle camera, const kmVec2 *point, kmRay3 *pOut);
GLHCKAPI const kmRay3* glhckCameraCastRayFromPointf(const glhckHandle camera, const kmScalar x, const kmScalar y, kmRay3 *pOut);

/* objects */
GLHCKAPI glhckHandle glhckObjectNew(void);
GLHCKAPI glhckHandle glhckObjectCopy(const glhckHandle src);
GLHCKAPI unsigned char glhckObjectGetParentAffection(const glhckHandle object);
GLHCKAPI void glhckObjectParentAffection(const glhckHandle object, const unsigned char affectionFlags);
GLHCKAPI glhckHandle glhckObjectParent(const glhckHandle object);
GLHCKAPI const glhckHandle* glhckObjectChildren(const glhckHandle object, size_t *outMemb);
GLHCKAPI void glhckObjectAddChild(const glhckHandle object, const glhckHandle child);
GLHCKAPI void glhckObjectRemoveChild(const glhckHandle object, const glhckHandle child);
GLHCKAPI void glhckObjectRemoveChildren(const glhckHandle object);
GLHCKAPI void glhckObjectRemoveFromParent(const glhckHandle object);
GLHCKAPI void glhckObjectMaterial(const glhckHandle object, const glhckHandle material);
GLHCKAPI glhckHandle glhckObjectGetMaterial(const glhckHandle object);

/* object animation */
GLHCKAPI int glhckObjectInsertBones(const glhckHandle object, const glhckHandle *bones, const size_t memb);
GLHCKAPI const glhckHandle* glhckObjectBones(const glhckHandle object, size_t *outMemb);
GLHCKAPI glhckHandle glhckObjectGetBone(const glhckHandle object, const char *name);
GLHCKAPI int glhckObjectInsertSkinBones(const glhckHandle object, const glhckHandle *skinBones, const size_t memb);
GLHCKAPI const glhckHandle* glhckObjectSkinBones(const glhckHandle object, size_t *outMemb);
GLHCKAPI glhckHandle glhckObjectGetSkinBone(const glhckHandle object, const char *name);
GLHCKAPI int glhckObjectInsertAnimations(const glhckHandle object, const glhckHandle *animations, const size_t memb);
GLHCKAPI const glhckHandle* glhckObjectAnimations(const glhckHandle object, size_t *outMemb);

/* object drawing options */
GLHCKAPI void glhckObjectRoot(const glhckHandle object, const int root);
GLHCKAPI int glhckObjectGetRoot(const glhckHandle object);
GLHCKAPI void glhckObjectVertexColors(const glhckHandle object, const int vertexColors);
GLHCKAPI int glhckObjectGetVertexColors(const glhckHandle object);
GLHCKAPI void glhckObjectCull(const glhckHandle object, const int cull);
GLHCKAPI int glhckObjectGetCull(const glhckHandle object);
GLHCKAPI void glhckObjectDepth(const glhckHandle object, const int depth);
GLHCKAPI int glhckObjectGetDepth(const glhckHandle object);
GLHCKAPI void glhckObjectDrawAABB(const glhckHandle object, const int drawAABB);
GLHCKAPI int glhckObjectGetDrawAABB(const glhckHandle object);
GLHCKAPI void glhckObjectDrawOBB(const glhckHandle object, const int drawOBB);
GLHCKAPI int glhckObjectGetDrawOBB(const glhckHandle object);
GLHCKAPI void glhckObjectDrawSkeleton(const glhckHandle object, const int drawSkeleton);
GLHCKAPI int glhckObjectGetDrawSkeleton(const glhckHandle object);
GLHCKAPI void glhckObjectDrawWireframe(const glhckHandle object, const int drawWireframe);
GLHCKAPI int glhckObjectGetDrawWireframe(const glhckHandle object);

/* object control */
GLHCKAPI const kmAABB* glhckObjectGetOBB(const glhckHandle object);
GLHCKAPI const kmAABB* glhckObjectGetOBBWithChildren(const glhckHandle object);
GLHCKAPI const kmAABB* glhckObjectGetAABB(const glhckHandle object);
GLHCKAPI const kmAABB* glhckObjectGetAABBWithChildren(const glhckHandle object);
GLHCKAPI const kmMat4* glhckObjectGetMatrix(const glhckHandle object);
GLHCKAPI const kmVec3* glhckObjectGetPosition(const glhckHandle object);
GLHCKAPI void glhckObjectPosition(const glhckHandle object, const kmVec3 *position);
GLHCKAPI void glhckObjectPositionf(const glhckHandle object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckObjectMove(const glhckHandle object, const kmVec3 *move);
GLHCKAPI void glhckObjectMovef(const glhckHandle object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetRotation(const glhckHandle object);
GLHCKAPI void glhckObjectRotation(const glhckHandle object, const kmVec3 *rotate);
GLHCKAPI void glhckObjectRotationf(const glhckHandle object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckObjectRotate(const glhckHandle object, const kmVec3 *rotate);
GLHCKAPI void glhckObjectRotatef(const glhckHandle object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetTarget(const glhckHandle object);
GLHCKAPI void glhckObjectTarget(const glhckHandle object, const kmVec3 *target);
GLHCKAPI void glhckObjectTargetf(const glhckHandle object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetScale(const glhckHandle object);
GLHCKAPI void glhckObjectScale(const glhckHandle object, const kmVec3 *scale);
GLHCKAPI void glhckObjectScalef(const glhckHandle object, const kmScalar x, const kmScalar y, const kmScalar z);

/* object geometry */
GLHCKAPI int glhckObjectInsertVertices(const glhckHandle object, const glhckVertexType type, const glhckImportVertexData *vertices, const int memb);
GLHCKAPI int glhckObjectInsertIndices(const glhckHandle object, const glhckIndexType type, const glhckImportIndexData *indices, const int memb);
GLHCKAPI void glhckObjectUpdate(const glhckHandle object);
GLHCKAPI void glhckObjectGeometry(const glhckHandle object, const glhckHandle geometry);
GLHCKAPI glhckHandle glhckObjectGetGeometry(const glhckHandle object);
GLHCKAPI int glhckObjectPickTextureCoordinatesWithRay(const glhckHandle object, const kmRay3 *ray, kmVec2 *outCoords);

/* pre-defined geometry */
GLHCKAPI glhckHandle glhckModelNew(const char *file, const kmScalar size, const glhckPostProcessModelParameters *postParams);
GLHCKAPI glhckHandle glhckModelNewEx(const char *file, const kmScalar size, const glhckPostProcessModelParameters *postParams, const glhckIndexType itype, const glhckVertexType vtype);
GLHCKAPI glhckHandle glhckCubeNew(const kmScalar size);
GLHCKAPI glhckHandle glhckCubeNewEx(const kmScalar size, const glhckIndexType itype, const glhckIndexType vtype);
GLHCKAPI glhckHandle glhckSphereNew(const kmScalar size);
GLHCKAPI glhckHandle glhckSphereNewEx(const kmScalar size, const glhckIndexType itype, const glhckIndexType vtype);
GLHCKAPI glhckHandle glhckEllipsoidNew(const kmScalar rx, const kmScalar ry, const kmScalar rz);
GLHCKAPI glhckHandle glhckEllipsoidNewEx(const kmScalar rx, const kmScalar ry, const kmScalar rz, const glhckIndexType itype, const glhckIndexType vtype);
GLHCKAPI glhckHandle glhckPlaneNew(const kmScalar width, const kmScalar height);
GLHCKAPI glhckHandle glhckPlaneNewEx(const kmScalar width, const kmScalar height, const glhckIndexType itype, const glhckIndexType vtype);
GLHCKAPI glhckHandle glhckSpriteNew(const glhckHandle texture, const kmScalar width, const kmScalar height);
GLHCKAPI glhckHandle glhckSpriteNewFromFile(const char *file, const kmScalar width, const kmScalar height, const glhckPostProcessImageParameters *postParams, const glhckTextureParameters *params);

/* bones */
GLHCKAPI glhckHandle glhckBoneNew(void);
GLHCKAPI void glhckBoneName(const glhckHandle bone, const char *name);
GLHCKAPI const char* glhckBoneGetName(const glhckHandle bone);
GLHCKAPI void glhckBoneParentBone(const glhckHandle bone, const glhckHandle parent);
GLHCKAPI glhckHandle glhckBoneGetParentBone(const glhckHandle bone);
GLHCKAPI void glhckBoneTransformationMatrix(const glhckHandle bone, const kmMat4 *transformationMatrix);
GLHCKAPI void glhckBoneOffsetMatrix(const glhckHandle bone, const kmMat4 *offsetMatrix);
GLHCKAPI const kmMat4* glhckBoneGetOffsetMatrix(const glhckHandle bone);
GLHCKAPI const kmMat4* glhckBoneGetTransformationMatrix(const glhckHandle bone);
GLHCKAPI const kmMat4* glhckBoneGetTransformedMatrix(const glhckHandle bone);
GLHCKAPI const kmMat4* glhckBoneGetPoseMatrix(const glhckHandle bone);
GLHCKAPI void glhckBoneGetPositionRelativeOnObject(const glhckHandle bone, const glhckHandle object, kmVec3 *outPosition);
GLHCKAPI void glhckBoneGetPositionAbsoluteOnObject(const glhckHandle bone, const glhckHandle object, kmVec3 *outPosition);

/* skin bones (for skinning \o/) */
GLHCKAPI glhckHandle glhckSkinBoneNew(void);
GLHCKAPI void glhckSkinBoneBone(const glhckHandle skinBone, const glhckHandle bone);
GLHCKAPI glhckHandle glhckSkinBoneGetBone(const glhckHandle skinBone);
GLHCKAPI const char* glhckSkinBoneGetName(const glhckHandle skinBone);
GLHCKAPI int glhckSkinBoneInsertWeights(const glhckHandle skinBone, const glhckVertexWeight *weights, size_t memb);
GLHCKAPI const glhckVertexWeight* glhckSkinBoneWeights(const glhckHandle skinBone, size_t *outMemb);
GLHCKAPI void glhckSkinBoneOffsetMatrix(const glhckHandle skinBone, const kmMat4 *offsetMatrix);
GLHCKAPI const kmMat4* glhckSkinBoneGetOffsetMatrix(const glhckHandle skinBone);

/* animation nodes */
GLHCKAPI glhckHandle glhckAnimationNodeNew(void);
GLHCKAPI void glhckAnimationNodeBoneName(const glhckHandle node, const char *name);
GLHCKAPI const char* glhckAnimationNodeGetBoneName(const glhckHandle node);
GLHCKAPI int glhckAnimationNodeInsertTranslations(const glhckHandle node, const glhckAnimationVectorKey *keys, const size_t memb);
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeTranslations(const glhckHandle node, size_t *outMemb);
GLHCKAPI int glhckAnimationNodeInsertRotations(const glhckHandle node, const glhckAnimationQuaternionKey *keys, const size_t memb);
GLHCKAPI const glhckAnimationQuaternionKey* glhckAnimationNodeRotations(const glhckHandle node, size_t *outMemb);
GLHCKAPI int glhckAnimationNodeInsertScalings(const glhckHandle node, const glhckAnimationVectorKey *keys, const size_t memb);
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeScalings(const glhckHandle node, size_t *outMemb);

/* animations */
GLHCKAPI glhckHandle glhckAnimationNew(void);
GLHCKAPI void glhckAnimationName(const glhckHandle animation, const char *name);
GLHCKAPI const char* glhckAnimationGetName(const glhckHandle animation);
GLHCKAPI void glhckAnimationTicksPerSecond(const glhckHandle animation, const float ticksPerSecond);
GLHCKAPI float glhckAnimationGetTicksPerSecond(const glhckHandle animation);
GLHCKAPI void glhckAnimationDuration(const glhckHandle animation, const float duration);
GLHCKAPI float glhckAnimationGetDuration(const glhckHandle animation);
GLHCKAPI int glhckAnimationInsertNodes(const glhckHandle animation, const glhckHandle *nodes, const size_t memb);
GLHCKAPI const glhckHandle* glhckAnimationNodes(const glhckHandle animation, size_t *outMemb);

/* animator */
GLHCKAPI glhckHandle glhckAnimatorNew(void);
GLHCKAPI void glhckAnimatorAnimation(const glhckHandle animator, const glhckHandle animation);
GLHCKAPI glhckHandle glhckAnimatorGetAnimation(const glhckHandle animator);
GLHCKAPI int glhckAnimatorInsertBones(const glhckHandle animator, const glhckHandle *bones, const size_t memb);
GLHCKAPI const glhckHandle* glhckAnimatorBones(const glhckHandle animator, size_t *outMemb);
GLHCKAPI void glhckAnimatorTransform(const glhckHandle animator, const glhckHandle object);
GLHCKAPI void glhckAnimatorUpdate(const glhckHandle animator, const float playTime);

/* text */
GLHCKAPI glhckHandle glhckTextNew(const int cacheWidth, const int cacheHeight);
GLHCKAPI void glhckTextFontFree(const glhckHandle text, const glhckFont font);
GLHCKAPI void glhckTextFlushCache(const glhckHandle text);
GLHCKAPI void glhckTextGetMetrics(const glhckHandle text, const glhckFont font, const float size, float *outAscender, float *outDescender, float *outLineHeight);
GLHCKAPI void glhckTextGetMinMax(const glhckHandle text, const glhckFont font, const float size, const char *s, kmVec2 *outMin, kmVec2 *outMax);
GLHCKAPI void glhckTextColor(const glhckHandle text, const glhckColor color);
GLHCKAPI void glhckTextColoru(const glhckHandle text, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a);
GLHCKAPI glhckColor glhckTextGetColor(const glhckHandle text);
GLHCKAPI glhckFont glhckTextFontNewKakwafont(const glhckHandle text, int *outNativeSize);
GLHCKAPI glhckFont glhckTextFontNewFromMemory(const glhckHandle text, const void *data, const size_t size);
GLHCKAPI glhckFont glhckTextFontNew(const glhckHandle text, const char *file);
GLHCKAPI glhckFont glhckTextFontNewFromTexture(const glhckHandle text, const glhckHandle texture, const int ascent, const int descent, const int lineGap);
GLHCKAPI glhckFont glhckTextFontNewFromImage(const glhckHandle text, const char *file, const int ascent, const int descent, const int lineGap);
GLHCKAPI void glhckTextGlyphNew(const glhckHandle text, const glhckFont font, const char *s, const short size, const short base, const int x, const int y, const int w, const int h, const float xoff, const float yoff, const float xadvance);
GLHCKAPI void glhckTextStash(const glhckHandle text, const glhckFont font, const float size, const float x, const float y, const char *s, float *outWidth);
GLHCKAPI void glhckTextClear(const glhckHandle text);
GLHCKAPI void glhckTextShader(const glhckHandle text, const glhckHandle shader);
GLHCKAPI glhckHandle glhckTextGetShader(const glhckHandle text);
GLHCKAPI glhckHandle glhckTextRTT(const glhckHandle text, const glhckFont font, const float size, const char *s, const glhckTextureParameters *params);
GLHCKAPI glhckHandle glhckTextPlane(const glhckHandle text, const glhckFont font, const float size, const char *s, const glhckTextureParameters *params);

/* materials */
GLHCKAPI glhckHandle glhckMaterialNew(const glhckHandle texture);
GLHCKAPI void glhckMaterialOptions(const glhckHandle material, const unsigned int flags);
GLHCKAPI unsigned int glhckMaterialGetOptions(const glhckHandle material);
GLHCKAPI void glhckMaterialShader(const glhckHandle material, const glhckHandle shader);
GLHCKAPI glhckHandle glhckMaterialGetShader(const glhckHandle material);
GLHCKAPI void glhckMaterialTexture(const glhckHandle material, const glhckHandle texture);
GLHCKAPI glhckHandle glhckMaterialGetTexture(const glhckHandle material);
GLHCKAPI void glhckMaterialTextureScale(const glhckHandle material, const kmVec2 *scale);
GLHCKAPI void glhckMaterialTextureScalef(const glhckHandle material, const kmScalar x, const kmScalar y);
GLHCKAPI const kmVec2* glhckMaterialGetTextureScale(const glhckHandle material);
GLHCKAPI void glhckMaterialTextureOffset(const glhckHandle material, const kmVec2 *offset);
GLHCKAPI void glhckMaterialTextureOffsetf(const glhckHandle material, const kmScalar x, const kmScalar y);
GLHCKAPI const kmVec2* glhckMaterialGetTextureOffset(const glhckHandle material);
GLHCKAPI void glhckMaterialTextureRotationf(const glhckHandle material, const kmScalar degrees);
GLHCKAPI kmScalar glhckMaterialGetTextureRotation(const glhckHandle material);
GLHCKAPI void glhckMaterialTextureTransform(const glhckHandle material, const glhckRect *transformed, const short degrees);
GLHCKAPI void glhckMaterialShininess(const glhckHandle material, const float shininess);
GLHCKAPI float glhckMaterialGetShininess(const glhckHandle material);
GLHCKAPI void glhckMaterialBlendFunc(const glhckHandle material, const glhckBlendingMode blenda, const glhckBlendingMode blendb);
GLHCKAPI void glhckMaterialGetBlendFunc(const glhckHandle material, glhckBlendingMode *outBlenda, glhckBlendingMode *outBlendb);
GLHCKAPI void glhckMaterialAmbient(const glhckHandle material, const glhckColor ambient);
GLHCKAPI void glhckMaterialAmibentu(const glhckHandle material, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a);
GLHCKAPI glhckColor glhckMaterialGetAmbient(const glhckHandle material);
GLHCKAPI void glhckMaterialDiffuse(const glhckHandle material, const glhckColor diffuse);
GLHCKAPI void glhckMaterialDiffuseu(const glhckHandle material, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a);
GLHCKAPI glhckColor glhckMaterialGetDiffuse(const glhckHandle material);
GLHCKAPI void glhckMaterialEmissive(const glhckHandle material, const glhckColor emissive);
GLHCKAPI void glhckMaterialEmissiveu(const glhckHandle material, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a);
GLHCKAPI glhckColor glhckMaterialGetEmissive(const glhckHandle material);
GLHCKAPI void glhckMaterialSpecular(const glhckHandle material, const glhckColor specular);
GLHCKAPI void glhckMaterialSpecularu(const glhckHandle material, const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a);
GLHCKAPI glhckColor glhckMaterialGetSpecular(const glhckHandle material);

/* textures */
GLHCKAPI glhckHandle glhckTextureNew(void);
GLHCKAPI glhckHandle glhckTextureNewFromFile(const char *file, const glhckPostProcessImageParameters *postParams, const glhckTextureParameters *params);
GLHCKAPI void glhckTextureGetInformation(const glhckHandle texture, glhckTextureTarget *target, int *width, int *height, int *depth, int *border, glhckTextureFormat *format, glhckDataType *type);
GLHCKAPI int glhckTextureGetTarget(const glhckHandle texture);
GLHCKAPI int glhckTextureGetWidth(const glhckHandle texture);
GLHCKAPI int glhckTextureGetHeight(const glhckHandle texture);
GLHCKAPI int glhckTextureCreate(const glhckHandle texture, const glhckTextureTarget target, const int level, const int width, const int height, const int depth, const int border, const glhckTextureFormat format, const glhckDataType type, const int size, const void *data);
GLHCKAPI int glhckTextureRecreate(const glhckHandle texture, const glhckTextureFormat format, const glhckDataType type, const int size, const void *data);
GLHCKAPI void glhckTextureFill(const glhckHandle texture, const int level, const int x, const int y, const int z, const int width, const int height, const int depth, const glhckTextureFormat format, const glhckDataType type, const int size, const void *data);
GLHCKAPI void glhckTextureFillFrom(const glhckHandle texture, const int level, const int sx, const int sy, const int sz, const int x, const int y, const int z, const int width, const int height, const int depth, const glhckTextureFormat format, const glhckDataType type, const int size, const void *data);
GLHCKAPI void glhckTextureParameter(const glhckHandle texture, const glhckTextureParameters *params);
GLHCKAPI int glhckTextureSave(const glhckHandle texture, const char *path);
GLHCKAPI glhckHandle glhckTextureCurrentForTarget(glhckTextureTarget target);
GLHCKAPI void glhckTextureActive(const unsigned int index);
GLHCKAPI void glhckTextureBind(const glhckHandle texture);
GLHCKAPI void glhckTextureUnbind(const glhckTextureTarget target);
GLHCKAPI void* glhckTextureCompress(const glhckTextureCompression compression, const int width, const int height, const glhckTextureFormat format, const glhckDataType type, const void *data, int *inOutSize, glhckTextureFormat *outFormat, glhckDataType *outType);

/* texture parameters */
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultParameters(void);
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultLinearParameters(void);
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultSpriteParameters(void);

/* texture atlases */
GLHCKAPI glhckHandle glhckAtlasNew(void);
GLHCKAPI int glhckAtlasInsertTexture(const glhckHandle atlas, const glhckHandle texture);
GLHCKAPI glhckHandle glhckAtlasGetTexture(const glhckHandle atlas);
GLHCKAPI int glhckAtlasPack(const glhckHandle atlas, const glhckTextureFormat format, const int powerOfTwo, const int border, const glhckTextureParameters *params);
GLHCKAPI glhckHandle glhckAtlasGetTextureByIndex(const glhckHandle atlas, const unsigned int index);
GLHCKAPI int glhckAtlasGetTransform(const glhckHandle atlas, const glhckHandle texture, glhckRect *outTransformed, short *outDegrees);
GLHCKAPI int glhckAtlasTransformCoordinates(const glhckHandle atlas, const glhckHandle texture, const kmVec2 *in, kmVec2 *out);

/* renderbuffer objects */
GLHCKAPI glhckHandle glhckRenderbufferNew(const int width, const int height, const glhckTextureFormat format);
GLHCKAPI glhckHandle glhckRenderbufferCurrent(void);
GLHCKAPI void glhckRenderbufferBind(const glhckHandle renderbuffer);

/* framebuffer objects */
GLHCKAPI glhckHandle glhckFramebufferNew(const glhckFramebufferTarget target);
GLHCKAPI glhckHandle glhckFramebufferCurrentForTarget(const glhckFramebufferTarget target);
GLHCKAPI glhckFramebufferTarget glhckFramebufferGetTarget(const glhckHandle framebuffer);
GLHCKAPI void glhckFramebufferBind(const glhckHandle framebuffer);
GLHCKAPI void glhckFramebufferUnbind(const glhckFramebufferTarget target);
GLHCKAPI void glhckFramebufferBegin(const glhckHandle framebuffer);
GLHCKAPI void glhckFramebufferEnd(const glhckHandle framebuffer);
GLHCKAPI void glhckFramebufferRect(const glhckHandle framebuffer, const glhckRect *rect);
GLHCKAPI void glhckFramebufferRecti(const glhckHandle framebuffer, const int x, const int y, const int w, const int h);
GLHCKAPI const glhckRect* glhckFramebufferGetRect(const glhckHandle framebuffer);
GLHCKAPI int glhckFramebufferAttachTexture(const glhckHandle framebuffer, const glhckHandle texture, const glhckFramebufferAttachmentType attachment);
GLHCKAPI int glhckFramebufferAttachRenderbuffer(const glhckHandle framebuffer, const glhckHandle renderbuffer, const glhckFramebufferAttachmentType attachment);

/* hardware buffer objects */
GLHCKAPI glhckHandle glhckHwBufferNew(void);
GLHCKAPI const char* glhckHwBufferGetName(const glhckHandle hwbufer);
GLHCKAPI int glhckHwBufferGetSize(const glhckHandle handle);
GLHCKAPI glhckHandle glhckHwBufferCurrentForTarget(const glhckHwBufferTarget target);
GLHCKAPI glhckHwBufferTarget glhckHwBufferGetTarget(const glhckHandle hwbuffer);
GLHCKAPI void glhckHwBufferBind(const glhckHandle hwbuffer);
GLHCKAPI void glhckHwBufferUnbind(const glhckHwBufferTarget target);
GLHCKAPI glhckHwBufferTarget glhckHwBufferGetType(const glhckHandle hwbuffer);
GLHCKAPI glhckHwBufferStoreType glhckHwBufferGetStoreType(const glhckHandle hwbuffer);
GLHCKAPI void glhckHwBufferCreate(const glhckHandle hwbuffer, const glhckHwBufferTarget target, const int size, const void *data, const glhckHwBufferStoreType usage);
GLHCKAPI void glhckHwBufferBindBase(const glhckHandle hwbuffer, const unsigned int index);
GLHCKAPI void glhckHwBufferBindRange(const glhckHandle hwbuffer, const unsigned int index, const int offset, const int size);
GLHCKAPI void glhckHwBufferFill(const glhckHandle hwbuffer, const int offset, const int size, const void *data);
GLHCKAPI void* glhckHwBufferMap(const glhckHandle hwbuffer, const glhckHwBufferAccessType access);
GLHCKAPI void glhckHwBufferUnmap(const glhckHandle hwbuffer);

/* uniform buffer specific */
GLHCKAPI int glhckHwBufferCreateUniformBufferFromShader(const glhckHandle hwbuffer, const glhckHandle shader, const char *uboName, const glhckHwBufferStoreType usage);
GLHCKAPI int glhckHwBufferAppendInformationFromShader(const glhckHandle hwbuffer, const glhckHandle shader);
GLHCKAPI const void* glhckHwBufferGetUniform(const glhckHandle hwbuffer, const char *uniform);
GLHCKAPI void glhckHwBufferFillUniform(const glhckHandle hwbuffer, const void *uniform, const int size, const void *data);

/* shaders */
GLHCKAPI unsigned int glhckCompileShaderObject(const glhckShaderType type, const char *effectKey, const char *contentsFromMemory);
GLHCKAPI void glhckDeleteShaderObject(const unsigned int shaderObject);
GLHCKAPI glhckHandle glhckShaderNew(const char *vertexEffect, const char *fragmentEffect, const char *contentsFromMemory);
GLHCKAPI glhckHandle glhckShaderNewWithShaderObjects(const unsigned int vertexShader, const unsigned int fragmentShader);
GLHCKAPI void glhckShaderBind(const glhckHandle shader);
GLHCKAPI const void* glhckShaderGetUniform(const glhckHandle shader, const char *uniform);
GLHCKAPI void glhckShaderUniform(const glhckHandle shader, const void *uniform, const int count, const void *value);
GLHCKAPI int glhckShaderAttachHwBuffer(const glhckHandle shader, const glhckHandle hwbuffer, const char *name, const unsigned int index);

/* trace && debug */
GLHCKAPI void glhckDebugHook(const glhckDebugHookFunc func);
GLHCKAPI void glhckMemoryGraph(void);
GLHCKAPI void glhckRenderPrintObjectQueue(void);
GLHCKAPI void glhckRenderPrintTextureQueue(void);

/* vertexdata geometry */
GLHCKAPI glhckHandle glhckGeometryNew(void);
GLHCKAPI void glhckGeometryCalculateBB(const glhckHandle handle, kmAABB *outBB);
GLHCKAPI glhckGeometry* glhckGeometryGetStruct(const glhckHandle handle);
GLHCKAPI int glhckGeometryInsertVertices(const glhckHandle handle, const glhckVertexType type, const void *data, const int memb);
GLHCKAPI int glhckGeometryInsertIndices(const glhckHandle handle, const glhckIndexType type, const void *data, const int memb);

/* collisions
 * XXX: incomplete */
GLHCKAPI glhckHandle glhckCollisionWorldNew(const void *userData);
GLHCKAPI void glhckCollisionWorldFree(const glhckHandle collisionWorld);
GLHCKAPI void* glhckCollisionPrimitiveGetUserData(const glhckHandle primitive);
GLHCKAPI glhckHandle glhckCollisionWorldAddAABBRef(const glhckHandle collisionWorld, const kmAABB *aabb, void *userData);
GLHCKAPI glhckHandle glhckCollisionWorldAddAABB(const glhckHandle collisionWorld, const kmAABB *aabb, void *userData);
GLHCKAPI void glhckCollisionWorldRemovePrimitive(const glhckHandle collisionWorld, glhckHandle primitive);
GLHCKAPI unsigned int glhckCollisionWorldCollideAABB(const glhckHandle collisionWorld, const kmAABB *aabb, const glhckCollisionInData *data);

#ifdef __cplusplus
}
#endif

#endif /* __glhck_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
