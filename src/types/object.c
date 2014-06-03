#include "object.h"

#include <glhck/glhck.h>
#include <assert.h>  /* for assert */
#include <float.h>   /* for FLT_MAX */

#include "system/tls.h"
#include "pool/pool.h"
#include "array/array.h"
#include "handle.h"
#include "trace.h"
#include "view.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT

enum pool {
   $file, // file handle
   $name, // name handle
   $view, // view handle
   $material, // material handle
   $parent, // object handle
   $childs, // list handle
   $bones, // list handle
   $skinBones, // list handle
   $animations, // list handle
   $geometry, // geometry handle
   $flags, // unsigned char (glhckObjectFlags)
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // file
   sizeof(glhckHandle), // name
   sizeof(glhckHandle), // view
   sizeof(glhckHandle), // material
   sizeof(glhckHandle), // parent
   sizeof(glhckHandle), // childs
   sizeof(glhckHandle), // bones
   sizeof(glhckHandle), // skinBones
   sizeof(glhckHandle), // animations
   sizeof(glhckHandle), // geometry
   sizeof(unsigned char), // flags
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static void setFlag(const glhckHandle handle, const enum glhckObjectFlags flag, const int enabled)
{
   CALL(2, "%s, %d", glhckHandleRepr(handle), enabled);
   assert(handle > 0);

   if (enabled) {
      *(unsigned char*)get($flags, handle) |= flag;
   } else {
      *(unsigned char*)get($flags, handle) &= ~flag;
   }

#if 0
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectVertexColors, vertexColors);
#endif
}

static int getFlag(const glhckHandle handle, const enum glhckObjectFlags flag)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   assert(handle > 0);
   const int enabled = (*(unsigned char*)get($flags, handle) & flag);
   RET(2, "%d", enabled);
   return enabled;
}

GLHCKAPI glhckHandle glhckObjectGetView(const glhckHandle handle)
{
   return *(glhckHandle*)get($view, handle);
}

static void setView(const glhckHandle handle, const glhckHandle view)
{
   setHandle($view, handle, view);
}

static int getDirty(const glhckHandle handle)
{
   return glhckViewGetDirty(glhckObjectGetView(handle));
}

static void updateMatrix(const glhckHandle handle)
{
   _glhckObjectUpdateMatrixMany(&handle, 1);
}

void _glhckObjectUpdateMatrixMany(const glhckHandle *handles, const size_t memb)
{
   for (size_t i = 0; i < memb; ++i) {
      if (!getDirty(handles[i]))
         continue;

      glhckHandle parent = glhckObjectParent(handles[i]);
      const glhckHandle view = glhckObjectGetView(handles[i]);
      const glhckHandle parentView = (parent ? glhckObjectGetView(parent) : 0);

      parent = handles[i];
      while ((parent = glhckObjectParent(parent)) && getDirty(parent))
         updateMatrix(parent);

      glhckViewUpdateMany(&view, &parentView, 1);
   }
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   releaseHandle($view, handle);
   releaseHandle($name, handle);
   releaseHandle($file, handle);
}

/***
 * public api
 ***/

/* \brief new object */
GLHCKAPI glhckHandle glhckObjectNew(void)
{
   TRACE(0);

   glhckHandle handle = 0, view = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_OBJECT, pools, pool_sizes, POOL_LAST, destructor, NULL)))
      goto fail;

   if (!(view = glhckViewNew()))
      goto fail;

   setView(handle, view);
   glhckHandleRelease(view);

   glhckObjectScalef(handle, 1.0f, 1.0f, 1.0f);

   /* default object settings */
   glhckObjectCull(handle, 1);
   glhckObjectDepth(handle, 1);

   /* default affection flags */
   glhckObjectParentAffection(handle, GLHCK_AFFECT_TRANSLATION | GLHCK_AFFECT_ROTATION);

   /* update matrix and go */
   updateMatrix(handle);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   IFDO(glhckHandleRelease, view);
   RET(0, "%s", glhckHandleRepr(handle));
   return 0;
}

GLHCKAPI glhckHandle glhckObjectCopy(const glhckHandle src)
{
   CALL(0, "%s", glhckHandleRepr(src));

   glhckHandle handle = 0;
   if (!(handle = glhckObjectNew()))
      goto fail;

   glhckObjectPosition(handle, glhckObjectGetPosition(src));
   glhckObjectRotation(handle, glhckObjectGetRotation(src));
   glhckObjectScale(handle, glhckObjectGetScale(src));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(handle));
   return 0;
}

/* \brief is object treated as root? */
GLHCKAPI int glhckObjectIsRoot(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_ROOT);
}

/* \brief make object as root, or demote it */
GLHCKAPI void glhckObjectMakeRoot(const glhckHandle handle, const int root)
{
   setFlag(handle, GLHCK_OBJECT_ROOT, root);
}

/* \brief get object's affection flags */
GLHCKAPI unsigned char glhckObjectGetParentAffection(const glhckHandle handle)
{
   return glhckViewGetParentAffection(glhckObjectGetView(handle));
}

/* \brief set object's affection flags */
GLHCKAPI void glhckObjectParentAffection(const glhckHandle handle, const unsigned char affectionFlags)
{
   // chckArrayIterCall(object->childs, glhckObjectParentAffection, affectionFlags);
   return glhckViewParentAffection(glhckObjectGetView(handle), affectionFlags);
}

/* \brief get object's parent */
GLHCKAPI glhckHandle glhckObjectParent(const glhckHandle handle)
{
   return *(glhckHandle*)get($parent, handle);
}

/* \brief get object's children */
GLHCKAPI const glhckHandle* glhckObjectChildren(const glhckHandle handle, size_t *outMemb)
{
   return getList($childs, handle, outMemb);
}

/* \brief add child object */
GLHCKAPI void glhckObjectAddChild(const glhckHandle handle, const glhckHandle child)
{
   CALL(0, "%s, %s", glhckHandleRepr(handle), glhckHandleRepr(child));
   assert(handle > 0 && child > 0 && handle != child);

   if (glhckObjectParent(child) == handle)
      return;

   glhckHandle list = *(glhckHandle*)get($childs, handle);
   if (!list) {
      if (!(list = glhckListNew(1, sizeof(glhckHandle))))
         return;
   }

   if (!glhckListAdd(list, &child))
      return;

   glhckHandleRef(child);

   if (glhckObjectParent(child))
      glhckObjectRemoveChild(glhckObjectParent(child), child);

   set($parent, child, &handle);
   set($childs, handle, &list);
}

/* \brief remove child object */
GLHCKAPI void glhckObjectRemoveChild(const glhckHandle handle, const glhckHandle child)
{
   CALL(0, "%s, %s", glhckHandleRepr(handle), glhckHandleRepr(child));
   assert(handle > 0 && child > 0 && handle != child);

   if (glhckObjectParent(child) != handle)
      return;

   size_t memb;
   const glhckHandle *children = glhckObjectChildren(handle, &memb);
   for (size_t i = 0; i < memb; ++i) {
      if (children[i] == child) {
         const glhckHandle list = *(glhckHandle*)get($childs, handle);
         glhckListRemove(list, i);
         break;
      }
   }

   set($parent, child, (glhckHandle[]){0});
   glhckHandleRelease(child);

   if (memb == 1)
      releaseHandle($childs, handle);
}

/* \brief remove children objects */
GLHCKAPI void glhckObjectRemoveChildren(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   size_t memb;
   const glhckHandle *children = glhckObjectChildren(handle, &memb);
   for (size_t i = 0; i < memb; ++i) {
      set($parent, children[i], (glhckHandle[]){0});
      glhckHandleRelease(children[i]);
   }

   releaseHandle($childs, handle);
}

/* \brief remove object from parent */
GLHCKAPI void glhckObjectRemoveFromParent(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   if (!glhckObjectParent(handle))
      return;

   glhckObjectRemoveChild(glhckObjectParent(handle), handle);
}


/* \brief set material to object */
GLHCKAPI void glhckObjectMaterial(const glhckHandle handle, const glhckHandle material)
{
   CALL(2, "%s, %s", glhckHandleRepr(handle), glhckHandleRepr(material));
   assert(handle > 0);

   if (!setHandle($material, handle, material))
      return;

#if 0
   if (material) {
      const glhckHandle texture = glhckMaterialGetTexture(material);
      const glhckGeometry *geometry = glhckObjectGetGeometry(handle);
      if (texture &&  geometry && geometry->textureRange > 1) {
         if (glhckTextureGetWidth(texture) > geometry->textureRange || glhckTextureGetHeight(texture) > geometry->textureRange) {
            DEBUG(GLHCK_DBG_WARNING, "Texture dimensions are above the maximum precision of object's vertexdata (%zu:%u)",
                  GLHCKVT(geometry->vertexType)->max[2], geometry->textureRange);
         }
      }
   }
#endif

#if 0
   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectMaterial, material);
#endif
}

/* \brief get material from object */
GLHCKAPI glhckHandle glhckObjectGetMaterial(const glhckHandle handle)
{
   return *(glhckHandle*)get($material, handle);
}

#if 0

/* \brief add object to draw queue */
GLHCKAPI void glhckObjectDraw(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* insert to draw queue, referenced until glhckRender */
   if (chckArrayAdd(GLHCKRD()->objects, object))
      glhckObjectRef(object);

   /* insert texture to drawing queue? */
   if (object->material && object->material->texture) {
      /* insert object's texture to textures queue, referenced until glhckRender */
      if (chckArrayAdd(GLHCKRD()->textures, object->material->texture))
         glhckTextureRef(object->material->texture);
   }

   /* draw childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectDraw);
}

#endif

GLHCKAPI glhckHandle glhckObjectGetParent(const glhckHandle handle)
{
   return *(glhckHandle*)get($parent, handle);
}

GLHCKAPI void glhckObjectRenderMany(const glhckHandle *handles, const size_t memb)
{
   CALL(2, "%s", glhckHandleReprArray(handles, memb));

   // updateMatrixMany(handles, memb);

   // glhckRenderObjectMany(handles, memb);
   // _glhckRendererRender(handles, memb);

}

GLHCKAPI void glhckObjectRender(const glhckHandle handle)
{
   glhckObjectRenderMany((glhckHandle[]){handle}, 1);
}

#if 0

/* \brief render object and its children */
GLHCKAPI void glhckObjectRenderAll(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   glhckObjectRender(object);

   if (object->childs)
      chckArrayIterCall(object->childs, glhckObjectRenderAll);
}

#endif

/* \brief set whether object should use vertex colors */
GLHCKAPI void glhckObjectVertexColors(const glhckHandle handle, const int vertexColors)
{
   setFlag(handle, GLHCK_OBJECT_VERTEX_COLOR, vertexColors);
}

/* \brief get whether object should use vertex colors */
GLHCKAPI int glhckObjectGetVertexColors(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_VERTEX_COLOR);
}

/* \brief set whether object should be culled */
GLHCKAPI void glhckObjectCull(const glhckHandle handle, const int cull)
{
   setFlag(handle, GLHCK_OBJECT_CULL, cull);
}

/* \brief get whether object is culled */
GLHCKAPI int glhckObjectGetCull(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_CULL);
}

/* \brief set wether object should be depth tested */
GLHCKAPI void glhckObjectDepth(const glhckHandle handle, const int depth)
{
   setFlag(handle, GLHCK_OBJECT_DEPTH, depth);
}

/* \brief get wether object should be depth tested */
GLHCKAPI int glhckObjectGetDepth(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_DEPTH);
}

/* \brief set object's AABB drawing */
GLHCKAPI void glhckObjectDrawAABB(const glhckHandle handle, const int drawAABB)
{
   setFlag(handle, GLHCK_OBJECT_DRAW_AABB, drawAABB);
}

/* \brief get object's AABB drawing */
GLHCKAPI int glhckObjectGetDrawAABB(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_DRAW_AABB);
}

/* \brief set object's OBB drawing */
GLHCKAPI void glhckObjectDrawOBB(const glhckHandle handle, const int drawOBB)
{
   setFlag(handle, GLHCK_OBJECT_DRAW_OBB, drawOBB);
}

/* \brief get object's OBB drawing */
GLHCKAPI int glhckObjectGetDrawOBB(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_DRAW_OBB);
}

/* \brief set object's skeleton drawing */
GLHCKAPI void glhckObjectDrawSkeleton(const glhckHandle handle, const int drawSkeleton)
{
   setFlag(handle, GLHCK_OBJECT_DRAW_SKELETON, drawSkeleton);
}

/* \brief get object's skeleton drawing */
GLHCKAPI int glhckObjectGetDrawSkeleton(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_DRAW_SKELETON);
}

/* \brief set object's wireframe drawing */
GLHCKAPI void glhckObjectDrawWireframe(const glhckHandle handle, const int drawWireframe)
{
   setFlag(handle, GLHCK_OBJECT_DRAW_WIREFRAME, drawWireframe);
}

/* \brief get object's wireframe drawing */
GLHCKAPI int glhckObjectGetDrawWireframe(const glhckHandle handle)
{
   return getFlag(handle, GLHCK_OBJECT_DRAW_WIREFRAME);
}

/* \brief get obb bounding box of the object that includes all the children */
GLHCKAPI const kmAABB* glhckObjectGetOBBWithChildren(const glhckHandle handle)
{
   updateMatrix(handle);
   return glhckViewGetOBBWithChildren(glhckObjectGetView(handle));
}

/* \brief get obb bounding box of the object */
GLHCKAPI const kmAABB* glhckObjectGetOBB(const glhckHandle handle)
{
#if 0
   /* if performed on root, return the full obb instead */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      return glhckObjectGetOBBWithChildren(object);
#endif

   updateMatrix(handle);
   return glhckViewGetOBB(glhckObjectGetView(handle));
}

/* \brief get aabb bounding box of the object that includes all the children */
GLHCKAPI const kmAABB* glhckObjectGetAABBWithChildren(const glhckHandle handle)
{
   updateMatrix(handle);
   return glhckViewGetAABBWithChildren(glhckObjectGetView(handle));
}

/* \brief get aabb bounding box of the object */
GLHCKAPI const kmAABB* glhckObjectGetAABB(const glhckHandle handle)
{
#if 0
   /* if performed on root, return the full aabb instead */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      return glhckObjectGetAABBWithChildren(object);
#endif

   updateMatrix(handle);
   return glhckViewGetAABB(glhckObjectGetView(handle));
}

/* \brief get object's matrix */
GLHCKAPI const kmMat4* glhckObjectGetMatrix(const glhckHandle handle)
{
   updateMatrix(handle);
   return glhckViewGetMatrix(glhckObjectGetView(handle));
}

GLHCKAPI const kmVec3* glhckObjectGetPosition(const glhckHandle handle)
{
   return glhckViewGetPosition(glhckObjectGetView(handle));
}

/* \brief position object */
GLHCKAPI void glhckObjectPosition(const glhckHandle handle, const kmVec3 *position)
{
   glhckViewPosition(glhckObjectGetView(handle), position);
}

/* \brief position object (with kmScalar) */
GLHCKAPI void glhckObjectPositionf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewPositionf(glhckObjectGetView(handle), x, y, z);
}

/* \brief move object */
GLHCKAPI void glhckObjectMove(const glhckHandle handle, const kmVec3 *move)
{
   glhckViewMove(glhckObjectGetView(handle), move);
}

/* \brief move object (with kmScalar) */
GLHCKAPI void glhckObjectMovef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewMovef(glhckObjectGetView(handle), x, y, z);
}

/* \brief get object rotation */
GLHCKAPI const kmVec3* glhckObjectGetRotation(const glhckHandle handle)
{
   return glhckViewGetRotation(glhckObjectGetView(handle));
}

/* \brief set object rotation */
GLHCKAPI void glhckObjectRotation(const glhckHandle handle, const kmVec3 *rotation)
{
   glhckViewRotation(glhckObjectGetView(handle), rotation);
}

/* \brief set object rotation (with kmScalar) */
GLHCKAPI void glhckObjectRotationf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewRotationf(glhckObjectGetView(handle), x, y, z);
}

/* \brief rotate object */
GLHCKAPI void glhckObjectRotate(const glhckHandle handle, const kmVec3 *rotate)
{
   glhckViewRotate(glhckObjectGetView(handle), rotate);
}

/* \brief rotate object (with kmScalar) */
GLHCKAPI void glhckObjectRotatef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewRotatef(glhckObjectGetView(handle), x, y, z);
}

/* \brief get object target */
GLHCKAPI const kmVec3* glhckObjectGetTarget(const glhckHandle handle)
{
   return glhckViewGetTarget(glhckObjectGetView(handle));
}

/* \brief set object target */
GLHCKAPI void glhckObjectTarget(const glhckHandle handle, const kmVec3 *target)
{
   glhckViewTarget(glhckObjectGetView(handle), target);
}

/* \brief set object target (with kmScalar) */
GLHCKAPI void glhckObjectTargetf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewTargetf(glhckObjectGetView(handle), x, y, z);
}

/* \brief get object scale */
GLHCKAPI const kmVec3* glhckObjectGetScale(const glhckHandle handle)
{
   return glhckViewGetScale(glhckObjectGetView(handle));
}

/* \brief scale object */
GLHCKAPI void glhckObjectScale(const glhckHandle handle, const kmVec3 *scale)
{
   glhckViewScale(glhckObjectGetView(handle), scale);
}

/* \brief scale object (with kmScalar) */
GLHCKAPI void glhckObjectScalef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewScalef(glhckObjectGetView(handle), x, y, z);
}


/* \brief insert bones to object */
GLHCKAPI int glhckObjectInsertBones(const glhckHandle handle, const glhckHandle *bones, const size_t memb)
{
   return setListHandles($bones, handle, bones, memb);
}

/* \brief get bones assigned to this object */
GLHCKAPI const glhckHandle* glhckObjectBones(const glhckHandle handle, size_t *outMemb)
{
   return getList($bones, handle, outMemb);
}

/* \brief get bone by name */
GLHCKAPI glhckHandle glhckObjectGetBone(const glhckHandle handle, const char *name)
{
   assert(handle > 0 && name);
   CALL(2, "%s, %s", glhckHandleRepr(handle), name);

   size_t memb;
   const glhckHandle *bones = glhckObjectBones(handle, &memb);
   for (size_t i = 0; i < memb; ++i) {
      const char *bname = glhckBoneGetName(bones[i]);
      if (bname && !strcmp(bname, name)) {
         RET(2, "%s", glhckHandleRepr(bones[i]));
         return bones[i];
      }
   }

   RET(2, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief insert skin bones to object */
GLHCKAPI int glhckObjectInsertSkinBones(const glhckHandle handle, const glhckHandle *skinBones, size_t memb)
{
   if (!setListHandles($bones, handle, skinBones, memb))
      return RETURN_FAIL;

#if 0
   if (skinBones && memb > 0) {
      __GLHCKvertexType *type = GLHCKVT(object->geometry->vertexType);
      if (!object->bind && !(object->bind = _glhckCopy(object->geometry->vertices, object->geometry->vertexCount * type->size)))
         goto fail;
      if (!object->zero && !(object->zero = _glhckCopy(object->geometry->vertices, object->geometry->vertexCount * type->size)))
         goto fail;

      for (int v = 0; v < object->geometry->vertexCount; ++v)
         memset(object->zero + type->size * v + type->offset[0], 0, type->msize[0] * type->memb[0]);
      _glhckSkinBoneTransformObject(object, 1);
   } else {
      IFDO(_glhckFree, object->bind);
      IFDO(_glhckFree, object->zero);
   }
#endif

   return RETURN_OK;
}

/* \brief get skin bones assigned to this object */
GLHCKAPI const glhckHandle* glhckObjectSkinBones(const glhckHandle handle, size_t *outMemb)
{
   return getList($skinBones, handle, outMemb);
}

/* \brief get skin bone by name */
GLHCKAPI glhckHandle glhckObjectGetSkinBone(const glhckHandle handle, const char *name)
{
   assert(handle > 0 && name);
   CALL(2, "%s, %s", glhckHandleRepr(handle), name);

   size_t memb;
   const glhckHandle *skinBones = glhckObjectSkinBones(handle, &memb);
   for (size_t i = 0; i < memb; ++i) {
      const char *bname = glhckSkinBoneGetName(skinBones[i]);
      if (bname && !strcmp(bname, name)) {
         RET(2, "%s", glhckHandleRepr(skinBones[i]));
         return skinBones[i];
      }
   }

   RET(2, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief insert animations to object */
GLHCKAPI int glhckObjectInsertAnimations(const glhckHandle handle, const glhckHandle *animations, const size_t memb)
{
   return setListHandles($animations, handle, animations, memb);
}

/* \brief get animations assigned to this object */
GLHCKAPI const glhckHandle* glhckObjectAnimations(const glhckHandle handle, size_t *outMemb)
{
   return getList($animations, handle, outMemb);
}

/* \brief create new geometry for object, replacing existing one. */
GLHCKAPI void glhckObjectGeometry(const glhckHandle handle, const glhckHandle geometry)
{
   setHandle($geometry, handle, geometry);
}

/* \brief get geometry from object */
GLHCKAPI glhckHandle glhckObjectGetGeometry(const glhckHandle handle)
{
   return *(glhckHandle*)get($geometry, handle);
}

/* \brief insert indices to object */
GLHCKAPI int glhckObjectInsertIndices(const glhckHandle handle, const glhckIndexType type, const glhckImportIndexData *indices, const int memb)
{
   CALL(0, "%s, %u, %p, %d", glhckHandleRepr(handle), type, indices, memb);
   assert(handle > 0);

   if (!indices || memb <= 0) {
      releaseHandle($geometry, handle);
   } else {
      glhckHandle geometry = glhckObjectGetGeometry(handle);

      if (!geometry) {
         if (!(geometry = glhckGeometryNew()))
            goto fail;

         glhckObjectGeometry(handle, geometry);
         glhckHandleRelease(geometry);
      }

      if (!glhckGeometryInsertIndices(geometry, type, indices, memb))
         goto fail;
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert vertices to object */
GLHCKAPI int glhckObjectInsertVertices(const glhckHandle handle, const glhckVertexType type, const glhckImportVertexData *vertices, const int memb)
{
   CALL(0, "%s, %u, %p, %d", glhckHandleRepr(handle), type, vertices, memb);
   assert(handle > 0);

   if (!vertices || memb <= 0) {
      releaseHandle($geometry, handle);
   } else {
      glhckHandle geometry = glhckObjectGetGeometry(handle);

      if (!geometry) {
         if (!(geometry = glhckGeometryNew()))
            goto fail;

         glhckObjectGeometry(handle, geometry);
         glhckHandleRelease(geometry);
      }

      if (!glhckGeometryInsertVertices(geometry, type, vertices, memb))
         goto fail;
   }

   glhckObjectUpdate(handle);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief update/recalculate object's internal state */
GLHCKAPI void glhckObjectUpdate(const glhckHandle handle)
{
   TRACE(1);
   assert(handle > 0);

   const glhckHandle geometry = glhckObjectGetGeometry(handle);
   if (!geometry)
      return;

   glhckViewUpdateFromGeometry(handle, geometry);

#if 0
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectUpdate);
#endif
}

#if 0
/* \brief return intersecting texture coordinate from object's geometry using ray */
GLHCKAPI int glhckObjectPickTextureCoordinatesWithRay(const glhckObject *object, const kmRay3 *ray, kmVec2 *outCoords)
{
   CALL(0, "%p, %p, %p", object, ray, outCoords);
   assert(object && ray && outCoords);

   /* FIXME: move to geometry.c
    * Can we use collision mesh for this instead somehow?
    * It would simplify a lot since collisionMesh is optimized more for intersections. */
   assert(object->geometry->vertexType == GLHCK_VTX_V3F && "Only GLHCK_VTX_V3F supported for now");
   assert((object->geometry->type == GLHCK_TRIANGLES || object->geometry->type == GLHCK_TRIANGLE_STRIP) && "Only TRIANGLES and STRIPS supported for now");

   /* Transform the ray to model coordinates */
   kmMat4 inverseView;
   kmMat4Inverse(&inverseView, &object->view.matrix);

   kmRay3 r;
   kmVec3Transform(&r.start, &ray->start, &inverseView);
   kmVec3Transform(&r.dir, &ray->dir, &inverseView);

   kmVec3 origin = {0, 0, 0};
   kmVec3Transform(&origin, &origin, &inverseView);
   kmVec3Subtract(&r.dir, &r.dir, &origin);

   /* Find closest intersecting primitive */
   int intersectionIndex, intersectionFound = 0;
   kmScalar closestDistanceSq, s, t;

   int fix[3];
   int skipCount = (object->geometry->type == GLHCK_TRIANGLES ? 3 : 1);
   glhckVertexData3f *vertices = (glhckVertexData3f*) object->geometry->vertices;
   for (int i = 0; i + 2 < object->geometry->vertexCount; i += skipCount) {
      int ix[3] = {0, 1, 2};
      if (skipCount == 1 && (i % 2)) {
         ix[0] = 1;
         ix[1] = 0;
         ix[2] = 2;
      }

      const kmTriangle tri = {
         {vertices[i+ix[0]].vertex.x, vertices[i+ix[0]].vertex.y, vertices[i+ix[0]].vertex.z},
         {vertices[i+ix[1]].vertex.x, vertices[i+ix[1]].vertex.y, vertices[i+ix[1]].vertex.z},
         {vertices[i+ix[2]].vertex.x, vertices[i+ix[2]].vertex.y, vertices[i+ix[2]].vertex.z}
      };

      kmVec3 intersection;
      kmScalar ss, tt;
      if (kmRay3IntersectTriangle(&r, &tri, &intersection, &ss, &tt)) {
         kmVec3 delta;
         kmVec3Subtract(&delta, &ray->start, &intersection);
         kmScalar distanceSq = kmVec3LengthSq(&delta);
         if (!intersectionFound || distanceSq < closestDistanceSq) {
            intersectionFound = 1;
            closestDistanceSq = distanceSq;
            intersectionIndex = i;
            s = ss;
            t = tt;
            memcpy(fix, ix, sizeof(fix));
         }
      }
   }

   /* determine texture coordinates of the intersection point */
   if (intersectionFound) {
      const kmVec2 texCoods[] = {
         {vertices[intersectionIndex+fix[0]].coord.x, vertices[intersectionIndex+fix[0]].coord.y},
         {vertices[intersectionIndex+fix[1]].coord.x, vertices[intersectionIndex+fix[1]].coord.y},
         {vertices[intersectionIndex+fix[2]].coord.x, vertices[intersectionIndex+fix[2]].coord.y},
      };

      kmVec2 u, v;
      kmVec2Subtract(&u, &texCoods[1], &texCoods[0]);
      kmVec2Subtract(&v, &texCoods[2], &texCoods[0]);
      kmVec2Scale(&u, &u, s);
      kmVec2Scale(&v, &v, t);
      memcpy(outCoords, &texCoods[0], sizeof(kmVec2));
      kmVec2Add(outCoords, outCoords, &u);
      kmVec2Add(outCoords, outCoords, &v);
   }

   return intersectionFound;
}
#endif

/* vim: set ts=8 sw=3 tw=0 :*/
