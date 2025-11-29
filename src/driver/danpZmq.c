/* danpZmq.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "../danpDebug.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <zmq.h>

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

static void *ZmqContext = NULL;
static void *PubSock = NULL;
static void *SubSock = NULL;
static danpInterface_t ZmqIface;
static pthread_t RxThreadId;

/* Functions */

int32_t danpZmqTx(danpInterface_t *iface, danpPacket_t *packet)
{
    UNUSED(iface);
    uint8_t buffer[DANP_MAX_PACKET_SIZE + 4];
    memcpy(buffer, &packet->headerRaw, 4);
    if (packet->length > 0)
    {
        memcpy(buffer + 4, packet->payload, packet->length);
    }

    // Manually unpack for logging (Shift values changed!)
    uint16_t dst = (packet->headerRaw >> 22) & 0xFF;
    uint8_t dPort = (packet->headerRaw >> 8) & 0x3F;
    uint8_t flags = packet->headerRaw & 0x03;

    danpLogMessage(
        DANP_LOG_VERBOSE,
        "ZMQ TX: dst=%u port=%u flags=0x%02X len=%u",
        dst,
        dPort,
        flags,
        packet->length);

    // ZMQ Topic    // 1. Send Topic (2 bytes, NodeID)
    uint16_t topic = packet->headerRaw >> 22 & 0xFF; // Extract DST Node
    zmq_send(PubSock, &topic, 2, ZMQ_SNDMORE);
    zmq_send(PubSock, buffer, 4 + packet->length, 0);
    return 0;
}

void *zmqRxRoutine(void *arg)
{
    UNUSED(arg);
    while (1)
    {
        zmq_recv(SubSock, NULL, 0, 0);
        uint8_t buffer[256];
        int32_t len = zmq_recv(SubSock, buffer, sizeof(buffer), 0);
        if (len >= 4)
        {
            uint32_t headerRaw;
            memcpy(&headerRaw, buffer, 4);

            uint16_t dst = (headerRaw >> 22) & 0xFF;
            uint8_t dPort = (headerRaw >> 8) & 0x3F;
            uint8_t flags = headerRaw & 0x03;

            danpLogMessage(
                DANP_LOG_VERBOSE,
                "ZMQ RX: dst=%u port=%u flags=0x%02X len=%d",
                dst,
                dPort,
                flags,
                len - 4);
            danpInput(&ZmqIface, buffer, len);
        }
        else
        {
            danpLogMessage(DANP_LOG_WARN, "ZMQ RX: received packet too short");
        }
    }
    return NULL;
}

void danpZmqInit(
    const char *pubBindEndpoint,
    const char **subConnectEndpoints,
    int32_t subCount,
    uint16_t nodeId)
{
    ZmqContext = zmq_ctx_new();

    // PUB
    PubSock = zmq_socket(ZmqContext, ZMQ_PUB);
    zmq_bind(PubSock, pubBindEndpoint);

    // SUB
    SubSock = zmq_socket(ZmqContext, ZMQ_SUB);
    for (int i = 0; i < subCount; i++)
    {
        zmq_connect(SubSock, subConnectEndpoints[i]);
    }

    // Filter: Subscribe to My Node ID
    uint16_t topic = nodeId;
    zmq_setsockopt(SubSock, ZMQ_SUBSCRIBE, &topic, 2);

    // Also subscribe to Broadcast (0xFF) if needed, or Promiscuous mode
    // For now, just specific node.

    ZmqIface.name = "ZMQ";
    ZmqIface.address = nodeId;
    ZmqIface.mtu = DANP_MAX_PACKET_SIZE;
    ZmqIface.txFunc = danpZmqTx;

    danpRegisterInterface(&ZmqIface);

    pthread_create(&RxThreadId, NULL, zmqRxRoutine, NULL);
}
