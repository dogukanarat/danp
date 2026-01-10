/* danpLo.c - one line definition */

/* Copyright Plan-S Satellite and Space Technologies Inc. All Rights Reserved */

/* Includes */

#include <assert.h>
#include <stdio.h>
#include "osal/osal_thread.h"
#include "osal/osal_time.h"
#include "osal/osal_message_queue.h"
#include "danp/danp.h"
#include "danp/danp_buffer.h"
#include "../danp_debug.h"
#include "danp/drivers/danp_lo.h"

/* Imports */


/* Definitions */

#define DANP_DRIVER_LO_STACK_SIZE               (1024 * 4)
#define DANP_DRIVER_LO_TIMEOUT_MS               (5000)

/* Types */

typedef struct danp_lo_context_s
{
    osal_message_queue_handle_t mq;
    danp_lo_interface_t *iface;
} danp_lo_context_t;

/* Forward Declarations */


/* Variables */

danp_lo_context_t danp_lo_context;

/* Functions */

static int32_t danp_lo_tx(void *iface_common, danp_packet_t *packet)
{
    danp_lo_interface_t *lo_iface = (danp_lo_interface_t *)iface_common;
    danp_lo_context_t *ctx = (danp_lo_context_t *)lo_iface->context;

    DANP_LOG_VER(
        "LO TX: dst=%u port=%u flags=0x%02X len=%u",
        (packet->header_raw >> 22) & 0xFF,
        (packet->header_raw >> 8) & 0x3F,
        packet->header_raw & 0x03,
        packet->length);

    osal_status_t osalStatus = osal_message_queue_send(ctx->mq, packet, OSAL_WAIT_FOREVER);
    if (osalStatus != OSAL_SUCCESS)
    {
        DANP_LOG_ERR("DANP LO: Failed to enqueue packet for RX");
        return -1;
    }

    return 0;
}

static void danp_lo_rx_routine(void *arg)
{
    danp_lo_interface_t *lo_iface = (danp_lo_interface_t *)arg;
    danp_lo_context_t *ctx = (danp_lo_context_t *)lo_iface->context;
    danp_packet_t *pkt = NULL;

    for (;;)
    {
        if (lo_iface->common.data.is_exiting)
        {
            DANP_LOG_IO_INF("Radio RX: Exiting RX thread");
            break;
        }

        if (false == lo_iface->common.data.is_running)
        {
            osal_delay_ms(1000);
            continue;
        }

        if (NULL == pkt)
        {
            pkt = (danp_packet_t *)danp_buffer_get();
            if (NULL == pkt)
            {
                DANP_LOG_IO_ERR("Radio RX: Failed to allocate packet buffer");
                osal_delay_ms(1000);
                continue;
            }
        }

        if (0 == osal_message_queue_receive(ctx->mq, &pkt, DANP_DRIVER_LO_TIMEOUT_MS))
        {
            DANP_LOG_IO_VER("Loopback RX: Incoming packet: [len]=%u", pkt->length);
            danp_input(&lo_iface->common, pkt);
        }
    }
}

int32_t danp_lo_init (danp_lo_interface_t *iface, uint16_t address)
{
    int32_t ret = 0;
    osal_thread_handle_t thread_handle = NULL;
    osal_thread_attr_t thread_attr =
    {
        .name = "danpLoCtx",
        .stack_size = DANP_DRIVER_LO_STACK_SIZE,
        .stack_mem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cb_mem = NULL,
        .cb_size = 0,
    };
    osal_message_queue_attr_t mq_attr =
    {
        .name = "danpLoMq",
        .mq_mem = NULL,
        .mq_size = 0,
        .cb_mem = NULL,
        .cb_size = 0,
    };

    do
    {
        if (iface == NULL)
        {
            ret = -1;
            break;
        }

        memset(iface, 0, sizeof(danp_lo_interface_t));
        iface->common.address = address;
        iface->common.ops.tx = danp_lo_tx;
        iface->common.data.is_exiting = false;
        iface->common.data.is_running = false;
        iface->common.name = "Loopback";
        iface->common.mtu = DANP_MAX_PACKET_SIZE;
        iface->context = &danp_lo_context;

        danp_lo_context.iface = iface;

        danp_lo_context.mq = osal_message_queue_create(2, sizeof(danp_packet_t), &mq_attr);
        if (danp_lo_context.mq == NULL)
        {
            DANP_LOG_ERR("DANP LO: Failed to create message queue");
            ret = -1;
            break;
        }

        thread_handle = osal_thread_create(
            danp_lo_rx_routine,
            iface,
            &thread_attr);
        if (!thread_handle)
        {
            DANP_LOG_ERR("DANP LO: Failed to create RX thread");
            ret = -1;
            break;
        }

    } while (0);

    return ret;
}
