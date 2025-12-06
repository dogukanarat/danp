/* danpLo.c - one line definition */

/* Copyright Plan-S Satellite and Space Technologies Inc. All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "../danpDebug.h"
#include "danp/drivers/danpLo.h"
#include <assert.h>
#include <stdio.h>

/* Imports */


/* Definitions */

#define DANP_LO_STACK_SIZE               (1024 * 4)
#define DANP_LO_TIMEOUT_MS               (5000)

/* Types */

typedef struct danpLoContext_s
{
    osalMessageQueueHandle_t mq;
    danpLoInterface_t *iface;
} danpLoContext_t;

/* Forward Declarations */


/* Variables */

danpLoContext_t DanpLoContext;

/* Functions */

int32_t danpLoTx (void *ifaceCommon, danpPacket_t *packet)
{
    danpLoInterface_t *loIface = (danpLoInterface_t *)ifaceCommon;
    danpLoContext_t *ctx = (danpLoContext_t *)loIface->context;

    danpLogMessage(
        DANP_LOG_VERBOSE,
        "LO TX: dst=%u port=%u flags=0x%02X len=%u",
        (packet->headerRaw >> 22) & 0xFF,
        (packet->headerRaw >> 8) & 0x3F,
        packet->headerRaw & 0x03,
        packet->length);

    osalStatus_t osalStatus = osalMessageQueueSend(ctx->mq, packet, OSAL_WAIT_FOREVER);
    if (osalStatus != OSAL_SUCCESS)
    {
        danpLogMessage(DANP_LOG_ERROR, "DANP LO: Failed to enqueue packet for RX");
        return -1;
    }

    return 0;
}

void *danpLoRxRoutine(void *arg)
{
    danpLoInterface_t *loIface = (danpLoInterface_t *)arg;
    danpLoContext_t *ctx = (danpLoContext_t *)loIface->context;

    for (;;)
    {
        danpPacket_t pkt = {0};
        if (0 == osalMessageQueueReceive(ctx->mq, &pkt, DANP_LO_TIMEOUT_MS))
        {
            danpLogMessage(
                DANP_LOG_VERBOSE,
                "LO RX: dst=%u port=%u flags=0x%02X len=%u",
                (pkt.headerRaw >> 22) & 0xFF,
                (pkt.headerRaw >> 8) & 0x3F,
                pkt.headerRaw & 0x03,
                pkt.length);

            danpInput(&loIface->common, (uint8_t *)&pkt, pkt.length + sizeof(pkt.headerRaw));
        }
    }
}

int32_t danpLoInit (danpLoInterface_t *iface, uint16_t address)
{
    int32_t ret = 0;
    osalThreadHandle_t threadHandle = NULL;
    osalThreadAttr_t threadAttr =
    {
        .name = "danpLoCtx",
        .stackSize = DANP_LO_STACK_SIZE,
        .stackMem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cbMem = NULL,
        .cbSize = 0,
    };
    osalMessageQueueAttr_t mqAttr =
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

        memset(iface, 0, sizeof(danpLoInterface_t));
        iface->common.address = address;
        iface->common.txFunc = danpLoTx;
        iface->common.name = "Loopback";
        iface->common.mtu = DANP_MAX_PACKET_SIZE;
        iface->context = &DanpLoContext;

        DanpLoContext.iface = iface;

        DanpLoContext.mq = osalMessageQueueCreate(2, sizeof(danpPacket_t), &mqAttr);
        if (DanpLoContext.mq == NULL)
        {
            danpLogMessage(DANP_LOG_ERROR, "DANP LO: Failed to create message queue");
            ret = -1;
            break;
        }

        threadHandle = osalThreadCreate(
            danpLoRxRoutine,
            iface,
            &threadAttr);
        if (!threadHandle)
        {
            danpLogMessage(DANP_LOG_ERROR, "DANP LO: Failed to create RX thread");
            ret = -1;
            break;
        }

    } while (0);

    return ret;
}
