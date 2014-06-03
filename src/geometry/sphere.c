#include <glhck/glhck.h>
#include "trace.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new sphere object */
GLHCKAPI glhckHandle glhckSphereNew(const kmScalar size)
{
   return glhckEllipsoidNew(size, size, size);
}

/* \brief create new sphere object (specify precision) */
GLHCKAPI glhckHandle glhckSphereNewEx(const kmScalar size, const glhckIndexType itype, const glhckVertexType vtype)
{
   return glhckEllipsoidNewEx(size, size, size, itype, vtype);
}

/* \brief create new ellipsoid object */
GLHCKAPI glhckHandle glhckEllipsoidNew(const kmScalar rx, const kmScalar ry, const kmScalar rz)
{
   glhckIndexType itype;
   glhckVertexType vtype;
   glhckGetGlobalPrecision(&itype, &vtype);

   if (vtype == GLHCK_VTX_AUTO)
      vtype = GLHCK_VTX_V3F;

   if (itype == GLHCK_IDX_AUTO)
      itype = GLHCK_IDX_UINT;

   return glhckEllipsoidNewEx(rx, ry, rz, itype, vtype);
}

/* \brief create new ellipsoid object (specify precision) */
GLHCKAPI glhckHandle glhckEllipsoidNewEx(const kmScalar rx, const kmScalar ry, const kmScalar rz, const glhckIndexType itype, const glhckVertexType vtype)
{
   const int bandPower = 4;
   const int bandPoints = bandPower * bandPower;
   const int bandMask = bandPoints - 2;
   const unsigned int sectionsInBand = (bandPoints / 2) - 1;
   const unsigned int totalPoints = sectionsInBand * bandPoints;
   const kmScalar sectionArc = 6.28f / sectionsInBand;
   CALL(0, "%f, %f, %f, %u, %u", rx, ry, rz, itype, vtype);

   /* generate ellipsoid tristrip */
   glhckImportVertexData vertices[totalPoints];
   memset(vertices, 0, sizeof(vertices));
   for (unsigned int i = 0; i < totalPoints; ++i) {
      kmScalar x = (kmScalar)(i & 1) + (i >> bandPower);
      kmScalar y = (kmScalar)((i & bandMask) >> 1) + ((i >> bandPower) * sectionsInBand);
      x *= (kmScalar)sectionArc * 0.5f;
      y *= (kmScalar)sectionArc * -1;

      vertices[i].vertex.x = -rx * sin(x) * sin(y);
      vertices[i].vertex.y = -ry * cos(x);
      vertices[i].vertex.z = -rz * sin(x) * cos(y);
      vertices[i].normal.x = sin(x) * sin(y);
      vertices[i].normal.y = cos(x);
      vertices[i].normal.z = sin(x) * cos(y);
      vertices[i].coord.x = sin(x) * sin(y);
      vertices[i].coord.y = cos(x);
   }

   glhckHandle handle;
   if (!(handle = glhckObjectNew()))
      goto fail;

   if (glhckObjectInsertVertices(handle, vtype, vertices, totalPoints) != RETURN_OK)
      goto fail;

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
