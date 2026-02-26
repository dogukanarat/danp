/* danp.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <errno.h>
#include <stdio.h>

#include "danp/danp.h"
#include "danp/danp_buffer.h"
#include "danp_debug.h"
#include "danp_internal_types.h"

/* Imports */

extern void danp_socket_input_handler(danp_packet_t *pkt);
extern void danp_start_interfaces(void);

/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief Global configuration structure. */
danp_config_t danp_config;

/* Functions */

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

int32_t danp_init(const danp_config_t *config)
{
    int32_t status = 0;

    for (;;)
    {
        if (!config)
        {
            status = -EINVAL;
            DANP_LOG_ERR("DANP init failed: NULL config");
            break;
        }
        memcpy(&danp_config, config, sizeof(danp_config_t));
        danp_socket_init();
        danp_route_init();
        danp_buffer_init();

        danp_start_interfaces();

        DANP_LOG_INF("DANP initialized with local node: %u", danp_config.local_node);

        break;
    }

    return status;
}

void danp_deinit(void)
{

}

void danp_input(danp_interface_t *iface, danp_packet_t *incoming_pkt)
{
    uint16_t dst;
    uint16_t src;
    uint8_t dst_port;
    uint8_t src_port;
    uint8_t flags;

    for (;;)
    {
        incoming_pkt->rx_interface = iface;
    
        danp_unpack_header(incoming_pkt->header_raw, &dst, &src, &dst_port, &src_port, &flags);
    
        DANP_LOG_IO_DBG(
            "Input: [dst]=%u [src]=%u [dPort]=%u [sPort]=%u [flags]=0x%02X [len]=%u [iface]=%s",
            dst,
            src,
            dst_port,
            src_port,
            flags,
            incoming_pkt->length,
            iface->name
        );
    
        if (dst != iface->address)
        {
            DANP_LOG_IO_INF("Input: Packet not for local node, dropping");
            danp_buffer_free(incoming_pkt);
            break;
        }
        
        DANP_LOG_IO_VER("Input: Packet received for local node");
        danp_socket_input_handler(incoming_pkt);

        break;
    }

}

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

void danp_log_message_handler_io(danp_log_level_t level, const char *func_name, const char *message, ...)
{
    if (danp_config.log_function_io)
    {
        va_list args;
        va_start(args, message);
        danp_config.log_function_io(level, func_name, message, args);
        va_end(args);
    }
}