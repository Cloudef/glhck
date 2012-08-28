#include "../internal.h"
#include <assert.h>    /* for assert */

#define GLHCK_CHANNEL GLHCK_CHANNEL_NETWORK

/* define for this feature */
#if GLHCK_USE_NETWORK

#include "packets.h"
#include <enet/enet.h> /* for enet   */

/* \brief local server struct */
typedef struct __GLHCKserver {
   ENetHost *enet;
} __GLHCKserver;
static __GLHCKserver _GLHCKserver;
static char _glhckServerInitialized = 0;

/* \brief initialize enet internally */
static int _glhckEnetInit(const char *host, int port)
{
   ENetAddress address;
   CALL(0, "%s, %d", host, port);

   /* initialize enet */
   if (enet_initialize() != 0)
      goto enet_init_fail;

   /* set host parameters */
   address.host = ENET_HOST_ANY;
   address.port = port;

   /* set host address, if specified */
   if (host) enet_address_set_host(&address, host);

   /* create host */
   _GLHCKserver.enet = enet_host_create(&address,
         32    /* max clients */,
         1     /* max channels */,
         0     /* download bandwidth */,
         0     /* upload bandwidth */);

   if (!_GLHCKserver.enet)
      goto enet_host_fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

enet_init_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to initialize ENet");
   goto fail;
enet_host_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to create ENet host");
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief destroy enet internally */
static void _glhckEnetDestroy(void)
{
   TRACE(0);

   if (!_GLHCKserver.enet)
      return;

   /* kill */
   enet_host_destroy(_GLHCKserver.enet);
   _GLHCKserver.enet = NULL;
}

/* convert network bams to vector */
static void _glhckNetBamsToVec3(kmVec3 *km3d, const _glhckNetVector3d *nv3d)
{
   unsigned int x = ntohl(nv3d->x)<<16;
   unsigned int y = ntohl(nv3d->y)<<16;
   unsigned int z = ntohl(nv3d->z)<<16;
   km3d->x = *((float*)&x);
   km3d->y = *((float*)&y);
   km3d->z = *((float*)&z);
}

static void _glhckServerManagePacketObject(unsigned char *data)
{
   size_t i;
   unsigned char         *offset;
   unsigned int          *indices;
   _glhckNetVertexData   *vertexData;
   _glhckNetObjectPacket *packet = (_glhckNetObjectPacket*)data;
   _glhckNetObjectPacket object;
   memset(&object, 0, sizeof(_glhckNetObjectPacket));
   memcpy(&object.view, &packet->view, sizeof(_glhckNetView));
   memcpy(&object.material.color, &packet->material.color, sizeof(_glhckNetColor));

   object.geometry.indicesCount  = ntohl(packet->geometry.indicesCount);
   object.geometry.vertexCount   = ntohl(packet->geometry.vertexCount);
   object.geometry.type          = ntohl(packet->geometry.type);
   object.geometry.flags         = ntohl(packet->geometry.flags);
   object.material.flags         = ntohl(packet->material.flags);

   kmVec3 bias, scale;
   _glhckNetBamsToVec3(&bias,  &packet->geometry.bias);
   _glhckNetBamsToVec3(&scale, &packet->geometry.scale);

   kmVec3 translation, target, rotation, scaling;
   _glhckNetBamsToVec3(&translation,  &packet->view.translation);
   _glhckNetBamsToVec3(&target,       &packet->view.target);
   _glhckNetBamsToVec3(&rotation,     &packet->view.rotation);
   _glhckNetBamsToVec3(&scaling,      &packet->view.scaling);

   printf("- Object Packet\n");
   printf("[] Geometry\n");
   printf("   :: Indices: %zu\n", object.geometry.indicesCount);
   printf("   :: Vertices: %zu\n", object.geometry.vertexCount);
   printf("   :: Bias: "VEC3S"\n", VEC3(&bias));
   printf("   :: Scale: "VEC3S"\n", VEC3(&scale));
   printf("   :: Type: %u\n", object.geometry.type);
   printf("   :: Flags: %u\n", object.geometry.flags);
   printf("[] View\n");
   printf("   :: Translation: "VEC3S"\n", VEC3(&translation));
   printf("   :: Target: "VEC3S"\n", VEC3(&target));
   printf("   :: Rotation: "VEC3S"\n", VEC3(&rotation));
   printf("   :: Scaling: "VEC3S"\n", VEC3(&scaling));
   printf("[] Material\n");
   printf("   :: Color: [%d, %d, %d, %d]\n",
         object.material.color.r,
         object.material.color.g,
         object.material.color.b,
         object.material.color.a);
   printf("   :: Flags: %u\n", object.material.flags);

   /* raise offset */
   offset = data + sizeof(_glhckNetObjectPacket);

   /* vertex data is appended next */
   if (object.geometry.vertexCount) {
      vertexData = (_glhckNetVertexData*)offset;
      printf("[] Vertex Data\n");
      for (i = 0; i != object.geometry.vertexCount; ++i) {
         kmVec3 vertex, normal;
         kmVec2 coord;
         _glhckNetBamsToVec3(&vertex, &vertexData[i].vertex);
         _glhckNetBamsToVec3(&normal, &vertexData[i].normal);
         printf("   :: Vertex: "VEC3S"\n", VEC3(&vertex));
         printf("   :: Normal: "VEC3S"\n", VEC3(&normal));
      }
   }

   /* vertex data offset */
   offset = offset + object.geometry.vertexCount * sizeof(_glhckNetVertexData);

   if (object.geometry.indicesCount) {
      indices = (unsigned int**)offset;
      printf("[] Indices\n");
      for (i = 0; i != object.geometry.indicesCount; ++i) {
         printf("%u%s", indices[i], (i!=object.geometry.indicesCount?", ":"\n"));
      }
   }

   printf("EOF\n");
}

/* \brief update enet state internally */
static int _glhckEnetUpdate(void)
{
   ENetEvent event;
   TRACE(2);

   /* wait up to 1000 milliseconds for an event */
   while (enet_host_service(_GLHCKserver.enet, &event, 1000) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_CONNECT:
            printf("A new client connected from %x:%u.\n",
                  event.peer->address.host,
                  event.peer->address.port);

            /* Store any relevant client information here. */
            event.peer->data = "Client information";
            break;

         case ENET_EVENT_TYPE_RECEIVE:

            /* manage packet by kind */
            printf("ID: %d\n", ((_glhckNetPacket*)event.packet->data)->type);
            switch (((_glhckNetPacket*)event.packet->data)->type) {
               case GLHCK_NET_PACKET_OBJECT:
                  _glhckServerManagePacketObject(event.packet->data);
                  break;

               default:
                  printf("A packet of length %zu containing %s was received from %s on channel %u.\n",
                     event.packet->dataLength,
                     (char*)event.packet->data,
                     (char*)event.peer->data,
                     event.channelID);
                  break;
            }

            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(event.packet);
            break;

         case ENET_EVENT_TYPE_DISCONNECT:
            printf("%s disconected.\n", (char*)event.peer->data);

            /* Reset the peer's client information. */
            event.peer->data = NULL;
            break;

         default:
            break;
      }
   }

   RET(2, "%d", RETURN_OK);
   return RETURN_OK;
}

/* public api */

/* \brief initialize glhck server */
GLHCKAPI int glhckServerInit(const char *host, int port)
{
   CALL(0, "%s, %d", host, port);

   /* reinit, if initialized */
   if (_glhckServerInitialized)
      glhckServerKill();

   /* null struct */
   memset(&_GLHCKserver, 0, sizeof(__GLHCKserver));

   /* initialize enet */
   if (_glhckEnetInit(host, port) != RETURN_OK)
      goto fail_host;

   _glhckServerInitialized = 1;
   DEBUG(GLHCK_DBG_CRAP, "Started ENet server [%s:%d]",
         host?host:"0.0.0.0", port);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail_host:
   DEBUG(GLHCK_DBG_ERROR, "Failed to create ENet server [%s:%d]",
         host?host:"0.0.0.0", port);
fail:
   _glhckEnetDestroy();
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief kill glhck server */
GLHCKAPI void glhckServerKill(void)
{
   TRACE(0);

   /* is initialized? */
   if (!_glhckServerInitialized)
      return;

   /* kill enet */
   _glhckEnetDestroy();

   _glhckServerInitialized = 0;
   DEBUG(GLHCK_DBG_CRAP, "Killed ENet server");
}

/* \brief update server state */
GLHCKAPI void glhckServerUpdate(void)
{
   TRACE(2);

   /* is initialized? */
   if (!_glhckServerInitialized)
      return;

   /* updat enet */
   _glhckEnetUpdate();
}

#else /* ^ network support defined */

static void _glhckServerStub()
{
   DEBUG(GLHCK_DBG_ERROR, "GLhck was not build with network transparency");
}

GLHCKAPI int glhckServerInit(const char *host, int port)
{
   _glhckServerStub();
   return RETURN_FAIL;
}
GLHCKAPI void glhckServerKill(void)
{
   _glhckServerStub();
}
GLHCKAPI void glhckServerUpdate(void)
{
   _glhckServerStub();
}

#endif /* GLHCK_USE_NETWORK */

/* vim: set ts=8 sw=3 tw=0 :*/
