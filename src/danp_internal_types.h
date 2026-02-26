/* danp_internal_types.h - Internal structure definitions for DANP */

#ifndef DANP_INTERNAL_TYPES_H
#define DANP_INTERNAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "danp/danp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure representing a DANP socket.
 */
struct danp_socket_s
{
    danp_socket_state_t state; ///< Current state of the socket.
    danp_socket_type_t type;   ///< Type of the socket.
 
    // Addressing
    uint16_t local_port;  ///< Local port number.
    uint16_t local_node;  ///< Local node address.
    uint16_t remote_node; ///< Remote node address.
    uint16_t remote_port; ///< Remote port number.
 
    // Reliability State (Stop-and-Wait)
    uint8_t tx_seq;         ///< Transmit sequence number.
    uint8_t rx_expected_seq; ///< Expected receive sequence number.
 
    // RTOS Handles
    danp_os_queue_handle_t rx_queue;     ///< Queue for received packets.
    danp_os_queue_handle_t accept_queue; ///< Queue for accepted connections.
    danp_os_semaphore_handle_t signal;  ///< Semaphore for signaling.
 
    struct danp_socket_s *next; ///< Pointer to the next socket in the list.
};

/**
 * @brief Structure representing a DANP packet.
 */
struct danp_packet_s
{
    uint32_t header_raw;                    ///< Raw header data.
    uint8_t payload[DANP_MAX_PACKET_SIZE]; ///< Payload data.
 
    uint16_t length;                       ///< Length of the payload.
    struct danp_interface_s *rx_interface;   ///< Interface where the packet was received.
    struct danp_packet_s *next;            ///< Pointer to next packet (for chaining/fragmentation).
};


#ifdef __cplusplus
}
#endif

#endif /* DANP_INTERNAL_TYPES_H */
