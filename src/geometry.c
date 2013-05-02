#include "internal.h"
#include <limits.h> /* for type limits */

#define GLHCK_CHANNEL GLHCK_CHANNEL_VDATA

/* This file contains all the methods that access
 * object's internal vertexdata, since the GLHCK object's
 * internal vertexdata structure is quite complex */

/* FIXME:
 * When moving to modern OpenGL api,
 * we can propably convert coordinates to unsigned types. */

/* conversion constants for vertexdata conversion */
#define GLHCK_BYTE_CMAGIC  CHAR_MAX
#define GLHCK_BYTE_VMAGIC  UCHAR_MAX - CHAR_MAX
#define GLHCK_BYTE_VBIAS   CHAR_MAX / (UCHAR_MAX - 1.0f)
#define GLHCK_BYTE_VSCALE  UCHAR_MAX - 1.0f

#define GLHCK_SHORT_CMAGIC  SHRT_MAX
#define GLHCK_SHORT_VMAGIC  USHRT_MAX - SHRT_MAX
#define GLHCK_SHORT_VBIAS   SHRT_MAX / (USHRT_MAX - 1.0f)
#define GLHCK_SHORT_VSCALE  USHRT_MAX - 1.0f

/* 1. 2D vector conversion function name
 * 2. 3D vector conversion function name
 * 3. 2D internal vector cast
 * 4. 3D internal vector cast
 * 5. Base cast of vector's units
 * 6. Vector magic constant */
#define glhckMagicDefine(namev2, namev3, castv2, castv3, bcast, magic)                    \
inline static void namev2(                                                                \
   castv2 *vb, const glhckVector3f *vh,                                                   \
   glhckVector3f *max, glhckVector3f *min)                                                \
{                                                                                         \
   vb->x = (bcast)floorf(((vh->x - min->x) / (max->x - min->x)) * magic + 0.5f);          \
   vb->y = (bcast)floorf(((vh->y - min->y) / (max->y - min->y)) * magic + 0.5f);          \
}                                                                                         \
inline static void namev3(                                                                \
   castv3 *vb, const glhckVector3f *vh,                                                   \
   glhckVector3f *max, glhckVector3f *min)                                                \
{                                                                                         \
   namev2((castv2*)vb, vh, max, min);                                                     \
   vb->z = (bcast)floorf(((vh->z - min->z) / (max->z - min->z)) * magic + 0.5f);          \
}

/* convert vector to internal byte format */
glhckMagicDefine(glhckMagic2b, glhckMagic3b,
      glhckVector2b, glhckVector3b, char, GLHCK_BYTE_VMAGIC);

/* convert vector to internal short format */
glhckMagicDefine(glhckMagic2s, glhckMagic3s,
      glhckVector2s, glhckVector3s, short, GLHCK_SHORT_VMAGIC);

/* \brief get min/max from import data */
static void _glhckMaxMinImportData(const glhckImportVertexData *import, int memb, glhckVector3f *vmin, glhckVector3f *vmax)
{
   int i;
   glhckSetV3(vmax, &import[0].vertex);
   glhckSetV3(vmin, &import[0].vertex);

   /* find max && min first */
   for (i = 1; i != memb; ++i) {
      glhckMaxV3(vmax, &import[i].vertex);
      glhckMinV3(vmin, &import[i].vertex);
   }
}

/* \brief get /min/max from index data */
static void _glhckMaxMinIndexData(const glhckImportIndexData *import, int memb, unsigned int *mini, unsigned int *maxi)
{
   int i;
   *mini = *maxi = import[0];
   for (i = 1; i != memb; ++i) {
      if (*mini > import[i]) *mini = import[i];
      if (*maxi < import[i]) *maxi = import[i];
   }
}

/* \brief check that vertex type is ok for allocation. If not override it with better one */
glhckGeometryVertexType _glhckGeometryCheckVertexType(glhckGeometryVertexType type)
{
   /* default to V3F on NONE type */
   if (type == GLHCK_VERTEX_NONE) {
      type = GLHCK_VERTEX_V3F;
   }

   /* fglrx on linux at least seems to fail with byte vertices.
    * workaround for now until I stumble upon why it's wrong. */
   if (GLHCKR()->driver == GLHCK_DRIVER_ATI) {
      if (type == GLHCK_VERTEX_V2B || type == GLHCK_VERTEX_V3B) {
         DEBUG(GLHCK_DBG_WARNING, "ATI has problems with BYTE precision vertexdata.");
         DEBUG(GLHCK_DBG_WARNING, "Will use SHORT precision instead, just a friendly warning.");
      }
      if (type == GLHCK_VERTEX_V3B) type = GLHCK_VERTEX_V3S;
      if (type == GLHCK_VERTEX_V2B) type = GLHCK_VERTEX_V2S;
   }
   return type;
}

/* \brief check that index type is ok for allocation. If not override it with better one */
glhckGeometryIndexType _glhckGeometryCheckIndexType(const glhckImportIndexData *indices, int memb, glhckGeometryIndexType type)
{
   unsigned int mini, maxi;

   /* autodetect */
   if (type == GLHCK_INDEX_NONE) {
      _glhckMaxMinIndexData(indices, memb, &mini, &maxi);
      type = glhckIndexTypeForValue(maxi);
   }
   return type;
}

/* \brief convert vertex data to internal format */
static void _glhckConvertVertexData(
      glhckGeometryVertexType type, glhckVertexData internal,
      const glhckImportVertexData *import, int memb,
      glhckVector3f *bias, glhckVector3f *scale)
{
   int i;
   float biasMagic = 0.0f, scaleMagic = 0.0f;
   glhckVector3f vmin, vmax;
   CALL(0, "%d, %p, %d, %p, %p", type, import, memb, bias, scale);

   /* default bias scale */
   bias->x  = bias->y  = bias->z  = 0.0f;
   scale->x = scale->y = scale->z = 1.0f;

   /* get min and max from import data */
   _glhckMaxMinImportData(import, memb, &vmin, &vmax);

   /* lie about bounds by 1 point so,
    * we don't get artifacts */
   vmax.x++; vmax.y++; vmax.z++;
   vmin.x--; vmin.y--; vmin.z--;

   if (type == GLHCK_VERTEX_V3F) {
      memcpy(internal.v3f, import, memb * sizeof(glhckVertexData3f));

      /* center geometry */
      for (i = 0; i != memb; ++i) {
         internal.v3f[i].vertex.x -= 0.5f * (vmax.x-vmin.x)+vmin.x;
         internal.v3f[i].vertex.y -= 0.5f * (vmax.y-vmin.y)+vmin.y;
         internal.v3f[i].vertex.z -= 0.5f * (vmax.z-vmin.z)+vmin.z;
      }
   } else {
      /* handle vertex data conversion */
      for (i = 0; i != memb; ++i) {
         switch (type) {
            case GLHCK_VERTEX_V3B:
               memcpy(&internal.v3b[i].color,  &import[i].color,  sizeof(glhckColorb));

               glhckMagic3b(&internal.v3b[i].vertex, &import[i].vertex, &vmax, &vmin);

               internal.v3b[i].normal.x = (short)(import[i].normal.x * GLHCK_SHORT_CMAGIC);
               internal.v3b[i].normal.y = (short)(import[i].normal.y * GLHCK_SHORT_CMAGIC);
               internal.v3b[i].normal.z = (short)(import[i].normal.z * GLHCK_SHORT_CMAGIC);

               internal.v3b[i].coord.x = (short)(import[i].coord.x * GLHCK_SHORT_CMAGIC);
               internal.v3b[i].coord.y = (short)(import[i].coord.y * GLHCK_SHORT_CMAGIC);

               biasMagic  = GLHCK_BYTE_VBIAS;
               scaleMagic = GLHCK_BYTE_VSCALE;
               break;

            case GLHCK_VERTEX_V2B:
               memcpy(&internal.v2b[i].color,  &import[i].color,  sizeof(glhckColorb));

               glhckMagic2b(&internal.v2b[i].vertex, &import[i].vertex, &vmax, &vmin);

               internal.v2b[i].normal.x = (short)(import[i].normal.x * GLHCK_SHORT_CMAGIC);
               internal.v2b[i].normal.y = (short)(import[i].normal.y * GLHCK_SHORT_CMAGIC);
               internal.v2b[i].normal.z = (short)(import[i].normal.z * GLHCK_SHORT_CMAGIC);

               internal.v2b[i].coord.x = (short)(import[i].coord.x * GLHCK_SHORT_CMAGIC);
               internal.v2b[i].coord.y = (short)(import[i].coord.y * GLHCK_SHORT_CMAGIC);

               biasMagic  = GLHCK_BYTE_VBIAS;
               scaleMagic = GLHCK_BYTE_VSCALE;
               break;

            case GLHCK_VERTEX_V3S:
               memcpy(&internal.v3s[i].color,  &import[i].color,  sizeof(glhckColorb));

               glhckMagic3s(&internal.v3s[i].vertex, &import[i].vertex, &vmax, &vmin);

               internal.v3s[i].normal.x = (short)(import[i].normal.x * GLHCK_SHORT_CMAGIC);
               internal.v3s[i].normal.y = (short)(import[i].normal.y * GLHCK_SHORT_CMAGIC);
               internal.v3s[i].normal.z = (short)(import[i].normal.z * GLHCK_SHORT_CMAGIC);

               internal.v3s[i].coord.x = (short)(import[i].coord.x * GLHCK_SHORT_CMAGIC);
               internal.v3s[i].coord.y = (short)(import[i].coord.y * GLHCK_SHORT_CMAGIC);

               biasMagic  = GLHCK_SHORT_VBIAS;
               scaleMagic = GLHCK_SHORT_VSCALE;
               break;

            case GLHCK_VERTEX_V2S:
               memcpy(&internal.v2s[i].color,  &import[i].color,  sizeof(glhckColorb));

               glhckMagic2s(&internal.v2s[i].vertex, &import[i].vertex, &vmax, &vmin);

               internal.v2s[i].normal.x = (short)(import[i].normal.x * GLHCK_SHORT_CMAGIC);
               internal.v2s[i].normal.y = (short)(import[i].normal.y * GLHCK_SHORT_CMAGIC);
               internal.v2s[i].normal.z = (short)(import[i].normal.z * GLHCK_SHORT_CMAGIC);

               internal.v2s[i].coord.x = (short)(import[i].coord.x * GLHCK_SHORT_CMAGIC);
               internal.v2s[i].coord.y = (short)(import[i].coord.y * GLHCK_SHORT_CMAGIC);

               biasMagic  = GLHCK_SHORT_VBIAS;
               scaleMagic = GLHCK_SHORT_VSCALE;
               break;

            case GLHCK_VERTEX_V3FS:
               memcpy(&internal.v3fs[i].color,  &import[i].color,  sizeof(glhckColorb));
               memcpy(&internal.v3fs[i].vertex, &import[i].vertex, sizeof(glhckVector3f));

               internal.v3fs[i].normal.x = (short)(import[i].normal.x * GLHCK_SHORT_CMAGIC);
               internal.v3fs[i].normal.y = (short)(import[i].normal.y * GLHCK_SHORT_CMAGIC);
               internal.v3fs[i].normal.z = (short)(import[i].normal.z * GLHCK_SHORT_CMAGIC);

               internal.v3fs[i].coord.x = (short)(import[i].coord.x * GLHCK_SHORT_CMAGIC);
               internal.v3fs[i].coord.y = (short)(import[i].coord.y * GLHCK_SHORT_CMAGIC);

               /* center */
               internal.v3fs[i].vertex.x -= 0.5f * (vmax.x-vmin.x)+vmin.x;
               internal.v3fs[i].vertex.y -= 0.5f * (vmax.y-vmin.y)+vmin.y;
               internal.v3fs[i].vertex.z -= 0.5f * (vmax.z-vmin.z)+vmin.z;
               break;

            case GLHCK_VERTEX_V2FS:
               memcpy(&internal.v2fs[i].color,  &import[i].color,  sizeof(glhckColorb));
               memcpy(&internal.v2fs[i].vertex, &import[i].vertex, sizeof(glhckVector2f));

               internal.v2fs[i].normal.x = (short)(import[i].normal.x * GLHCK_SHORT_CMAGIC);
               internal.v2fs[i].normal.y = (short)(import[i].normal.y * GLHCK_SHORT_CMAGIC);
               internal.v2fs[i].normal.z = (short)(import[i].normal.z * GLHCK_SHORT_CMAGIC);

               internal.v2fs[i].coord.x = (short)(import[i].coord.x * GLHCK_SHORT_CMAGIC);
               internal.v2fs[i].coord.y = (short)(import[i].coord.y * GLHCK_SHORT_CMAGIC);

               /* center */
               internal.v2fs[i].vertex.x -= 0.5f * (vmax.x-vmin.x)+vmin.x;
               internal.v2fs[i].vertex.y -= 0.5f * (vmax.y-vmin.y)+vmin.y;
               break;

            case GLHCK_VERTEX_V2F:
               memcpy(&internal.v2f[i].color,  &import[i].color,  sizeof(glhckColorb));
               memcpy(&internal.v2f[i].vertex, &import[i].vertex, sizeof(glhckVector2f));
               memcpy(&internal.v2f[i].normal, &import[i].normal, sizeof(glhckVector3f));
               memcpy(&internal.v2f[i].coord,  &import[i].coord,  sizeof(glhckVector2f));

               /* center */
               internal.v2f[i].vertex.x -= 0.5f * (vmax.x-vmin.x)+vmin.x;
               internal.v2f[i].vertex.y -= 0.5f * (vmax.y-vmin.y)+vmin.y;
               break;

            default:
               DEBUG(GLHCK_DBG_ERROR, "Something terrible happenned.");
               assert(0 && "NOT IMPLEMENTED");
               return;
         }
      }
   }

   DEBUG(GLHCK_DBG_CRAP, "CONV: "VEC3S" : "VEC3S" (%f:%f)",
         VEC3(&vmax),  VEC3(&vmin), biasMagic, scaleMagic);

   if (type == GLHCK_VERTEX_V3F  || type == GLHCK_VERTEX_V2F ||
       type == GLHCK_VERTEX_V3FS || type == GLHCK_VERTEX_V2FS) {
      /* fix bias after centering */
      bias->x = 0.5f * (vmax.x - vmin.x) + vmin.x;
      bias->y = 0.5f * (vmax.y - vmin.y) + vmin.y;
      bias->z = 0.5f * (vmax.z - vmin.z) + vmin.z;
   } else {
      /* fix geometry bias && scale after conversion */
      bias->x  = biasMagic*(vmax.x-vmin.x)+vmin.x;
      bias->y  = biasMagic*(vmax.y-vmin.y)+vmin.y;
      bias->z  = biasMagic*(vmax.z-vmin.z)+vmin.z;
      scale->x = (vmax.x-vmin.x)/scaleMagic;
      scale->y = (vmax.y-vmin.y)/scaleMagic;
      scale->z = (vmax.z-vmin.z)/scaleMagic;
   }

   DEBUG(GLHCK_DBG_CRAP, "Converted bias "VEC3S, VEC3(bias));
   DEBUG(GLHCK_DBG_CRAP, "Converted scale "VEC3S, VEC3(scale));
}

/* \brief convert index data to internal format */
static void _glhckConvertIndexData(
      glhckGeometryIndexType type, glhckIndexData internal,
      const glhckImportIndexData *import, int memb)
{
   int i;
   CALL(0, "%d, %p, %d", type, import, memb);
   if (type == GLHCK_INDEX_INTEGER) {
      memcpy(internal.ivi, import, memb *sizeof(glhckImportIndexData));
   } else {
      for (i = 0; i != memb; ++i) {
         switch (type) {
            case GLHCK_INDEX_BYTE:
               internal.ivb[i] = (glhckIndexb)import[i];
               break;

            case GLHCK_INDEX_SHORT:
               internal.ivs[i] = (glhckIndexs)import[i];
               break;

            default:
               DEBUG(GLHCK_DBG_ERROR, "Something terrible happenned.");
               assert(0 && "NOT IMPLEMENTED");
               break;
         }
      }
   }
}

/* \brief free geometry's vertex data */
static void _glhckGeometryFreeVertices(glhckGeometry *object)
{
   IFDO(_glhckFree, object->vertices.any);
   object->vertexType   = GLHCK_VERTEX_NONE;
   object->vertexCount  = 0;
   object->textureRange = 1;
}

/* \brief assign vertices to object internally */
static void _glhckGeometrySetVertices(glhckGeometry *object,
      glhckGeometryVertexType type, void *data, int memb)
{
   unsigned short textureRange = 1;

   /* free old vertices */
   _glhckGeometryFreeVertices(object);

   /* assign vertices depending on type */
   switch (type) {
      case GLHCK_VERTEX_V3B:
         object->vertices.v3b = (glhckVertexData3b*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V2B:
         object->vertices.v2b = (glhckVertexData2b*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V3S:
         object->vertices.v3s = (glhckVertexData3s*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V2S:
         object->vertices.v2s = (glhckVertexData2s*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V3FS:
         object->vertices.v3fs = (glhckVertexData3fs*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V2FS:
         object->vertices.v2fs = (glhckVertexData2fs*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V3F:
         object->vertices.v3f = (glhckVertexData3f*)data;
         break;

      case GLHCK_VERTEX_V2F:
         object->vertices.v2f = (glhckVertexData2f*)data;
         break;

      default:
         assert(0 && "NOT IMPLEMENTED!");
         break;
   }

   /* set vertex type */
   object->vertexType   = type;
   object->vertexCount  = memb;
   object->textureRange = textureRange;
}

/* \brief free indices from object */
static void _glhckGeometryFreeIndices(glhckGeometry *object)
{
   /* set index type to none */
   IFDO(_glhckFree, object->indices.any);
   object->indexType  = GLHCK_INDEX_NONE;
   object->indexCount = 0;
}

/* \brief assign indices to object internally */
static void _glhckGeometrySetIndices(glhckGeometry *object,
      glhckGeometryIndexType type, void *data, int memb)
{
   /* free old indices */
   _glhckGeometryFreeIndices(object);

   /* assign indices depending on type */
   switch (type) {
      case GLHCK_INDEX_BYTE:
         object->indices.ivb = (glhckIndexb*)data;
         break;

      case GLHCK_INDEX_SHORT:
         object->indices.ivs = (glhckIndexs*)data;
         break;

      case GLHCK_INDEX_INTEGER:
         object->indices.ivi = (glhckIndexi*)data;
         break;

      default:
         assert(0 && "NOT IMPLEMENTED!");
         break;
   }

   /* set index type */
   object->indexType  = type;
   object->indexCount = memb;
}

/* \brief insert vertices into object */
int _glhckGeometryInsertVertices(
      glhckGeometry *object, int memb,
      glhckGeometryVertexType type,
      const glhckImportVertexData *vertices)
{
   void *data = NULL;
   glhckVertexData vd;
   CALL(0, "%p, %d, %d, %p", object, memb, type, vertices);
   assert(object);

   /* check vertex type */
   type = _glhckGeometryCheckVertexType(type);

   /* check vertex precision conflicts */
   if ((unsigned int)memb > glhckIndexTypeMaxPrecision(object->indexType))
      goto bad_precision;

   /* allocate depending on type */
   switch (type) {
      case GLHCK_VERTEX_V3B:
         data = vd.v3b = _glhckMalloc(memb * sizeof(glhckVertexData3b));
         break;

      case GLHCK_VERTEX_V2B:
         data = vd.v2b = _glhckMalloc(memb * sizeof(glhckVertexData2b));
         break;

      case GLHCK_VERTEX_V3S:
         data = vd.v3s = _glhckMalloc(memb * sizeof(glhckVertexData3s));
         break;

      case GLHCK_VERTEX_V2S:
         data = vd.v2s = _glhckMalloc(memb * sizeof(glhckVertexData2s));
         break;

       case GLHCK_VERTEX_V3FS:
         data = vd.v3fs = _glhckMalloc(memb * sizeof(glhckVertexData3fs));
         break;

      case GLHCK_VERTEX_V2FS:
         data = vd.v2fs = _glhckMalloc(memb * sizeof(glhckVertexData2fs));
         break;

      case GLHCK_VERTEX_V3F:
         data = vd.v3f = _glhckMalloc(memb * sizeof(glhckVertexData3f));
         break;

      case GLHCK_VERTEX_V2F:
         data = vd.v2f = _glhckMalloc(memb * sizeof(glhckVertexData2f));
         break;

      default:
         assert(0 && "NOT IMPLEMNTED");
         break;
   }
   if (!data) goto fail;

   /* convert and assign */
   _glhckConvertVertexData(type, vd, vertices, memb, &object->bias, &object->scale);
   _glhckGeometrySetVertices(object, type, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %s, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                          glhckIndexTypeString(object->indexType),
                          object);
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert index data into object */
int _glhckGeometryInsertIndices(
      glhckGeometry *object, int memb,
      glhckGeometryIndexType type,
      const glhckImportIndexData *indices)
{
   void *data = NULL;
   glhckIndexData id;
   CALL(0, "%p, %d, %d, %p", object, memb, type, indices);
   assert(object);

   /* check index type */
   type = _glhckGeometryCheckIndexType(indices, memb, type);

   /* check vertex precision conflicts */
   if ((unsigned int)object->vertexCount > glhckIndexTypeMaxPrecision(object->indexType))
      goto bad_precision;

   /* allocate depending on type */
   switch (type) {
      case GLHCK_INDEX_BYTE:
         data = id.ivb = _glhckMalloc(memb * sizeof(glhckIndexb));
         break;

      case GLHCK_INDEX_SHORT:
         data = id.ivs = _glhckMalloc(memb * sizeof(glhckIndexs));
         break;

      case GLHCK_INDEX_INTEGER:
         data = id.ivi = _glhckMalloc(memb * sizeof(glhckIndexi));
         break;

      default:
         assert(0 && "NOT IMPLEMENTED!");
         break;
   }
   if (!data) goto fail;

   /* convert and assign */
   _glhckConvertIndexData(type, id, indices, memb);
   _glhckGeometrySetIndices(object, type, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %s, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                          glhckIndexTypeString(object->indexType),
                          object);
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief create new geometry object */
glhckGeometry* _glhckGeometryNew(void)
{
   glhckGeometry *object;

   if (!(object = _glhckCalloc(1, sizeof(glhckGeometry))))
      return NULL;

   /* default geometry type */
   object->type = GLHCK_TRIANGLE_STRIP;

   /* default scale */
   object->scale.x = object->scale.y = object->scale.z = 1.0f;

   return object;
}

/* \brief copy geometry object */
glhckGeometry* _glhckGeometryCopy(glhckGeometry *src)
{
   glhckGeometry *object;
   if (!src) return NULL;

   if (!(object = _glhckCopy(src, sizeof(glhckGeometry))))
      return NULL;

   if (src->vertices.any)
      object->vertices.any =_glhckCopy(src->vertices.any, src->vertexCount * glhckVertexTypeElementSize(src->vertexType));
   if (src->indices.any)
      object->indices.any = _glhckCopy(src->indices.any, src->indexCount * glhckIndexTypeElementSize(src->indexType));

   return object;
}

/* \brief release geometry object */
void _glhckGeometryFree(glhckGeometry *object)
{
   assert(object);
   IFDO(_glhckFree, object->transformedCoordinates);
   _glhckGeometryFreeVertices(object);
   _glhckGeometryFreeIndices(object);
   _glhckFree(object);
}

/***
 * public api
 ***/

/* \brief get element size for index type */
GLHCKAPI size_t glhckIndexTypeElementSize(glhckGeometryIndexType type)
{
   switch (type) {
      case GLHCK_INDEX_BYTE:
         return sizeof(glhckIndexb);

      case GLHCK_INDEX_SHORT:
         return sizeof(glhckIndexs);

      case GLHCK_INDEX_INTEGER:
         return sizeof(glhckIndexi);

      default:break;
   }

   /* default */
   return 0;
}

/* \brief return the maxium index precision allowed for object */
GLHCKAPI glhckIndexi glhckIndexTypeMaxPrecision(glhckGeometryIndexType type)
{
   switch (type) {
      case GLHCK_INDEX_BYTE:
         return UCHAR_MAX;

      case GLHCK_INDEX_SHORT:
         return USHRT_MAX;

      case GLHCK_INDEX_INTEGER:
         return UINT_MAX;

      default:break;
   }

   /* default */
   return UINT_MAX;
}

/* \brief get string for index precision type */
GLHCKAPI const char* glhckIndexTypeString(glhckGeometryIndexType type)
{
   switch (type) {
      case GLHCK_INDEX_BYTE:
         return "GLHCK_INDEX_BYTE";

      case GLHCK_INDEX_SHORT:
         return "GLHCK_INDEX_SHORT";

      case GLHCK_INDEX_INTEGER:
         return "GLHCK_INDEX_INTEGER";

      default:break;
   }

   /* default */
   return "GLHCK_INDEX_NONE";
}

/* \brief is value withing precision range? */
GLHCKAPI int glhckIndexTypeWithinRange(unsigned int value, glhckGeometryIndexType type)
{
   return (value <  glhckIndexTypeMaxPrecision(type));
}

/* \brief you should feed this function the value with highest precision */
GLHCKAPI glhckGeometryIndexType glhckIndexTypeForValue(unsigned int value)
{
   glhckGeometryIndexType itype = GLHCK_INDEX_INTEGER;
   if (glhckIndexTypeWithinRange(value, GLHCK_INDEX_BYTE))
      itype = GLHCK_INDEX_BYTE;
   else if (glhckIndexTypeWithinRange(value, GLHCK_INDEX_SHORT))
      itype = GLHCK_INDEX_SHORT;

   return itype;
}

/* \brief get element size for vertex type */
GLHCKAPI size_t glhckVertexTypeElementSize(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
         return sizeof(glhckVertexData3b);

      case GLHCK_VERTEX_V2B:
         return sizeof(glhckVertexData2b);

      case GLHCK_VERTEX_V3S:
         return sizeof(glhckVertexData3s);

      case GLHCK_VERTEX_V2S:
         return sizeof(glhckVertexData2s);

      case GLHCK_VERTEX_V3FS:
         return sizeof(glhckVertexData3fs);

      case GLHCK_VERTEX_V2FS:
         return sizeof(glhckVertexData2fs);

      case GLHCK_VERTEX_V3F:
         return sizeof(glhckVertexData3f);

      case GLHCK_VERTEX_V2F:
         return sizeof(glhckVertexData2f);

      default:break;
   }

   /* default */
   return 0;
}

/* \brief get maxium precision for vertex */
GLHCKAPI float glhckVertexTypeMaxPrecision(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
      case GLHCK_VERTEX_V2B:
         return CHAR_MAX;

      case GLHCK_VERTEX_V3S:
      case GLHCK_VERTEX_V2S:
         return SHRT_MAX;

      case GLHCK_VERTEX_V3FS:
      case GLHCK_VERTEX_V2FS:
      case GLHCK_VERTEX_V3F:
      case GLHCK_VERTEX_V2F:
         break;

      default:break;
   }

   /* default */
   return 0;
}

/* \brief get string for vertex precision type */
GLHCKAPI const char* glhckVertexTypeString(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
         return "GLHCK_VERTEX_V3B";

      case GLHCK_VERTEX_V2B:
         return "GLHCK_VERTEX_V2B";

      case GLHCK_VERTEX_V3S:
         return "GLHCK_VERTEX_V3S";

      case GLHCK_VERTEX_V2S:
         return "GLHCK_VERTEX_V2S";

      case GLHCK_VERTEX_V3FS:
         return "GLHCK_VERTEX_V3FS";

      case GLHCK_VERTEX_V2FS:
         return "GLHCK_VERTEX_V2FS";

      case GLHCK_VERTEX_V3F:
         return "GLHCK_VERTEX_V3F";

      case GLHCK_VERTEX_V2F:
         return "GLHCK_VERTEX_V2F";

      default:break;
   }

   /* default */
   return "GLHCK_VERTEX_NONE";
}

/* \brief get v2 counterpart of the vertexdata */
GLHCKAPI glhckGeometryVertexType glhckVertexTypeGetV2Counterpart(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
      case GLHCK_VERTEX_V2B:
         return GLHCK_VERTEX_V2B;

      case GLHCK_VERTEX_V3S:
      case GLHCK_VERTEX_V2S:
         return GLHCK_VERTEX_V2S;

      case GLHCK_VERTEX_V3FS:
      case GLHCK_VERTEX_V2FS:
         return GLHCK_VERTEX_V2FS;

      case GLHCK_VERTEX_V3F:
      case GLHCK_VERTEX_V2F:
         return GLHCK_VERTEX_V2F;

      default:
         break;
   }

   /* default */
   return type;
}

/* \brief get v3 counterpart of the vertexdata */
GLHCKAPI glhckGeometryVertexType glhckVertexTypeGetV3Counterpart(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
      case GLHCK_VERTEX_V2B:
         return GLHCK_VERTEX_V3B;

      case GLHCK_VERTEX_V3S:
      case GLHCK_VERTEX_V2S:
         return GLHCK_VERTEX_V3S;

      case GLHCK_VERTEX_V3FS:
      case GLHCK_VERTEX_V2FS:
         return GLHCK_VERTEX_V3FS;

      case GLHCK_VERTEX_V3F:
      case GLHCK_VERTEX_V2F:
         return GLHCK_VERTEX_V3F;

      default:
         break;
   }

   /* default */
   return type;
}

/* \brief get high precision counterpart of the vertexdata */
GLHCKAPI glhckGeometryVertexType glhckVertexTypeGetFloatingPointCounterpart(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3S:
      case GLHCK_VERTEX_V3B:
         return GLHCK_VERTEX_V3F;

      case GLHCK_VERTEX_V2S:
      case GLHCK_VERTEX_V2B:
         return GLHCK_VERTEX_V2F;

      case GLHCK_VERTEX_V3FS:
      case GLHCK_VERTEX_V2FS:
      case GLHCK_VERTEX_V3F:
      case GLHCK_VERTEX_V2F:
      default:
         break;
   }

   /* default */
   return type;
}

/* \brief is value withing precision range? */
GLHCKAPI int glhckVertexTypeWithinRange(float value, glhckGeometryVertexType type)
{
   return (value <  glhckVertexTypeMaxPrecision(type) &&
           value > -glhckVertexTypeMaxPrecision(type));
}

/* \brief get optimal vertex type for size */
GLHCKAPI glhckGeometryVertexType glhckVertexTypeForSize(kmScalar width, kmScalar height)
{
   glhckGeometryVertexType vtype = GLHCK_VERTEX_V3F;
   if (glhckVertexTypeWithinRange(width, GLHCK_VERTEX_V3B) &&
         glhckVertexTypeWithinRange(height, GLHCK_VERTEX_V3B))
      vtype = GLHCK_VERTEX_V3B;
   else if (glhckVertexTypeWithinRange(width, GLHCK_VERTEX_V3S) &&
         glhckVertexTypeWithinRange(height, GLHCK_VERTEX_V3S))
      vtype = GLHCK_VERTEX_V3S;
   return vtype;
}

/* \brief does vertex type have normal? */
GLHCKAPI int glhckVertexTypeHasNormal(glhckGeometryVertexType type)
{
   (void)type;
   return 1;
}

/* \brief does vertex type have color? */
GLHCKAPI int glhckVertexTypeHasColor(glhckGeometryVertexType type)
{
   (void)type;
   return 1;
}

/* \brief retieve vertex index from index array */
GLHCKAPI glhckIndexi glhckGeometryGetVertexIndexForIndex(glhckGeometry *object, glhckIndexi ix)
{
   assert(object && ix < (glhckIndexi)object->indexCount);
   switch (object->indexType) {
      case GLHCK_INDEX_BYTE:
         return object->indices.ivb[ix];
      case GLHCK_INDEX_SHORT:
         return object->indices.ivs[ix];
      case GLHCK_INDEX_INTEGER:
         return object->indices.ivi[ix];
      default:
         break;
   }
   return 0;
}

/* \brief set vertex index for index */
GLHCKAPI void glhckGeometrySetVertexIndexForIndex(glhckGeometry *object, glhckIndexi ix, glhckIndexi vertexIndex)
{
   assert(object && ix < (glhckIndexi)object->indexCount);
   switch (object->indexType) {
      case GLHCK_INDEX_BYTE:
         object->indices.ivb[ix] = vertexIndex;
      case GLHCK_INDEX_SHORT:
         object->indices.ivs[ix] = vertexIndex;
      case GLHCK_INDEX_INTEGER:
         object->indices.ivi[ix] = vertexIndex;
      default:
         break;
   }
}

/* \brief retieve internal vertex data for index */
GLHCKAPI void glhckGeometryGetVertexDataForIndex(
      glhckGeometry *object, glhckIndexi ix,
      glhckVector3f *vertex, glhckVector3f *normal,
      glhckVector2f *coord, glhckColorb *color)
{
   assert(object && ix < (glhckIndexi)object->vertexCount);
   if (vertex) memset(vertex, 0, sizeof(glhckVector3f));
   if (normal) memset(normal, 0, sizeof(glhckVector3f));
   if (coord)  memset(coord,  0, sizeof(glhckVector2f));
   if (color)  memset(color,  0, sizeof(glhckColorb));

   switch (object->vertexType) {
      case GLHCK_VERTEX_V3B:
         if (vertex) { glhckSetV3(vertex, &object->vertices.v3b[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v3b[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v3b[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v3b[ix].color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2B:
         if (vertex) { glhckSetV2(vertex, &object->vertices.v2b[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v2b[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v2b[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v2b[ix].color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V3S:
         if (vertex) { glhckSetV3(vertex, &object->vertices.v3s[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v3s[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v3s[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v3s[ix].color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2S:
         if (vertex) { glhckSetV2(vertex, &object->vertices.v2s[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v2s[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v2s[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v2s[ix].color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V3FS:
         if (vertex) { glhckSetV3(vertex, &object->vertices.v3fs[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v3fs[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v3fs[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v3fs[ix].color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2FS:
         if (vertex) { glhckSetV2(vertex, &object->vertices.v2fs[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v2fs[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v2fs[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v2fs[ix].color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V3F:
         if (vertex) { glhckSetV3(vertex, &object->vertices.v3f[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v3f[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v3f[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v3f[ix].color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2F:
         if (vertex) { glhckSetV2(vertex, &object->vertices.v2f[ix].vertex); }
         if (normal) { glhckSetV3(normal, &object->vertices.v2f[ix].normal); }
         if (coord)  { glhckSetV2(coord, &object->vertices.v2f[ix].coord); }
         if (color)  { memcpy(color, &object->vertices.v2f[ix].color, sizeof(glhckColorb)); }
         break;

      default:
         break;
   }
}

/* \brief set vertexdata for index */
GLHCKAPI void glhckGeometrySetVertexDataForIndex(
      glhckGeometry *object, glhckIndexi ix,
      const glhckVector3f *vertex, const glhckVector3f *normal,
      const glhckVector2f *coord, const glhckColorb *color)
{
   assert(ix < (glhckIndexi)object->vertexCount);
   switch (object->vertexType) {
      case GLHCK_VERTEX_V3B:
         if (vertex) { glhckSetV3(&object->vertices.v3b[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v3b[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v3b[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v3b[ix].color, color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2B:
         if (vertex) { glhckSetV2(&object->vertices.v2b[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v2b[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v2b[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v2b[ix].color, color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V3S:
         if (vertex) { glhckSetV3(&object->vertices.v3s[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v3s[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v3s[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v3s[ix].color, color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2S:
         if (vertex) { glhckSetV2(&object->vertices.v2s[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v2s[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v2s[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v2s[ix].color, color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V3FS:
         if (vertex) { glhckSetV3(&object->vertices.v3fs[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v3fs[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v3fs[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v3fs[ix].color, color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2FS:
         if (vertex) { glhckSetV2(&object->vertices.v2fs[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v2fs[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v2fs[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v2fs[ix].color, color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V3F:
         if (vertex) { glhckSetV3(&object->vertices.v3f[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v3f[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v3f[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v3f[ix].color, color, sizeof(glhckColorb)); }
         break;

      case GLHCK_VERTEX_V2F:
         if (vertex) { glhckSetV2(&object->vertices.v2f[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&object->vertices.v2f[ix].normal, normal); }
         if (coord)  { glhckSetV2(&object->vertices.v2f[ix].coord, coord); }
         if (color)  { memcpy(&object->vertices.v2f[ix].color, color, sizeof(glhckColorb)); }
         break;

      default:
         break;
   }
}

/* \brief transform coordinates with vec4 (off x, off y, width, height) and rotation */
GLHCKAPI void glhckGeometryTransformCoordinates(glhckGeometry *object, const glhckRect *transformed, short degrees)
{
   int i;
   kmVec2 out, center = { 0.5f, 0.5f };
   glhckVector2f coord;
   glhckCoordTransform *newCoords;
   CALL(2, "%p, %p, %d", object, transformed, degrees);
   assert(object);

   if (transformed->w == 0.f || transformed->h == 0.f)
      return;

   if (!(newCoords = _glhckMalloc(sizeof(glhckCoordTransform))))
      return;

   /* transform coordinates */
   for (i = 0; i != object->vertexCount; ++i) {
      glhckGeometryGetVertexDataForIndex(object, i, NULL, NULL, &coord, NULL);
      out.x = coord.x/(float)object->textureRange;
      out.y = coord.y/(float)object->textureRange;

      if (object->transformedCoordinates) {
         if (object->transformedCoordinates->degrees != 0)
            kmVec2RotateBy(&out, &out, -object->transformedCoordinates->degrees, &center);

         out.x -= object->transformedCoordinates->transform.x;
         out.x /= object->transformedCoordinates->transform.w;
         out.y -= object->transformedCoordinates->transform.y;
         out.y /= object->transformedCoordinates->transform.h;
      }

      if (degrees != 0) kmVec2RotateBy(&out, &out, degrees, &center);
      out.x *= transformed->w;
      out.x += transformed->x;
      out.y *= transformed->h;
      out.y += transformed->y;

      coord.x = out.x*object->textureRange;
      coord.y = out.y*object->textureRange;
      glhckGeometrySetVertexDataForIndex(object, i, NULL, NULL, &coord, NULL);
   }

   /* assign to geometry */
   newCoords->degrees   = degrees;
   newCoords->transform = *transformed;
   IFDO(_glhckFree, object->transformedCoordinates);
   object->transformedCoordinates = newCoords;
}

/* \brief calculate object's bounding box */
GLHCKAPI void glhckGeometryCalculateBB(glhckGeometry *object, kmAABB *bb)
{
   int i;
   glhckVector3f vertex;
   glhckVector3f min, max;
   CALL(2, "%p, %p", object, bb);
   assert(object && bb);

   glhckGeometryGetVertexDataForIndex(object, 0, &vertex, NULL, NULL, NULL);
   glhckSetV3(&max, &vertex);
   glhckSetV3(&min, &vertex);

   /* find min and max vertices */
   for(i = 1; i != object->vertexCount; ++i) {
      glhckGeometryGetVertexDataForIndex(object, i, &vertex, NULL, NULL, NULL);
      glhckMaxV3(&max, &vertex);
      glhckMinV3(&min, &vertex);
   }

   glhckSetV3(&bb->min, &min);
   glhckSetV3(&bb->max, &max);
}

/* \brief assign indices to object */
GLHCKAPI int glhckGeometryInsertIndices(glhckGeometry *object,
      glhckGeometryIndexType type, const void *data, int memb)
{
   void *idata;
   int size;
   CALL(0, "%p, %d, %p, %d", object, type, data, memb);
   assert(object);

   if (data) {
      /* check index type */
      type = _glhckGeometryCheckIndexType(data, memb, type);
      size = memb * glhckIndexTypeElementSize(type);
      if (!(idata = _glhckCopy(data, size)))
         goto fail;

      _glhckGeometrySetIndices(object, type, idata, memb);
   } else {
      _glhckGeometryFreeIndices(object);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief assign vertices to object */
GLHCKAPI int glhckGeometryInsertVertices(glhckGeometry *object,
      glhckGeometryVertexType type, const void *data, int memb)
{
   void *vdata;
   int size;
   CALL(0, "%p, %d, %p, %d", object, type, data, memb);
   assert(object);

   if (data) {
      /* check vertex type */
      type = _glhckGeometryCheckVertexType(type);
      size = memb * glhckVertexTypeElementSize(type);
      if (!(vdata = _glhckCopy(data, size)))
         goto fail;

      _glhckGeometrySetVertices(object, type, vdata, memb);
   } else {
      _glhckGeometryFreeVertices(object);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
