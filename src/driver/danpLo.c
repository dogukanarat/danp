/* danpLo.c - one line definition */

/* Copyright Plan-S Satellite and Space Technologies Inc. All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "../danpDebug.h"
#include "danp/drivers/danpLo.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/* Functions */

int32_t danpLoTx (void *ifaceCommon, danpPacket_t *packet)
{
    danpLoInterface_t *loIface = (danpLoInterface_t *)ifaceCommon;
    uint8_t buffer[DANP_MAX_PACKET_SIZE + 4];
    memcpy(buffer, &packet->headerRaw, 4);
    if (packet->length > 0)
    {
        memcpy(buffer + 4, packet->payload, packet->length);
    }

    danpLogMessage(
        DANP_LOG_VERBOSE,
        "LO TX: dst=%u port=%u flags=0x%02X len=%u",
        (packet->headerRaw >> 22) & 0xFF,
        (packet->headerRaw >> 8) & 0x3F,
        packet->headerRaw & 0x03,
        packet->length);

    danpInput(&loIface->common, buffer, 4 + packet->length);
    return 0;
}

int32_t danpLoInit (danpLoInterface_t *iface, uint16_t address)
{
    if (iface == NULL)
    {
        return -1;
    }

    memset(iface, 0, sizeof(danpLoInterface_t));
    iface->common.address = address;
    iface->common.txFunc = danpLoTx;
    iface->common.name = "Loopback";
    iface->common.mtu = DANP_MAX_PACKET_SIZE;

    return 0;
}
