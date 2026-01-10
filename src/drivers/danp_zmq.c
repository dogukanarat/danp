/* danpZmq.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include <zmq.h>
#include "osal/osal_thread.h"
#include "osal/osal_time.h"

#include "danp/danp.h"
#include "danp/danp_buffer.h"
#include "../danp_debug.h"
#include "danp/drivers/danp_zmq.h"

/* LCOV_EXCL_START */ /* Requires live ZMQ endpoints; excluded from unit coverage */

/* Imports */


/* Definitions */

#define DANP_DRIVER_ZMQ_STACK_SIZE (1024 * 4)
#define DANP_DRIVER_ZMQ_TIMEOUT_MS (5000)

/* Types */


/* Forward Declarations */


/* Variables */

static void *zmq_context = NULL;

/* Functions */

static int32_t danp_zmq_tx(void *iface_common, danp_packet_t *packet)
{
    danp_zmq_interface_t *iface = (danp_zmq_interface_t *)iface_common;
    uint8_t buffer[DANP_MAX_PACKET_SIZE + 4];
    memcpy(buffer, &packet->header_raw, 4);
    if (packet->length > 0)
    {
        memcpy(buffer + 4, packet->payload, packet->length);
    }

    // Manually unpack for logging (Shift values changed!)
    uint16_t dst = (packet->header_raw >> 22) & 0xFF;
    uint8_t d_port = (packet->header_raw >> 8) & 0x3F;
    uint8_t flags = packet->header_raw & 0x03;

    DANP_LOG_VER(
        "ZMQ TX: dst=%u port=%u flags=0x%02X len=%u",
        dst,
        d_port,
        flags,
        packet->length);

    // ZMQ Topic    // 1. Send Topic (2 bytes, NodeID)
    uint16_t topic = packet->header_raw >> 22 & 0xFF; // Extract DST Node
    zmq_send(iface->pub_sock, &topic, 2, ZMQ_SNDMORE);
    zmq_send(iface->pub_sock, buffer, 4 + packet->length, 0);
    return 0;
}

static void danp_zmq_rx_routine(void *arg)
{
    danp_zmq_interface_t *zmq_iface = (danp_zmq_interface_t *)arg;
    danp_packet_t *pkt = NULL;

    for (;;)
    {
        if (zmq_iface->common.data.is_exiting)
        {
            DANP_LOG_IO_INF("ZMQ RX: Exiting RX thread");
            break;
        }

        if (false == zmq_iface->common.data.is_running)
        {
            osal_delay_ms(1000);
            continue;
        }

        if (NULL == pkt)
        {
            pkt = (danp_packet_t *)danp_buffer_get();
            if (NULL == pkt)
            {
                DANP_LOG_IO_ERR("ZMQ RX: Failed to allocate packet buffer");
                osal_delay_ms(1000);
                continue;
            }
        }

        zmq_recv(zmq_iface->sub_sock, NULL, 0, 0);
        uint8_t buffer[256];
        int32_t len = zmq_recv(zmq_iface->sub_sock, buffer, sizeof(buffer), 0);
        if (len >= (int32_t)sizeof(pkt->header_raw))
        {
            memcpy(&pkt->header_raw, buffer, sizeof(pkt->header_raw));
            pkt->length = len - sizeof(pkt->header_raw);
            if (pkt->length > 0)
            {
                memcpy(pkt->payload, buffer + sizeof(pkt->header_raw), pkt->length);
            }

            uint16_t dst = (pkt->header_raw >> 22) & 0xFF;
            uint8_t d_port = (pkt->header_raw >> 8) & 0x3F;
            uint8_t flags = pkt->header_raw & 0x03;

            DANP_LOG_IO_VER(
                "ZMQ RX: [dst]=%u, [port]=%u [flags]=0x%02X [len]=%u",
                dst,
                d_port,
                flags,
                pkt->length);

            danp_input(&zmq_iface->common, pkt);
            pkt = NULL; // Ownership transferred to danp_input
        }
        else
        {
            DANP_LOG_IO_WRN("ZMQ RX: received packet too short");
        }
    }
}

int32_t danp_zmq_init(
    danp_zmq_interface_t *iface,
    const char *pub_bind_endpoint,
    const char **sub_connect_endpoints,
    int32_t sub_count,
    uint16_t node_id)
{
    int32_t ret = 0;
    osal_thread_handle_t thread_handle = NULL;
    osal_thread_attr_t thread_attr = {
        .name = "danpZmqCtx",
        .stack_size = DANP_DRIVER_ZMQ_STACK_SIZE,
        .stack_mem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cb_mem = NULL,
        .cb_size = 0,
    };

    for (;;)
    {
        if (NULL == iface)
        {
            ret = -1;
            DANP_LOG_ERR("Invalid parameters to danp_zmq_init");
            break;
        }

        memset(iface, 0, sizeof(danp_zmq_interface_t));

        if (zmq_context == NULL)
        {
            zmq_context = zmq_ctx_new();
            if (zmq_context == NULL)
            {
                ret = -1;
                DANP_LOG_ERR("Failed to create ZMQ context");
                break;
            }
        }

        // PUB
        iface->pub_sock = zmq_socket(zmq_context, ZMQ_PUB);
        zmq_bind(iface->pub_sock, pub_bind_endpoint);

        // SUB
        iface->sub_sock = zmq_socket(zmq_context, ZMQ_SUB);
        for (int i = 0; i < sub_count; i++)
        {
            zmq_connect(iface->sub_sock, sub_connect_endpoints[i]);
        }

        // Filter: Subscribe to My Node ID
        uint16_t topic = node_id;
        zmq_setsockopt(iface->sub_sock, ZMQ_SUBSCRIBE, &topic, 2);

        // Also subscribe to Broadcast (0xFF) if needed, or Promiscuous mode
        // For now, just specific node.

        iface->common.address = node_id;
        iface->common.ops.tx = danp_zmq_tx;
        iface->common.data.is_exiting = false;
        iface->common.data.is_running = false;
        iface->common.name = "ZMQ";
        iface->common.mtu = DANP_MAX_PACKET_SIZE;

        thread_handle = osal_thread_create(danp_zmq_rx_routine, iface, &thread_attr);
        if (NULL == thread_handle)
        {
            ret = -1;
            DANP_LOG_ERR("Failed to create ZMQ RX thread");
            break;
        }

        DANP_LOG_INF("ZMQ interface initialized at address %d", node_id);

        break;
    }

    return ret;
}

/* LCOV_EXCL_STOP */
