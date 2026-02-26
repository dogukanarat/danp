/* danp_radio.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "danp/drivers/danp_z_radio.h"
#include "../danp_debug.h"
#include "danp/danp.h"
#include "danp/danp_buffer.h"
#include "osal/osal_thread.h"
#include "osal/osal_time.h"
#include <stdlib.h>

/* Imports */


/* Definitions */

#define DANP_DRIVER_RADIO_STACK_SIZE (1024 * 4)
#define DANP_DRIVER_RADIO_TIMEOUT_MS (5000)

/* Types */

typedef struct danp_radio_context_s
{
    const struct device *radio_dev;
} danp_radio_context_t;

/* Forward Declarations */


/* Variables */


/* Functions */

static int32_t danp_radio_tx(void *iface_common, danp_packet_t *packet)
{
    danp_radio_interface_t *radio_iface = (danp_radio_interface_t *)iface_common;
    danp_radio_context_t *radio_ctx = (danp_radio_context_t *)radio_iface->context;
    int32_t ret = 0;

    for (;;)
    {
        DANP_LOG_IO_VER(
            "Radio TX: Outgoing packet: [dst]=%u [port]=%u [flags]=0x%02X [len]=%u",
            (danp_packet_get_header(packet) >> 22) & 0xFF,
            (danp_packet_get_header(packet) >> 8) & 0x3F,
            danp_packet_get_header(packet) & 0x03,
            danp_packet_get_length(packet));

        ret = radio_ctrl_transmit(
            radio_ctx->radio_dev,
            (const uint8_t *)packet,
            danp_packet_get_length(packet) + sizeof(uint32_t));
        if (ret < 0)
        {
            DANP_LOG_IO_ERR("Radio TX: Failed: %08X", ret);
            break;
        }

        ret = radio_ctrl_listen(radio_ctx->radio_dev, RADIO_CTRL_RX_TIMEOUT_MAX_MS);
        if (ret < 0)
        {
            DANP_LOG_IO_ERR("Radio Listen: Failed: %08X", ret);
            break;
        }

        break;
    }

    return ret;
}

static void danp_radio_rx_routine(void *arg)
{
    danp_radio_interface_t *radio_iface = (danp_radio_interface_t *)arg;
    danp_radio_context_t *radio_ctx = (danp_radio_context_t *)radio_iface->context;
    int32_t ret = 0;
    danp_packet_t *pkt = NULL;

    for (;;)
    {
        if (radio_iface->common.data.is_exiting)
        {
            DANP_LOG_IO_INF("Radio RX: Exiting RX thread");
            break;
        }

        if (false == radio_iface->common.data.is_running)
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

        ret = radio_ctrl_receive(
            radio_ctx->radio_dev,
            (uint8_t *)pkt,
            DANP_MAX_PACKET_SIZE + sizeof(uint32_t),
            NULL,
            DANP_DRIVER_RADIO_TIMEOUT_MS);
        if (ret > 0)
        {
            danp_packet_set_length(pkt, ret - sizeof(uint32_t));

            if (ret < (int32_t)sizeof(uint32_t))
            {
                DANP_LOG_IO_WRN("Radio RX: received packet too short");
                continue;
            }

            DANP_LOG_IO_VER("Radio RX: Incoming packet: [len]=%u", danp_packet_get_length(pkt));

            danp_input(&radio_iface->common, pkt);
            pkt = NULL; // Ownership transferred to danp_input
        }
    }
}

int32_t danp_radio_init(
    danp_radio_interface_t *iface,
    const char *name,
    const struct device *radio_dev,
    const ralf_params_lora_t *rx_params,
    const ralf_params_lora_t *tx_params,
    const ralf_params_lora_cad_t *cad_params,
    int16_t address)
{
    int32_t ret = 0;
    osal_thread_handle_t thread_handle = NULL;
    osal_thread_attr_t thread_attr = {
        .name = "danpRadioCtx",
        .stack_size = DANP_DRIVER_RADIO_STACK_SIZE,
        .stack_mem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cb_mem = NULL,
        .cb_size = 0,
    };
    static danp_radio_context_t static_radio_ctx;
    danp_radio_context_t *radio_ctx = &static_radio_ctx;

    for (;;)
    {
        if ((NULL == iface) || (NULL == radio_dev))
        {
            ret = -1;
            DANP_LOG_IO_ERR("Invalid parameters to danp_radio_init");
            break;
        }

        memset(iface, 0, sizeof(danp_radio_interface_t));

        // Use static context instead of malloc
        radio_ctx->radio_dev = radio_dev;

        iface->common.address = address;
        iface->common.ops.tx = danp_radio_tx;
        iface->common.data.is_exiting = false;
        iface->common.data.is_running = false;
        iface->common.name = name;
        iface->common.mtu = DANP_MAX_PACKET_SIZE;
        iface->context = radio_ctx;

        ret = radio_ctrl_set_config_lora(radio_ctx->radio_dev, rx_params, tx_params, cad_params);
        if (ret < 0)
        {
            DANP_LOG_IO_ERR("Failed to set LoRa config: %d", ret);
            break;
        }

        ret = radio_ctrl_listen(radio_ctx->radio_dev, RADIO_CTRL_RX_TIMEOUT_MAX_MS);
        if (ret < 0)
        {
            DANP_LOG_IO_ERR("Radio Listen: Failed: %08X", ret);
            break;
        }

        thread_handle = osal_thread_create(danp_radio_rx_routine, iface, &thread_attr);
        if (NULL == thread_handle)
        {
            ret = -1;
            DANP_LOG_IO_ERR("Failed to create radio RX thread");
            break;
        }

        DANP_LOG_IO_INF("Radio interface '%s' initialized at address %d", name, address);

        break;
    }

    return ret;
}
