#include "danp/danp.h"
#include "osal/osal.h"
#include <stdio.h>
#include <string.h>

// Defines a fake interface
static danpInterface_t MockIface;
static uint16_t MyNodeId = 1;

// The TX function simply feeds data back into the Input
int32_t mockLoopbackTx(danpInterface_t *iface, danpPacket_t *packet)
{
    uint8_t buffer[DANP_MAX_PACKET_SIZE + 4];

    // Serialize just like a real driver would
    memcpy(buffer, &packet->headerRaw, 4);
    if (packet->length > 0)
    {
        memcpy(buffer + 4, packet->payload, packet->length);
    }

    // Simulate "Network Delay" (Optional, usually 0 for unit tests)

    // Feed back into core
    // We treat this as a perfect loopback: If I send it, I receive it.
    // The Core's input filter will decide if it's actually for me (Dst == MyID).
    danpInput(iface, buffer, 4 + packet->length);

    return 0;
}

void mockDriverInit(uint16_t nodeId)
{
    MyNodeId = nodeId;
    MockIface.name = "LOOPBACK";
    MockIface.address = nodeId;
    MockIface.mtu = 128;
    MockIface.txFunc = mockLoopbackTx;

    danpRegisterInterface(&MockIface);
}
