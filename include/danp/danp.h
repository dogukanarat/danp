/* danp.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_H
#define INC_DANP_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */

#include "danp/danpTypes.h"
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
typedef enum danpPacketFlags_e
{
    DANP_FLAG_NONE = 0x00, /**< No flags set. */
    DANP_FLAG_SYN = 0x01,  /**< Connection Request. */
    DANP_FLAG_ACK = 0x02,  /**< Acknowledge (used for both Connect and Data). */
    DANP_FLAG_RST = 0x04   /**< Reset Connection. */
} danpPacketFlags_t;

/**
 * @brief Log levels for the library.
 */
typedef enum danpLogLevel_e
{
    DANP_LOG_VERBOSE = 0, /**< Verbose logging. */
    DANP_LOG_DEBUG,       /**< Debug logging. */
    DANP_LOG_INFO,        /**< Informational logging. */
    DANP_LOG_WARN,        /**< Warning logging. */
    DANP_LOG_ERROR        /**< Error logging. */
} danpLogLevel_t;

/**
 * @brief Socket types.
 */
typedef enum danpSocketType_e
{
    DANP_TYPE_DGRAM = 0, /**< Unreliable (UDP-like). */
    DANP_TYPE_STREAM = 1 /**< Reliable (RDP/TCP-like). */
} danpSocketType_t;

/**
 * @brief Socket states.
 */
typedef enum danpSocketState_e
{
    DANP_SOCK_CLOSED,    /**< Socket is unused or closed. */
    DANP_SOCK_OPEN,      /**< Socket is allocated and bound, but not connected (DGRAM default). */
    DANP_SOCK_LISTENING, /**< Socket is waiting for incoming connections (STREAM). */
    DANP_SOCK_SYN_SENT,  /**< Connection initiated, waiting for SYN-ACK (STREAM). */
    DANP_SOCK_SYN_RECEIVED, /**< SYN received, waiting for final ACK (STREAM). */
    DANP_SOCK_ESTABLISHED   /**< Connection established (STREAM) or Default Peer Set (DGRAM). */
} danpSocketState_t;

/** @brief Handle for an OS queue. */
typedef osalMessageQueueHandle_t danpOsQueueHandle_t;

/** @brief Handle for an OS semaphore. */
typedef osalSemaphoreHandle_t danpOsSemaphoreHandle_t;

/* Header Packing Details (omitted for brevity) */

/**
 * @brief Structure representing a DANP packet.
 */
typedef struct danpPacket_e
{
    uint32_t headerRaw;                    /**< Raw header data. */
    uint16_t length;                       /**< Length of the payload. */
    uint8_t payload[DANP_MAX_PACKET_SIZE]; /**< Payload data. */

    struct danpInterface_s *rxInterface; /**< Interface where the packet was received. */
} danpPacket_t;

/**
 * @brief Structure representing a DANP socket.
 */
typedef struct danpSocket_s
{
    danpSocketState_t state; /**< Current state of the socket. */
    danpSocketType_t type;   /**< Type of the socket. */

    // Addressing
    uint16_t localPort;  /**< Local port number. */
    uint16_t localNode;  /**< Local node address. */
    uint16_t remoteNode; /**< Remote node address. */
    uint16_t remotePort; /**< Remote port number. */

    // Reliability State (Stop-and-Wait)
    uint8_t txSeq;         /**< Transmit sequence number. */
    uint8_t rxExpectedSeq; /**< Expected receive sequence number. */

    // RTOS Handles
    danpOsQueueHandle_t rxQueue;     /**< Queue for received packets. */
    danpOsQueueHandle_t acceptQueue; /**< Queue for accepted connections. */
    danpOsSemaphoreHandle_t signal;  /**< Semaphore for signaling. */

    struct danpSocket_s *next; /**< Pointer to the next socket in the list. */
} danpSocket_t;

/**
 * @brief Structure representing a network interface.
 */
typedef struct danpInterface_s
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
    int32_t (*txFunc)(void *ifaceCommon, danpPacket_t *packet);

    struct danpInterface_s *next; /**< Pointer to the next interface in the list. */
} danpInterface_t;

/**
 * @brief Callback function type for logging.
 * @param level Log level.
 * @param funcName Name of the function generating the log.
 * @param message Log message format string.
 * @param args Variable arguments list.
 */
typedef void (*danpLogFunctionCallback)(
    danpLogLevel_t level,
    const char *funcName,
    const char *message,
    va_list args);

/**
 * @brief Configuration structure for DANP initialization.
 */
typedef struct danpConfig_s
{
    uint16_t localNode;                  /**< Local node address. */
    danpLogFunctionCallback logFunction; /**< Logging callback function. */
} danpConfig_t;

/* External Declarations */

// Core functions

/**
 * @brief Initialize the DANP library.
 * @param config Pointer to the configuration structure.
 */
void danpInit(const danpConfig_t *config);

/**
 * @brief Pack a DANP header.
 * @param priority Packet priority.
 * @param dstNode Destination node address.
 * @param srcNode Source node address.
 * @param dstPort Destination port.
 * @param srcPort Source port.
 * @param flags Packet flags.
 * @return Packed header as a 32-bit integer.
 */
uint32_t danpPackHeader(
    uint8_t priority,
    uint16_t dstNode,
    uint16_t srcNode,
    uint8_t dstPort,
    uint8_t srcPort,
    uint8_t flags);

/**
 * @brief Unpack a DANP header.
 * @param raw Raw 32-bit header.
 * @param dstNode Pointer to store destination node address.
 * @param srcNode Pointer to store source node address.
 * @param dstPort Pointer to store destination port.
 * @param srcPort Pointer to store source port.
 * @param flags Pointer to store packet flags.
 */
void danpUnpackHeader(
    uint32_t raw,
    uint16_t *dstNode,
    uint16_t *srcNode,
    uint8_t *dstPort,
    uint8_t *srcPort,
    uint8_t *flags);

/**
 * @brief Log a message using the registered callback.
 * @param level Log level.
 * @param funcName Name of the function.
 * @param message Message format string.
 * @param ... Variable arguments.
 */
void danpLogMessage(danpLogLevel_t level, const char *funcName, const char *message, ...);

/**
 * @brief Route a packet for transmission.
 * @param packet Pointer to the packet to route.
 * @return 0 on success, negative on error.
 */
int32_t danpRouteTx(danpPacket_t *packet);

/**
 * @brief Process incoming data from an interface.
 * @param iface Pointer to the interface receiving data.
 * @param data Pointer to the received data.
 * @param length Length of the received data.
 */
void danpInput(danpInterface_t *iface, uint8_t *data, uint16_t length);

/**
 * @brief Allocate a packet from the pool.
 * @return Pointer to the allocated packet, or NULL if pool is empty.
 */
danpPacket_t *danpBufferAllocate(void);

/**
 * @brief Free a packet back to the pool.
 * @param packet Pointer to the packet to free.
 */
void danpBufferFree(danpPacket_t *packet);

/**
 * @brief Register a network interface.
 * @param iface Pointer to the interface to register.
 */
void danpRegisterInterface(void *iface);

// Socket API

/**
 * @brief Initialize the socket subsystem.
 * @return int32_t
 */
int32_t danpSocketInit(void);

/**
 * @brief Create a new DANP socket.
 * @param type Type of the socket (DGRAM or STREAM).
 * @return Pointer to the created socket, or NULL on failure.
 */
danpSocket_t *danpSocket(danpSocketType_t type);

/**
 * @brief Bind a socket to a local port.
 * @param sock Pointer to the socket.
 * @param port Local port to bind to.
 * @return 0 on success, negative on error.
 */
int32_t danpBind(danpSocket_t *sock, uint16_t port);

/**
 * @brief Listen for incoming connections on a socket.
 * @param sock Pointer to the socket.
 * @param backlog Maximum number of pending connections (currently unused).
 * @return 0 on success, negative on error.
 */
int32_t danpListen(danpSocket_t *sock, int backlog);

/**
 * @brief Close a socket and release resources.
 * @param sock Pointer to the socket to close.
 * @return 0 on success, negative on error.
 */
int32_t danpClose(danpSocket_t *sock); // Added close function

// Functions updated with timeoutMs

/**
 * @brief Accept a new connection on a listening socket.
 * @param serverSock Pointer to the listening socket.
 * @param timeoutMs Timeout in milliseconds.
 * @return Pointer to the new connected socket, or NULL on timeout/error.
 */
danpSocket_t *danpAccept(danpSocket_t *serverSock, uint32_t timeoutMs);

/**
 * @brief Connect a socket to a remote node and port.
 * @param sock Pointer to the socket.
 * @param node Remote node address.
 * @param port Remote port number.
 * @return 0 on success, negative on error.
 */
int32_t danpConnect(danpSocket_t *sock, uint16_t node, uint16_t port);

/**
 * @brief Send data over a connected socket.
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @return Number of bytes sent, or negative on error.
 */
int32_t danpSend(danpSocket_t *sock, void *data, uint16_t len);

/**
 * @brief Receive data from a connected socket.
 * @param sock Pointer to the socket.
 * @param buffer Pointer to the buffer to store received data.
 * @param maxLen Maximum length of the buffer.
 * @param timeoutMs Timeout in milliseconds.
 * @return Number of bytes received, or negative on error/timeout.
 */
int32_t danpRecv(danpSocket_t *sock, void *buffer, uint16_t maxLen, uint32_t timeoutMs);

/**
 * @brief Send data to a specific destination (for DGRAM sockets).
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @param dstNode Destination node address.
 * @param dstPort Destination port number.
 * @return Number of bytes sent, or negative on error.
 */
int32_t
danpSendTo(danpSocket_t *sock, void *data, uint16_t len, uint16_t dstNode, uint16_t dstPort);

/**
 * @brief Receive data from any source (for DGRAM sockets).
 * @param sock Pointer to the socket.
 * @param buffer Pointer to the buffer to store received data.
 * @param maxLen Maximum length of the buffer.
 * @param srcNode Pointer to store the source node address.
 * @param srcPort Pointer to store the source port number.
 * @param timeoutMs Timeout in milliseconds.
 * @return Number of bytes received, or negative on error/timeout.
 */
int32_t danpRecvFrom(
    danpSocket_t *sock,
    void *buffer,
    uint16_t maxLen,
    uint16_t *srcNode,
    uint16_t *srcPort,
    uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_H */
