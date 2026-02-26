/// danpType.h - one line definition

/// All Rights Reserved

#ifndef INC_DANP_TYPES_H
#define INC_DANP_TYPES_H

/// Includes

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "osal/osal_message_queue.h"
#include "osal/osal_semaphore.h"

#include "danp/danp_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// Configurations


/// Definitions

/// @brief Weak attribute for overridable functions.
#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

/// @brief Packed attribute for structures.
#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

/// @brief Macro to mark unused variables and avoid compiler warnings.
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/// Types

/// @brief Packet flags for control and state.
typedef enum danp_flags_e
{
    DANP_FLAG_NONE = 0x00, ///< No flags set.
    DANP_FLAG_SYN = 0x01,  ///< Connection Request.
    DANP_FLAG_ACK = 0x02,  ///< Acknowledge (used for both Connect and Data).
    DANP_FLAG_RST = 0x04   ///< Reset Connection.
} danp_flags_t;
 
/// @brief Log levels for the library.
typedef enum danp_log_level_e
{
    DANP_LOG_LEVEL_VER = 0,    ///< Verbose logging.
    DANP_LOG_LEVEL_DBG,        ///< Debug logging.
    DANP_LOG_LEVEL_INF,        ///< Informational logging.
    DANP_LOG_LEVEL_WRN,        ///< Warning logging.
    DANP_LOG_LEVEL_ERR,        ///< Error logging.
    DANP_LOG_LEVEL_MAX
} danp_log_level_t;
 
/// @brief Socket types.
typedef enum danp_socket_type_e
{
    DANP_TYPE_DGRAM = 0, ///< Unreliable (UDP-like).
    DANP_TYPE_STREAM = 1 ///< Reliable (RDP/TCP-like).
} danp_socket_type_t;
 
/// @brief Socket states.
typedef enum danp_socket_state_e
{
    DANP_SOCK_CLOSED,       ///< Socket is unused or closed.
    DANP_SOCK_OPEN,         ///< Socket is allocated and bound, but not connected (DGRAM default).
    DANP_SOCK_LISTENING,    ///< Socket is waiting for incoming connections (STREAM).
    DANP_SOCK_SYN_SENT,     ///< Connection initiated, waiting for SYN-ACK (STREAM).
    DANP_SOCK_SYN_RECEIVED, ///< SYN received, waiting for final ACK (STREAM).
    DANP_SOCK_ESTABLISHED   ///< Connection established (STREAM) or Default Peer Set (DGRAM).
} danp_socket_state_t;
 
/// @brief Handle for an OS queue.
typedef osal_message_queue_handle_t danp_os_queue_handle_t;
 
/// @brief Handle for an OS semaphore.
typedef osal_semaphore_handle_t danp_os_semaphore_handle_t;
 
/// @brief Structure representing a DANP socket (Opaque).
typedef struct danp_socket_s danp_socket_t;
 
/// @brief Structure representing a DANP packet (Opaque).
typedef struct danp_packet_s danp_packet_t;

/// @brief Structure representing interface operations.
typedef struct danp_interface_ops_s
{
    int32_t (*tx)(void *iface_common, danp_packet_t *packet);
} danp_interface_ops_t;

/// @brief Structure representing interface state data.
typedef struct danp_interface_data_s
{
    bool is_running;
    bool is_exiting;
} danp_interface_data_t;

/// @brief Structure representing a network interface.
typedef struct danp_interface_s
{
    const char *name; ///< Name of the interface.
    uint16_t address; ///< Address of the interface.
    uint16_t mtu;     ///< Maximum Transmission Unit.

    danp_interface_ops_t ops;    ///< Interface operations.
    danp_interface_data_t data;  ///< Interface state data.

    struct danp_interface_s *next; ///< Pointer to the next interface in the list.
} danp_interface_t;

/// External Declarations

#ifdef __cplusplus
}
#endif

#endif /// INC_DANP_TYPES_H
