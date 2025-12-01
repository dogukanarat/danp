/* danp.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "danp/danpBuffer.h"
#include "danpDebug.h"
#include <stdarg.h>
#include <stdio.h>

/* Imports */

extern void danpSocketInputHandler(danpPacket_t *pkt);

/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief List of registered network interfaces. */
static danpInterface_t *IfaceList = NULL;

/** @brief Global configuration structure. */
danpConfig_t DanpConfig;

/* Functions */

/**
 * @brief Pack a DANP header.
 * @param prio Packet priority.
 * @param dst Destination node address.
 * @param src Source node address.
 * @param dstPort Destination port.
 * @param srcPort Source port.
 * @param flags Packet flags.
 * @return Packed header as a 32-bit integer.
 */
uint32_t danpPackHeader(
    uint8_t prio,
    uint16_t dst,
    uint16_t src,
    uint8_t dstPort,
    uint8_t srcPort,
    uint8_t flags)
{
    uint32_t h = 0;

    h |= (uint32_t)(prio & 0x01) << 30;

    if (flags & DANP_FLAG_RST)
    {
        h |= (1U << 31);
    }

    // Standard fields
    h |= (uint32_t)(dst & 0xFF) << 22;
    h |= (uint32_t)(src & 0xFF) << 14;
    h |= (uint32_t)(dstPort & 0x3F) << 8;
    h |= (uint32_t)(srcPort & 0x3F) << 2;

    h |= (uint32_t)(flags & 0x03);

    return h;
}

/**
 * @brief Unpack a DANP header.
 * @param raw Raw 32-bit header.
 * @param dst Pointer to store destination node address.
 * @param src Pointer to store source node address.
 * @param dstPort Pointer to store destination port.
 * @param srcPort Pointer to store source port.
 * @param flags Pointer to store packet flags.
 */
void danpUnpackHeader(
    uint32_t raw,
    uint16_t *dst,
    uint16_t *src,
    uint8_t *dstPort,
    uint8_t *srcPort,
    uint8_t *flags)
{
    *dst = (raw >> 22) & 0xFF;
    *src = (raw >> 14) & 0xFF;
    *dstPort = (raw >> 8) & 0x3F;
    *srcPort = (raw >> 2) & 0x3F;

    uint8_t f = (raw) & 0x03;

    if (raw & (1U << 31))
    {
        f |= DANP_FLAG_RST;
    }

    *flags = f;
}

/**
 * @brief Initialize the DANP library.
 * @param config Pointer to the configuration structure.
 */
void danpInit(const danpConfig_t *config)
{
    for (;;)
    {
        memcpy(&DanpConfig, config, sizeof(danpConfig_t));
        danpSocketInit();
        danpBufferInit();

        break;
    }

}

/**
 * @brief Register a network interface.
 * @param iface Pointer to the interface to register.
 */
void danpRegisterInterface(void *iface)
{
    danpInterface_t *ifaceCommon = iface;
    if (!ifaceCommon)
    {
        danpLogMessage(DANP_LOG_ERROR, "Cannot register NULL interface");
        return;
    }
    if (!ifaceCommon->txFunc)
    {
        danpLogMessage(DANP_LOG_ERROR, "Interface txFunc is NULL, cannot register");
        return;
    }
    if (!ifaceCommon->name)
    {
        danpLogMessage(DANP_LOG_ERROR, "Interface name is NULL, cannot register");
        return;
    }
    if (ifaceCommon->mtu == 0)
    {
        danpLogMessage(DANP_LOG_ERROR, "Interface MTU is zero, cannot register");
        return;
    }
    if (!IfaceList)
    {
        danpLogMessage(DANP_LOG_INFO, "Registering first network interface: %s", ifaceCommon->name);
    }
    else
    {
        danpLogMessage(DANP_LOG_INFO, "Registering network interface: %s", ifaceCommon->name);
    }
    ifaceCommon->next = IfaceList;
    IfaceList = iface;
    danpLogMessage(DANP_LOG_VERBOSE, "Registered network interface");
}

/**
 * @brief Lookup the route for a destination node.
 * @param destNodeId Destination node ID.
 * @return Pointer to the interface to use, or NULL if no route found.
 */
static danpInterface_t *danpRouteLookup(uint16_t destNodeId)
{
    (void)destNodeId; // Mark parameter as unused
    return IfaceList;
}

/**
 * @brief Route a packet for transmission.
 * @param pkt Pointer to the packet to route.
 * @return 0 on success, negative on error.
 */
int32_t danpRouteTx(danpPacket_t *pkt)
{
    uint16_t dst, src;
    uint8_t dstPort, srcPort, flags;
    danpUnpackHeader(pkt->headerRaw, &dst, &src, &dstPort, &srcPort, &flags);

    danpInterface_t *out = danpRouteLookup(dst);
    if (!out)
    {
        danpLogMessage(DANP_LOG_ERROR, "No route to destination");
        return -1;
    }
    danpLogMessage(
        DANP_LOG_DEBUG,
        "TX [dst]=%u, [src]=%u, [dPort]=%u, [sPort]=%u, [flags]=0x%02X, [len]=%u, [iface]=%s",
        dst,
        src,
        dstPort,
        srcPort,
        flags,
        pkt->length,
        out->name);
    return out->txFunc(out, pkt);
}

/**
 * @brief Process incoming data from an interface.
 * @param iface Pointer to the interface receiving data.
 * @param rawData Pointer to the received data.
 * @param len Length of the received data.
 */
void danpInput(danpInterface_t *iface, uint8_t *rawData, uint16_t len)
{
    if (len < DANP_HEADER_SIZE)
    {
        danpLogMessage(DANP_LOG_WARN, "Received packet too short, dropping");
        return;
    }
    danpPacket_t *pkt = danpBufferAllocate();
    if (!pkt)
    {
        danpLogMessage(DANP_LOG_ERROR, "No memory for incoming packet, dropping");
        return;
    }
    memcpy(&pkt->headerRaw, rawData, 4);
    pkt->length = len - 4;
    if (pkt->length > 0)
    {
        memcpy(pkt->payload, rawData + 4, pkt->length);
    }
    pkt->rxInterface = iface;

    uint16_t dst, src;
    uint8_t dstPort, srcPort, flags;
    danpUnpackHeader(pkt->headerRaw, &dst, &src, &dstPort, &srcPort, &flags);

    danpLogMessage(
        DANP_LOG_DEBUG,
        "RX [dst]=%u [src]=%u [dPort]=%u [sPort]=%u [flags]=0x%02X [len]=%u, [iface]=%s",
        dst,
        src,
        dstPort,
        srcPort,
        flags,
        pkt->length,
        iface->name
    );

    if (dst == iface->address)
    {
        danpLogMessage(DANP_LOG_VERBOSE, "Packet received for local node");
        danpSocketInputHandler(pkt);
    }
    else
    {
        danpLogMessage(DANP_LOG_INFO, "Packet not for local node, dropping");
        danpBufferFree(pkt);
    }
}

/**
 * @brief Log a message using the registered callback.
 * @param level Log level.
 * @param funcName Name of the function.
 * @param message Message format string.
 * @param ... Variable arguments.
 */
void danpLogMessageHandler(danpLogLevel_t level, const char *funcName, const char *message, ...)
{
    if (DanpConfig.logFunction)
    {
        va_list args;
        va_start(args, message);
        DanpConfig.logFunction(level, funcName, message, args);
        va_end(args);
    }
}
