/* danp.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "danp/danp_buffer.h"
#include "danp_debug.h"
#include <stdarg.h>
#include <stdio.h>

/* Imports */

extern void danp_socket_input_handler(danp_packet_t *pkt);

/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief Global configuration structure. */
danp_config_t danp_config;

/* Functions */

/**
 * @brief Pack a DANP header.
 * @param prio Packet priority.
 * @param dst Destination node address.
 * @param src Source node address.
 * @param dst_port Destination port.
 * @param src_port Source port.
 * @param flags Packet flags.
 * @return Packed header as a 32-bit integer.
 */
uint32_t danp_pack_header(
    uint8_t prio,
    uint16_t dst,
    uint16_t src,
    uint8_t dst_port,
    uint8_t src_port,
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
    h |= (uint32_t)(dst_port & 0x3F) << 8;
    h |= (uint32_t)(src_port & 0x3F) << 2;

    h |= (uint32_t)(flags & 0x03);

    return h;
}

/**
 * @brief Unpack a DANP header.
 * @param raw Raw 32-bit header.
 * @param dst Pointer to store destination node address.
 * @param src Pointer to store source node address.
 * @param dst_port Pointer to store destination port.
 * @param src_port Pointer to store source port.
 * @param flags Pointer to store packet flags.
 */
void danp_unpack_header(
    uint32_t raw,
    uint16_t *dst,
    uint16_t *src,
    uint8_t *dst_port,
    uint8_t *src_port,
    uint8_t *flags)
{
    *dst = (raw >> 22) & 0xFF;
    *src = (raw >> 14) & 0xFF;
    *dst_port = (raw >> 8) & 0x3F;
    *src_port = (raw >> 2) & 0x3F;

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
void danp_init(const danp_config_t *config)
{
    for (;;)
    {
        memcpy(&danp_config, config, sizeof(danp_config_t));
        danp_socket_init();
        danp_buffer_init();

        break;
    }

}

void danp_input(danp_interface_t *iface, uint8_t *raw_data, uint16_t len)
{
    if (len < DANP_HEADER_SIZE)
    {
        danp_log_message(DANP_LOG_WARN, "Received packet too short, dropping");
        return;
    }
    danp_packet_t *pkt = danp_buffer_allocate();
    if (!pkt)
    {
        danp_log_message(DANP_LOG_ERROR, "No memory for incoming packet, dropping");
        return;
    }
    memcpy(&pkt->header_raw, raw_data, 4);
    pkt->length = len - 4;
    if (pkt->length > 0)
    {
        memcpy(pkt->payload, raw_data + 4, pkt->length);
    }
    pkt->rx_interface = iface;

    uint16_t dst, src;
    uint8_t dst_port, src_port, flags;
    danp_unpack_header(pkt->header_raw, &dst, &src, &dst_port, &src_port, &flags);

    danp_log_message(
        DANP_LOG_DEBUG,
        "RX [dst]=%u [src]=%u [dPort]=%u [sPort]=%u [flags]=0x%02X [len]=%u, [iface]=%s",
        dst,
        src,
        dst_port,
        src_port,
        flags,
        pkt->length,
        iface->name
    );

    if (dst == iface->address)
    {
        danp_log_message(DANP_LOG_VERBOSE, "Packet received for local node");
        danp_socket_input_handler(pkt);
    }
    else
    {
        danp_log_message(DANP_LOG_INFO, "Packet not for local node, dropping");
        danp_buffer_free(pkt);
    }
}

/**
 * @brief Log a message using the registered callback.
 * @param level Log level.
 * @param func_name Name of the function.
 * @param message Message format string.
 * @param ... Variable arguments.
 */
void danp_log_message_handler(danp_log_level_t level, const char *func_name, const char *message, ...)
{
    if (danp_config.log_function)
    {
        va_list args;
        va_start(args, message);
        danp_config.log_function(level, func_name, message, args);
        va_end(args);
    }
}
