/* danp_uart.c - DANP UART interface driver */

/* All Rights Reserved */

/* Includes */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include "danp/danp.h"
#include "../danp_debug.h"
#include "danp/drivers/danp_z_uart.h"

/* Definitions */

#define DANP_DRIVER_UART_STACK_SIZE         (1024 * 2)
#define DANP_DRIVER_UART_RX_RING_SIZE       (512)
#define DANP_DRIVER_UART_TX_TIMEOUT_MS      (1000)
#define DANP_DRIVER_UART_RX_TIMEOUT_MS      (100)

/* Frame format:
 * [SYNC_BYTE][LENGTH_HIGH][LENGTH_LOW][PAYLOAD...][CRC_HIGH][CRC_LOW]
 * SYNC_BYTE = 0x7E
 * LENGTH = payload length (header_raw + data), big-endian
 * CRC = CRC-16 over LENGTH + PAYLOAD, big-endian
 */
#define DANP_UART_SYNC_BYTE                 (0x7E)

/* Types */

typedef enum danp_z_uart_rx_state_e
{
    DANP_UART_RX_STATE_SYNC,
    DANP_UART_RX_STATE_LEN_HIGH,
    DANP_UART_RX_STATE_LEN_LOW,
    DANP_UART_RX_STATE_PAYLOAD,
    DANP_UART_RX_STATE_CRC_HIGH,
    DANP_UART_RX_STATE_CRC_LOW,
} danp_z_uart_rx_state_t;

typedef struct danp_z_uart_context_s
{
    const struct device *uart_dev;
    struct k_sem rx_sem;
    uint8_t rx_ring_buf[DANP_DRIVER_UART_RX_RING_SIZE];
    struct ring_buf rx_ringbuf;
    volatile bool running;
} danp_z_uart_context_t;

/* Forward Declarations */

static void danp_z_uart_isr_callback(const struct device *dev, void *user_data);
static uint16_t danp_z_uart_crc16(const uint8_t *data, size_t len);

/* Functions */

/**
 * @brief CRC-16-CCITT calculation
 */
static uint16_t danp_z_uart_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief UART ISR callback for interrupt-driven reception
 */
static void danp_z_uart_isr_callback(const struct device *dev, void *user_data)
{
    danp_uart_context_t *ctx = (danp_uart_context_t *)user_data;
    uint8_t byte;

    if (!uart_irq_update(dev))
    {
        return;
    }

    while (uart_irq_rx_ready(dev))
    {
        if (uart_fifo_read(dev, &byte, 1) == 1)
        {
            ring_buf_put(&ctx->rx_ringbuf, &byte, 1);
            k_sem_give(&ctx->rx_sem);
        }
    }
}

/**
 * @brief Transmit a DANP packet over UART
 */
static int32_t danp_uart_tx(void *iface_common, danp_packet_t *packet)
{
    danp_z_uart_interface_t *uart_iface = (danp_z_uart_interface_t *)iface_common;
    danp_z_uart_context_t *uart_ctx = (danp_z_uart_context_t *)uart_iface->context;
    int32_t ret = 0;
    uint8_t frame_header[3];
    uint8_t frame_crc[2];
    uint16_t payload_len;
    uint16_t crc;

    for (;;)
    {
        payload_len = packet->length + sizeof(packet->header_raw);

        DANP_LOG_VER(
            "UART TX: dst=%u port=%u flags=0x%02X len=%u",
            (packet->header_raw >> 22) & 0xFF,
            (packet->header_raw >> 8) & 0x3F,
            packet->header_raw & 0x03,
            packet->length);

        /* Build frame header */
        frame_header[0] = DANP_UART_SYNC_BYTE;
        frame_header[1] = (uint8_t)(payload_len >> 8);
        frame_header[2] = (uint8_t)(payload_len & 0xFF);

        /* Calculate CRC over length + payload */
        uint8_t crc_buf[2 + DANP_MAX_PACKET_SIZE + sizeof(uint32_t)];
        crc_buf[0] = frame_header[1];
        crc_buf[1] = frame_header[2];
        memcpy(&crc_buf[2], packet, payload_len);
        crc = danp_uart_crc16(crc_buf, 2 + payload_len);

        frame_crc[0] = (uint8_t)(crc >> 8);
        frame_crc[1] = (uint8_t)(crc & 0xFF);

        /* Transmit frame: SYNC + LENGTH + PAYLOAD + CRC */
        for (size_t i = 0; i < sizeof(frame_header); i++)
        {
            uart_poll_out(uart_ctx->uart_dev, frame_header[i]);
        }

        const uint8_t *payload_bytes = (const uint8_t *)packet;
        for (size_t i = 0; i < payload_len; i++)
        {
            uart_poll_out(uart_ctx->uart_dev, payload_bytes[i]);
        }

        for (size_t i = 0; i < sizeof(frame_crc); i++)
        {
            uart_poll_out(uart_ctx->uart_dev, frame_crc[i]);
        }

        break;
    }

    return ret;
}

/**
 * @brief RX processing thread
 */
static void danp_uart_rx_routine(void *arg)
{
    danp_uart_interface_t *uart_iface = (danp_uart_interface_t *)arg;
    danp_uart_context_t *uart_ctx = (danp_uart_context_t *)uart_iface->context;

    danp_uart_rx_state_t state = DANP_UART_RX_STATE_SYNC;
    danp_packet_t pkt = {0};
    uint8_t *pkt_bytes = (uint8_t *)&pkt;
    uint16_t expected_len = 0;
    uint16_t rx_index = 0;
    uint16_t rx_crc = 0;
    uint8_t byte;

    while (uart_ctx->running)
    {
        /* Wait for data with timeout */
        if (k_sem_take(&uart_ctx->rx_sem, K_MSEC(DANP_DRIVER_UART_RX_TIMEOUT_MS)) != 0)
        {
            continue;
        }

        /* Process all available bytes */
        while (ring_buf_get(&uart_ctx->rx_ringbuf, &byte, 1) == 1)
        {
            switch (state)
            {
                case DANP_UART_RX_STATE_SYNC:
                    if (byte == DANP_UART_SYNC_BYTE)
                    {
                        state = DANP_UART_RX_STATE_LEN_HIGH;
                        memset(&pkt, 0, sizeof(pkt));
                        rx_index = 0;
                    }
                    break;

                case DANP_UART_RX_STATE_LEN_HIGH:
                    expected_len = (uint16_t)byte << 8;
                    state = DANP_UART_RX_STATE_LEN_LOW;
                    break;

                case DANP_UART_RX_STATE_LEN_LOW:
                    expected_len |= byte;
                    if (expected_len > sizeof(danp_packet_t) || expected_len < sizeof(uint32_t))
                    {
                        DANP_LOG_WRN(
                            "UART RX: Invalid length %u, resync",
                            expected_len);
                        state = DANP_UART_RX_STATE_SYNC;
                    }
                    else
                    {
                        state = DANP_UART_RX_STATE_PAYLOAD;
                        rx_index = 0;
                    }
                    break;

                case DANP_UART_RX_STATE_PAYLOAD:
                    pkt_bytes[rx_index++] = byte;
                    if (rx_index >= expected_len)
                    {
                        state = DANP_UART_RX_STATE_CRC_HIGH;
                    }
                    break;

                case DANP_UART_RX_STATE_CRC_HIGH:
                    rx_crc = (uint16_t)byte << 8;
                    state = DANP_UART_RX_STATE_CRC_LOW;
                    break;

                case DANP_UART_RX_STATE_CRC_LOW:
                {
                    rx_crc |= byte;

                    /* Verify CRC */
                    uint8_t crc_buf[2 + DANP_MAX_PACKET_SIZE + sizeof(uint32_t)];
                    crc_buf[0] = (uint8_t)(expected_len >> 8);
                    crc_buf[1] = (uint8_t)(expected_len & 0xFF);
                    memcpy(&crc_buf[2], &pkt, expected_len);
                    uint16_t calc_crc = danp_uart_crc16(crc_buf, 2 + expected_len);

                    if (rx_crc == calc_crc)
                    {
                        DANP_LOG_VER(
                            "UART RX: len=%u",
                            expected_len);

                        danp_input(&uart_iface->common, (uint8_t *)&pkt, expected_len);
                    }
                    else
                    {
                        DANP_LOG_WRN(
                            "UART RX: CRC mismatch (rx=0x%04X, calc=0x%04X)",
                            rx_crc, calc_crc);
                    }

                    state = DANP_UART_RX_STATE_SYNC;
                    break;
                }

                default:
                    state = DANP_UART_RX_STATE_SYNC;
                    break;
            }
        }
    }
}

int32_t danp_z_uart_init(
    danp_z_uart_interface_t *iface,
    const char *name,
    const struct device *uart_dev,
    int16_t address)
{
    int32_t ret = 0;
    osalThreadHandle_t thread_handle = NULL;
    osalThreadAttr_t thread_attr =
    {
        .name = "danpUartRx",
        .stackSize = DANP_DRIVER_UART_STACK_SIZE,
        .stackMem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cbMem = NULL,
        .cbSize = 0,
    };
    danp_uart_context_t *uart_ctx = NULL;

    for (;;)
    {
        if ((NULL == iface) || (NULL == uart_dev))
        {
            ret = -EINVAL;
            DANP_LOG_ERR(
                "Invalid parameters to danp_uart_init");
            break;
        }

        if (!device_is_ready(uart_dev))
        {
            ret = -ENODEV;
            DANP_LOG_ERR(
                "UART device not ready");
            break;
        }

        memset(iface, 0, sizeof(danp_uart_interface_t));

        uart_ctx = (danp_uart_context_t *)malloc(sizeof(danp_uart_context_t));
        if (NULL == uart_ctx)
        {
            ret = -ENOMEM;
            DANP_LOG_ERR(
                "Failed to allocate UART context");
            break;
        }

        memset(uart_ctx, 0, sizeof(danp_uart_context_t));
        uart_ctx->uart_dev = uart_dev;
        uart_ctx->running = true;

        /* Initialize ring buffer */
        ring_buf_init(&uart_ctx->rx_ringbuf, sizeof(uart_ctx->rx_ring_buf), uart_ctx->rx_ring_buf);

        /* Initialize semaphore */
        ret = k_sem_init(&uart_ctx->rx_sem, 0, K_SEM_MAX_LIMIT);
        if (ret != 0)
        {
            DANP_LOG_ERR(
                "Failed to initialize RX semaphore: %d",
                ret);
            free(uart_ctx);
            break;
        }

        /* Setup interface */
        iface->common.address = address;
        iface->common.tx_func = danp_uart_tx;
        iface->common.name = name;
        iface->common.mtu = DANP_MAX_PACKET_SIZE;
        iface->context = uart_ctx;

        /* Setup UART interrupt callback */
        uart_irq_callback_user_data_set(uart_dev, danp_uart_isr_callback, uart_ctx);
        uart_irq_rx_enable(uart_dev);

        /* Create RX thread */
        thread_handle = osalThreadCreate(danp_uart_rx_routine, iface, &thread_attr);
        if (NULL == thread_handle)
        {
            ret = -EAGAIN;
            DANP_LOG_ERR(
                "Failed to create UART RX thread");
            uart_irq_rx_disable(uart_dev);
            free(uart_ctx);
            break;
        }

        DANP_LOG_INF(
            "DANP UART interface initialized: %s",
            name);

        break;
    }

    return ret;
}

int32_t danp_z_uart_deinit(danp_z_uart_interface_t *iface)
{
    int32_t ret = 0;

    if ((NULL == iface) || (NULL == iface->context))
    {
        return -EINVAL;
    }

    danp_uart_context_t *uart_ctx = (danp_uart_context_t *)iface->context;

    /* Signal thread to stop */
    uart_ctx->running = false;

    /* Disable UART interrupts */
    uart_irq_rx_disable(uart_ctx->uart_dev);

    /* Give thread time to exit */
    k_sleep(K_MSEC(DANP_DRIVER_UART_RX_TIMEOUT_MS * 2));

    /* Free context */
    free(uart_ctx);
    iface->context = NULL;

    return ret;
}
