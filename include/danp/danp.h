/* danp.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_H
#define INC_DANP_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */

#include "danp/danp_types.h"
#include "osal/osal.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @defgroup DANP_Config Configuration
 * @brief Configuration constants for the DANP library.
 * @{
 */

/* Configurations */

/* Definitions */

/** @brief Maximum size of a DANP packet payload in bytes. */
#define DANP_MAX_PACKET_SIZE 128

/** @brief Size of the packet pool. */
#define DANP_POOL_SIZE 20

/** @brief Size of the DANP header in bytes. */
#define DANP_HEADER_SIZE 4

/** @brief Maximum number of retries for reliable transmission. */
#define DANP_RETRY_LIMIT 3

/** @brief Acknowledgment timeout in milliseconds. */
#define DANP_ACK_TIMEOUT_MS 500

/** @brief Maximum number of supported ports. */
#define DANP_MAX_PORTS 64

/** @brief Maximum number of supported nodes. */
#define DANP_MAX_NODES 256

/** @brief Constant for infinite wait. */
#define DANP_WAIT_FOREVER 0xFFFFFFFFU

/** @brief High priority for packets. */
#define DANP_PRIORITY_HIGH 1

/** @brief Normal priority for packets. */
#define DANP_PRIORITY_NORMAL 0

/** @} */ // end of DANP_Config

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
typedef osalMessageQueueHandle_t danp_os_queue_handle_t;

/** @brief Handle for an OS semaphore. */
typedef osalSemaphoreHandle_t danp_os_semaphore_handle_t;

/* Header Packing Details (omitted for brevity) */

/**
 * @brief Structure representing a DANP packet.
 */
typedef struct danp_packet_s
{
    uint32_t header_raw;                    /**< Raw header data. */
    uint8_t payload[DANP_MAX_PACKET_SIZE]; /**< Payload data. */

    uint16_t length;                       /**< Length of the payload. */
    struct danp_interface_s *rx_interface;   /**< Interface where the packet was received. */
} danp_packet_t;

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

/**
 * @brief Callback function type for logging.
 * @param level Log level.
 * @param func_name Name of the function generating the log.
 * @param message Log message format string.
 * @param args Variable arguments list.
 */
typedef void (*danp_log_function_callback)(
    danp_log_level_t level,
    const char *func_name,
    const char *message,
    va_list args);

/**
 * @brief Configuration structure for DANP initialization.
 */
typedef struct danp_config_s
{
    uint16_t local_node;                  /**< Local node address. */
    danp_log_function_callback log_function; /**< Logging callback function. */
} danp_config_t;

/* External Declarations */

// Core functions

/**
 * @brief Initialize the DANP library.
 * @param config Pointer to the configuration structure.
 */
void danp_init(const danp_config_t *config);

/**
 * @brief Pack a DANP header.
 * @param priority Packet priority.
 * @param dst_node Destination node address.
 * @param src_node Source node address.
 * @param dst_port Destination port.
 * @param src_port Source port.
 * @param flags Packet flags.
 * @return Packed header as a 32-bit integer.
 */
uint32_t danp_pack_header(
    uint8_t priority,
    uint16_t dst_node,
    uint16_t src_node,
    uint8_t dst_port,
    uint8_t src_port,
    uint8_t flags);

/**
 * @brief Unpack a DANP header.
 * @param raw Raw 32-bit header.
 * @param dst_node Pointer to store destination node address.
 * @param src_node Pointer to store source node address.
 * @param dst_port Pointer to store destination port.
 * @param src_port Pointer to store source port.
 * @param flags Pointer to store packet flags.
 */
void danp_unpack_header(
    uint32_t raw,
    uint16_t *dst_node,
    uint16_t *src_node,
    uint8_t *dst_port,
    uint8_t *src_port,
    uint8_t *flags);

/**
 * @brief Log a message using the registered callback.
 * @param level Log level.
 * @param func_name Name of the function.
 * @param message Message format string.
 * @param ... Variable arguments.
 */
void danp_log_message_handler(danp_log_level_t level, const char *func_name, const char *message, ...);

/**
 * @brief Route a packet for transmission.
 * @param packet Pointer to the packet to route.
 * @return 0 on success, negative on error.
 */
int32_t danp_route_tx(danp_packet_t *packet);

/**
 * @brief Load a static routing table from a string.
 *
 * The table string uses comma or newline separated entries with the format
 * "<destination_node>:<interface_name>". Whitespace around tokens is ignored.
 *
 * @param table Routing table string (e.g., "1:if0, 42:backbone\n100:radio").
 * @return 0 on success, negative on error.
 */
int32_t danp_route_table_load(const char *table);

/**
 * @brief Process incoming data from an interface.
 * @param iface Pointer to the interface receiving data.
 * @param data Pointer to the received data.
 * @param length Length of the received data.
 */
void danp_input(danp_interface_t *iface, uint8_t *data, uint16_t length);

/**
 * @brief Allocate a packet from the pool.
 * @return Pointer to the allocated packet, or NULL if pool is empty.
 */
danp_packet_t *danp_buffer_allocate(void);

/**
 * @brief Free a packet back to the pool.
 * @param packet Pointer to the packet to free.
 */
void danp_buffer_free(danp_packet_t *packet);

/**
 * @brief Register a network interface.
 * @param iface Pointer to the interface to register.
 */
void danp_register_interface(void *iface);

// Socket API

/**
 * @brief Initialize the socket subsystem.
 * @return int32_t
 */
int32_t danp_socket_init(void);

/**
 * @brief Create a new DANP socket.
 * @param type Type of the socket (DGRAM or STREAM).
 * @return Pointer to the created socket, or NULL on failure.
 */
danp_socket_t *danp_socket(danp_socket_type_t type);

/**
 * @brief Bind a socket to a local port.
 * @param sock Pointer to the socket.
 * @param port Local port to bind to.
 * @return 0 on success, negative on error.
 */
int32_t danp_bind(danp_socket_t *sock, uint16_t port);

/**
 * @brief Listen for incoming connections on a socket.
 * @param sock Pointer to the socket.
 * @param backlog Maximum number of pending connections (currently unused).
 * @return 0 on success, negative on error.
 */
int32_t danp_listen(danp_socket_t *sock, int backlog);

/**
 * @brief Close a socket and release resources.
 * @param sock Pointer to the socket to close.
 * @return 0 on success, negative on error.
 */
int32_t danp_close(danp_socket_t *sock); // Added close function

// Functions updated with timeout_ms

/**
 * @brief Accept a new connection on a listening socket.
 * @param server_sock Pointer to the listening socket.
 * @param timeout_ms Timeout in milliseconds.
 * @return Pointer to the new connected socket, or NULL on timeout/error.
 */
danp_socket_t *danp_accept(danp_socket_t *server_sock, uint32_t timeout_ms);

/**
 * @brief Connect a socket to a remote node and port.
 * @param sock Pointer to the socket.
 * @param node Remote node address.
 * @param port Remote port number.
 * @return 0 on success, negative on error.
 */
int32_t danp_connect(danp_socket_t *sock, uint16_t node, uint16_t port);

/**
 * @brief Send data over a connected socket.
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @return Number of bytes sent, or negative on error.
 */
int32_t danp_send(danp_socket_t *sock, void *data, uint16_t len);

/**
 * @brief Receive data from a connected socket.
 * @param sock Pointer to the socket.
 * @param buffer Pointer to the buffer to store received data.
 * @param max_len Maximum length of the buffer.
 * @param timeout_ms Timeout in milliseconds.
 * @return Number of bytes received, or negative on error/timeout.
 */
int32_t danp_recv(danp_socket_t *sock, void *buffer, uint16_t max_len, uint32_t timeout_ms);

/**
 * @brief Send data to a specific destination (for DGRAM sockets).
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @param dst_node Destination node address.
 * @param dst_port Destination port number.
 * @return Number of bytes sent, or negative on error.
 */
int32_t danp_send_to(danp_socket_t *sock, void *data, uint16_t len, uint16_t dst_node, uint16_t dst_port);

/**
 * @brief Receive data from any source (for DGRAM sockets).
 * @param sock Pointer to the socket.
 * @param buffer Pointer to the buffer to store received data.
 * @param max_len Maximum length of the buffer.
 * @param src_node Pointer to store the source node address.
 * @param src_port Pointer to store the source port number.
 * @param timeout_ms Timeout in milliseconds.
 * @return Number of bytes received, or negative on error/timeout.
 */
int32_t danp_recv_from(
    danp_socket_t *sock,
    void *buffer,
    uint16_t max_len,
    uint16_t *src_node,
    uint16_t *src_port,
    uint32_t timeout_ms);


void danp_print_stats(void (*print_func)(const char *fmt, ...));

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_H */
