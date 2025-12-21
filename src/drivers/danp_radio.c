/* danp_radio.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <stdlib.h>
#include "osal/osal.h"
#include "danp/danp.h"
#include "../danp_debug.h"
#include "danp/drivers/danp_radio.h"

/* Imports */


/* Definitions */

#define DANP_DRIVER_RADIO_STACK_SIZE               (1024 * 4)
#define DANP_DRIVER_RADIO_TIMEOUT_MS               (5000)

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
        danp_log_message(
            DANP_LOG_VERBOSE,
            "Radio TX: dst=%u port=%u flags=0x%02X len=%u",
            (packet->header_raw >> 22) & 0xFF,
            (packet->header_raw >> 8) & 0x3F,
            packet->header_raw & 0x03,
            packet->length);

        ret = radio_ctrl_transmit(radio_ctx->radio_dev, (const uint8_t *)packet, packet->length + sizeof(packet->header_raw));
        if (ret < 0)
        {
            danp_log_message(
                DANP_LOG_ERROR,
                "Radio TX failed: %d",
                ret);
            break;
        }

        ret = radio_ctrl_listen(radio_ctx->radio_dev, RADIO_CTRL_RX_TIMEOUT_MAX_MS);
        if (ret < 0)
        {
            danp_log_message(
                DANP_LOG_ERROR,
                "Radio listen failed: %d",
                ret);
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

    for (;;)
    {
        danp_packet_t pkt = {0};
        ret = radio_ctrl_receive(radio_ctx->radio_dev, (uint8_t *)&pkt, sizeof(pkt), NULL, DANP_DRIVER_RADIO_TIMEOUT_MS);
        if (ret > 0)
        {
            danp_log_message(
                DANP_LOG_VERBOSE,
                "Radio RX: len=%u",
                ret);

            danp_input(&radio_iface->common, (uint8_t *)&pkt, (uint16_t)ret);
        }
    }
}

int32_t danp_radio_init (
    danp_radio_interface_t *iface,
    const char *name,
    const struct device *radio_dev,
    const ralf_params_lora_t *rx_params,
    const ralf_params_lora_t *tx_params,
    const ralf_params_lora_cad_t *cad_params,
    int16_t address)
{
    int32_t ret = 0;
    osalThreadHandle_t thread_handle = NULL;
    osalThreadAttr_t thread_attr =
    {
        .name = "danpRadioCtx",
        .stackSize = DANP_DRIVER_RADIO_STACK_SIZE,
        .stackMem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cbMem = NULL,
        .cbSize = 0,
    };
    danp_radio_context_t *radio_ctx = NULL;

    for (;;)
    {
        if ((NULL == iface) || (NULL == radio_dev))
        {
            ret = -1;
            danp_log_message(
                DANP_LOG_ERROR,
                "Invalid parameters to danp_radio_init");
            break;
        }

        memset(iface, 0, sizeof(danp_radio_interface_t));

        radio_ctx = (danp_radio_context_t *)malloc(sizeof(danp_radio_context_t));
        if (NULL == radio_ctx)
        {
            ret = -1;
            danp_log_message(
                DANP_LOG_ERROR,
                "Failed to allocate radio context");
            break;
        }

        radio_ctx->radio_dev = radio_dev;

        iface->common.address = address;
        iface->common.tx_func = danp_radio_tx;
        iface->common.name = name;
        iface->common.mtu = DANP_MAX_PACKET_SIZE;
        iface->context = radio_ctx;

        ret = radio_ctrl_set_config_lora(radio_ctx->radio_dev, rx_params, tx_params, cad_params);
        if (ret < 0)
        {
            danp_log_message(
                DANP_LOG_ERROR,
                "Failed to set LoRa config: %d",
                ret);
            break;
        }

        ret = radio_ctrl_listen(radio_ctx->radio_dev, RADIO_CTRL_RX_TIMEOUT_MAX_MS);
        if (ret < 0)
        {
            danp_log_message(
                DANP_LOG_ERROR,
                "Radio listen failed: %d",
                ret);
            break;
        }

        thread_handle = osalThreadCreate(danp_radio_rx_routine, iface, &thread_attr);
        if (NULL == thread_handle)
        {
            ret = -1;
            danp_log_message(
                DANP_LOG_ERROR,
                "Failed to create radio RX thread");
            break;
        }

        break;
    }

    return ret;
}
