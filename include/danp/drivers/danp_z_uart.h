/* danp_uart.h - DANP UART interface driver */

/* All Rights Reserved */

#ifndef DANP_UART_H
#define DANP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */

#include <zephyr/device.h>
#include "danp/danp.h"

/* Definitions */

/* Types */

typedef struct danp_z_uart_interface_s
{
    danp_interface_common_t common;
    void *context;
} danp_z_uart_interface_t;

/* Function Prototypes */

/**
 * @brief Initialize DANP UART interface
 *
 * @param iface         Pointer to UART interface structure to initialize
 * @param name          Name of the interface
 * @param uart_dev      Pointer to Zephyr UART device
 * @param address       DANP address for this interface
 *
 * @return 0 on success, negative error code on failure
 */
int32_t danp_z_uart_init(
    danp_z_uart_interface_t *iface,
    const char *name,
    const struct device *uart_dev,
    int16_t address);

/**
 * @brief Deinitialize DANP UART interface
 *
 * @param iface         Pointer to UART interface structure
 *
 * @return 0 on success, negative error code on failure
 */
int32_t danp_z_uart_deinit(danp_z_uart_interface_t *iface);

#ifdef __cplusplus
}
#endif

#endif /* DANP_UART_H */
