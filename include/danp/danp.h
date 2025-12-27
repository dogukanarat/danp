/* danp.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_H
#define INC_DANP_H

/* Includes */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "danp/danp_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Configurations */


/* Definitions */


/* Types */


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
 * @brief Get the number of free packets in the pool.
 * @return Number of free packets available.
 */
size_t danp_buffer_get_free_count(void);

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

/**
 * @brief Send a packet directly without copying (zero-copy TX).
 * @param sock Pointer to the socket.
 * @param pkt Pointer to the packet to send (ownership transfers to stack).
 * @return 0 on success, negative on error.
 */
int32_t danp_send_packet(danp_socket_t *sock, danp_packet_t *pkt);

/**
 * @brief Send a packet to specific destination without copying (zero-copy TX for DGRAM).
 * @param sock Pointer to the socket.
 * @param pkt Pointer to the packet to send (ownership transfers to stack).
 * @param dst_node Destination node address.
 * @param dst_port Destination port number.
 * @return 0 on success, negative on error.
 */
int32_t danp_send_packet_to(danp_socket_t *sock, danp_packet_t *pkt, uint16_t dst_node, uint16_t dst_port);

/**
 * @brief Receive a packet directly without copying (zero-copy RX).
 * @param sock Pointer to the socket.
 * @param timeout_ms Timeout in milliseconds.
 * @return Pointer to received packet (caller must free), or NULL on timeout/error.
 */
danp_packet_t *danp_recv_packet(danp_socket_t *sock, uint32_t timeout_ms);

/**
 * @brief Receive a packet from any source without copying (zero-copy RX for DGRAM).
 * @param sock Pointer to the socket.
 * @param src_node Pointer to store source node address (optional, can be NULL).
 * @param src_port Pointer to store source port number (optional, can be NULL).
 * @param timeout_ms Timeout in milliseconds.
 * @return Pointer to received packet (caller must free), or NULL on timeout/error.
 */
danp_packet_t *danp_recv_packet_from(
    danp_socket_t *sock,
    uint16_t *src_node,
    uint16_t *src_port,
    uint32_t timeout_ms);

/**
 * @brief Send data with automatic fragmentation for large messages (SFP).
 * @param sock Pointer to the socket.
 * @param data Pointer to data to send.
 * @param len Length of data (can exceed DANP_MAX_PACKET_SIZE).
 * @return Total bytes sent on success, negative on error.
 */
int32_t danp_send_sfp(danp_socket_t *sock, void *data, uint16_t len);

/**
 * @brief Receive and reassemble fragmented message (SFP).
 * @param sock Pointer to the socket.
 * @param timeout_ms Timeout in milliseconds.
 * @return Pointer to first packet in chain (caller must free all), or NULL on timeout/error.
 */
danp_packet_t *danp_recv_sfp(danp_socket_t *sock, uint32_t timeout_ms);

void danp_print_stats(void (*print_func)(const char *fmt, ...));

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_H */
