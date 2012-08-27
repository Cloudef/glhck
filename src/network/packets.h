#ifndef __packets_h__
#define __packets_h__

#define VEC3NS "%f,%f,%f"

typedef enum _glhckNetPacketType {
   GLHCK_NET_PACKET_OBJECT,
} _glhckNetPacketType;

typedef struct _glhckNetColor {
   unsigned char r, g, b, a;
} _glhckNetColor;

typedef struct _glhckNetView {
   char translation[32], target[32], rotation[32], scaling[32];
} _glhckNetView;

typedef struct _glhckNetMaterial {
   _glhckNetColor color;
   unsigned int flags;
} _glhckNetMaterial;

typedef struct _glhckNetGeometry {
   size_t indicesCount, vertexCount;
   char bias[32], scale[32];
   unsigned int type, flags;
} _glhckNetGeometry;

typedef struct _glhckNetObjectPacket {
   unsigned char type;
   struct _glhckNetGeometry geometry;
   struct _glhckNetView view;
   struct _glhckNetMaterial material;
} _glhckNetObjectPacket;

typedef struct _glhckNetPacket {
   unsigned char type;
} _glhckNetPacket;

#endif /* __packets_h__ */
