/* danpType.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_TYPES_H
#define INC_DANP_TYPES_H

/* Includes */

#include <stdint.h>

#include "osal/osal_message_queue.h"
#include "osal/osal_semaphore.h"

#include "danp/danp_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Configurations */


/* Definitions */

#ifndef WEAK
/** @brief Weak attribute for overridable functions. */
#define WEAK __attribute__((weak))
#endif

#ifndef PACKED
/** @brief Packed attribute for structures. */
#define PACKED __attribute__((packed))
#endif

#ifndef UNUSED
/** @brief Macro to mark unused variables and avoid compiler warnings. */
#define UNUSED(x) (void)(x)
#endif

/* Types */


/**
 * @brief Packet flags for control and state.
 */
 typedef enum danp_packet_flags_e
 {
     DANP_FLAG_NONE = 0x00, /**< No flags set. */
     DANP_FLAG_SYN = 0x01,  /**< Connection Request. */
     DANP_FLAG_ACK = 0x02,  /**< Acknowledge (used for both Connect and Data). */
     DANP_FLAG_RST = 0x04   /**< Reset Connection. */
 } danp_packet_flags_t;
 
 /**
  * @brief Log levels for the library.
  */
 typedef enum danp_log_level_e
 {
     DANP_LOG_VERBOSE = 0, /**< Verbose logging. */
     DANP_LOG_DEBUG,       /**< Debug logging. */
     DANP_LOG_INFO,        /**< Informational logging. */
     DANP_LOG_WARN,        /**< Warning logging. */
     DANP_LOG_ERROR        /**< Error logging. */
 } danp_log_level_t;
 
 /**
  * @brief Socket types.
  */
 typedef enum danp_socket_type_e
 {
     DANP_TYPE_DGRAM = 0, /**< Unreliable (UDP-like). */
     DANP_TYPE_STREAM = 1 /**< Reliable (RDP/TCP-like). */
 } danp_socket_type_t;
 
 /**
  * @brief Socket states.
  */
 typedef enum danp_socket_state_e
 {
     DANP_SOCK_CLOSED,    /**< Socket is unused or closed. */
     DANP_SOCK_OPEN,      /**< Socket is allocated and bound, but not connected (DGRAM default). */
     DANP_SOCK_LISTENING, /**< Socket is waiting for incoming connections (STREAM). */
     DANP_SOCK_SYN_SENT,  /**< Connection initiated, waiting for SYN-ACK (STREAM). */
     DANP_SOCK_SYN_RECEIVED, /**< SYN received, waiting for final ACK (STREAM). */
     DANP_SOCK_ESTABLISHED   /**< Connection established (STREAM) or Default Peer Set (DGRAM). */
 } danp_socket_state_t;
 
 /** @brief Handle for an OS queue. */
 typedef osal_message_queue_handle_t danp_os_queue_handle_t;
 
 /** @brief Handle for an OS semaphore. */
 typedef osal_semaphore_handle_t danp_os_semaphore_handle_t;
 
 /**
  * @brief Structure representing a DANP socket.
  */
 typedef struct danp_socket_s
 {
     danp_socket_state_t state; /**< Current state of the socket. */
     danp_socket_type_t type;   /**< Type of the socket. */
 
     // Addressing
     uint16_t local_port;  /**< Local port number. */
     uint16_t local_node;  /**< Local node address. */
     uint16_t remote_node; /**< Remote node address. */
     uint16_t remote_port; /**< Remote port number. */
 
     // Reliability State (Stop-and-Wait)
     uint8_t tx_seq;         /**< Transmit sequence number. */
     uint8_t rx_expected_seq; /**< Expected receive sequence number. */
 
     // RTOS Handles
     danp_os_queue_handle_t rx_queue;     /**< Queue for received packets. */
     danp_os_queue_handle_t accept_queue; /**< Queue for accepted connections. */
     danp_os_semaphore_handle_t signal;  /**< Semaphore for signaling. */
 
     struct danp_socket_s *next; /**< Pointer to the next socket in the list. */
 } danp_socket_t;
 
 /**
 * @brief Structure representing a DANP packet.
 */
 typedef struct danp_packet_s
 {
     uint32_t header_raw;                    /**< Raw header data. */
     uint8_t payload[DANP_MAX_PACKET_SIZE]; /**< Payload data. */
 
     uint16_t length;                       /**< Length of the payload. */
     struct danp_interface_s *rx_interface;   /**< Interface where the packet was received. */
     struct danp_packet_s *next;            /**< Pointer to next packet (for chaining/fragmentation). */
 } danp_packet_t;

 /**
  * @brief Structure representing a network interface.
  */
 typedef struct danp_interface_s
 {
     const char *name; /**< Name of the interface. */
     uint16_t address; /**< Address of the interface. */
     uint16_t mtu;     /**< Maximum Transmission Unit. */
 
     /**
      * @brief Function pointer to transmit a packet.
      * @param iface Pointer to the interface.
      * @param packet Pointer to the packet to transmit.
      * @return 0 on success, negative on error.
      */
     int32_t (*tx_func)(void *iface_common, danp_packet_t *packet);
 
     struct danp_interface_s *next; /**< Pointer to the next interface in the list. */
 } danp_interface_t;

/* External Declarations */


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_TYPES_H */
