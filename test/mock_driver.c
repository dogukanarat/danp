/**
 * @file mockDriver.c
 * @brief Mock network driver for DANP testing
 *
 * This file implements a simple loopback network driver that simulates
 * network communication by immediately feeding transmitted packets back
 * into the receive path. This enables unit testing without real hardware.
 */

#include "danp/danp.h"
#include "osal/osal.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Module Variables
 * ============================================================================
 */

/**
 * @brief Mock interface structure for loopback testing
 */
static danp_interface_t mock_interface;

/**
 * @brief Local node ID for the mock driver
 */
static uint16_t my_node_id = 1;

/* ============================================================================
 * Internal Functions
 * ============================================================================
 */

/**
 * @brief Loopback transmit function
 *
 * This function simulates packet transmission by immediately feeding the
 * packet back into the DANP input handler, creating a perfect loopback.
 * This allows testing of protocol logic without real network hardware.
 *
 * @param iface Pointer to the network interface (unused in loopback)
 * @param packet Pointer to the packet to transmit
 * @return 0 on success, negative on error
 */
int32_t mock_loopback_tx(danp_interface_t *iface, danp_packet_t *packet)
{
    uint8_t buffer[DANP_MAX_PACKET_SIZE + 4];

    /* Serialize packet into buffer (header + payload) */
    /* Copy 4-byte header */
    memcpy(buffer, &packet->header_raw, 4);

    /* Copy payload if present */
    if (packet->length > 0)
    {
        memcpy(buffer + 4, packet->payload, packet->length);
    }

    /* Simulate network delay (optional, usually 0 for unit tests) */
    /* In real drivers, this would transmit over the physical medium */

    /* Feed packet back into the DANP core as received data */
    /* This creates a perfect loopback: if we send it, we immediately receive it */
    /* The DANP core will filter based on destination address (dst == my_node_id) */
    danp_input(iface, buffer, 4 + packet->length);

    return 0;
}

/* ============================================================================
 * Public Functions
 * ============================================================================
 */

/**
 * @brief Initialize the mock driver with loopback functionality
 *
 * This function sets up a fake network interface that loops back all
 * transmitted packets. This is essential for unit testing socket operations
 * without requiring actual network hardware or peer nodes.
 *
 * @param node_id The local node ID for this interface
 */
void mock_driver_init(uint16_t node_id)
{
    /* Store the node ID for reference */
    my_node_id = node_id;

    /* Configure the mock interface */
    mock_interface.name = "LOOPBACK";
    mock_interface.address = node_id;
    mock_interface.mtu = 128;
    mock_interface.tx_func = (int32_t (*)(void *iface_common, danp_packet_t *packet))mock_loopback_tx;

    /* Register the interface with the DANP core */
    danp_register_interface(&mock_interface);
}
