/* danpLo.c - one line definition */

/* Copyright Plan-S Satellite and Space Technologies Inc. All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "../danp_debug.h"
#include "danp/drivers/danp_lo.h"
#include <assert.h>
#include <stdio.h>

/* Imports */


/* Definitions */

#define DANP_LO_STACK_SIZE               (1024 * 4)
#define DANP_LO_TIMEOUT_MS               (5000)

/* Types */

typedef struct danp_lo_context_s
{
    osalMessageQueueHandle_t mq;
    danp_lo_interface_t *iface;
} danp_lo_context_t;

/* Forward Declarations */


/* Variables */

danp_lo_context_t danp_lo_context;

/* Functions */

int32_t danp_lo_tx(void *iface_common, danp_packet_t *packet)
{
    danp_lo_interface_t *lo_iface = (danp_lo_interface_t *)iface_common;
    danp_lo_context_t *ctx = (danp_lo_context_t *)lo_iface->context;

    danp_log_message(
        DANP_LOG_VERBOSE,
        "LO TX: dst=%u port=%u flags=0x%02X len=%u",
        (packet->header_raw >> 22) & 0xFF,
        (packet->header_raw >> 8) & 0x3F,
        packet->header_raw & 0x03,
        packet->length);

    osalStatus_t osalStatus = osalMessageQueueSend(ctx->mq, packet, OSAL_WAIT_FOREVER);
    if (osalStatus != OSAL_SUCCESS)
    {
        danp_log_message(DANP_LOG_ERROR, "DANP LO: Failed to enqueue packet for RX");
        return -1;
    }

    return 0;
}

void *danp_lo_rx_routine(void *arg)
{
    danp_lo_interface_t *lo_iface = (danp_lo_interface_t *)arg;
    danp_lo_context_t *ctx = (danp_lo_context_t *)lo_iface->context;

    for (;;)
    {
        danp_packet_t pkt = {0};
        if (0 == osalMessageQueueReceive(ctx->mq, &pkt, DANP_LO_TIMEOUT_MS))
        {
            danp_log_message(
                DANP_LOG_VERBOSE,
                "LO RX: dst=%u port=%u flags=0x%02X len=%u",
                (pkt.header_raw >> 22) & 0xFF,
                (pkt.header_raw >> 8) & 0x3F,
                pkt.header_raw & 0x03,
                pkt.length);

            danp_input(&lo_iface->common, (uint8_t *)&pkt, pkt.length + sizeof(pkt.header_raw));
        }
    }
}

int32_t danp_lo_init (danp_lo_interface_t *iface, uint16_t address)
{
    int32_t ret = 0;
    osalThreadHandle_t thread_handle = NULL;
    osalThreadAttr_t thread_attr =
    {
        .name = "danpLoCtx",
        .stackSize = DANP_LO_STACK_SIZE,
        .stackMem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cbMem = NULL,
        .cbSize = 0,
    };
    osalMessageQueueAttr_t mq_attr =
    {
        .name = "danpLoMq",
        .mqMem = NULL,
        .mqSize = 0,
        .cbMem = NULL,
        .cbSize = 0,
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
        iface->common.tx_func = danp_lo_tx;
        iface->common.name = "Loopback";
        iface->common.mtu = DANP_MAX_PACKET_SIZE;
        iface->context = &danp_lo_context;

        danp_lo_context.iface = iface;

        danp_lo_context.mq = osalMessageQueueCreate(2, sizeof(danp_packet_t), &mq_attr);
        if (danp_lo_context.mq == NULL)
        {
            danp_log_message(DANP_LOG_ERROR, "DANP LO: Failed to create message queue");
            ret = -1;
            break;
        }

        thread_handle = osalThreadCreate(
            danp_lo_rx_routine,
            iface,
            &thread_attr);
        if (!thread_handle)
        {
            danp_log_message(DANP_LOG_ERROR, "DANP LO: Failed to create RX thread");
            ret = -1;
            break;
        }

    } while (0);

    return ret;
}
