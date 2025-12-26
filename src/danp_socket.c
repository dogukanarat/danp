/* danp_socket.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <stdio.h>

#include "osal/osal_mutex.h"
#include "osal/osal_time.h"

#include "danp/danp_types.h"
#include "danp/danp.h"
#include "danp_debug.h"

/* Imports */

extern danp_config_t danp_config;

/* Definitions */

#define DANP_MAX_SOCKET_COUNT              (20)

/* Types */


/* Forward Declarations */


/* Variables */

/** @brief List of active sockets. */
static danp_socket_t *socket_list;

/** @brief Next available ephemeral port. */
static uint16_t next_ephemeral_port = 1;

/** @brief Mutex for socket operations. */
static osal_mutex_handle_t mutex_socket;

static danp_socket_t socket_pool[DANP_MAX_SOCKET_COUNT];

static bool danp_port_in_use(uint16_t port)
{
    danp_socket_t *cur = socket_list;
    size_t guard = 0U;

    while (cur)
    {
        if (guard++ >= DANP_MAX_SOCKET_COUNT)
        {
            break;
        }

        if (cur->state != DANP_SOCK_CLOSED && cur->local_port == port)
        {
            return true;
        }

        cur = cur->next;
    }

    return false;
}

/* Functions */

/**
 * @brief Find a socket matching the given parameters.
 * @param local_port Local port number.
 * @param remote_node Remote node address.
 * @param remote_port Remote port number.
 * @return Pointer to the matching socket, or NULL if not found.
 */
static danp_socket_t *danp_find_socket(uint16_t local_port, uint16_t remote_node, uint16_t remote_port)
{
    danp_socket_t *cur = socket_list;
    danp_socket_t *ret = NULL;
    size_t i = 0;

    while (cur)
    {
        if (i >= DANP_MAX_SOCKET_COUNT)
        {
            // Safety check to prevent infinite loops
            break;
        }
        if (cur->local_port == local_port)
        {
            // Priority 1: Exact Match (Established/Connecting streams or Connected DGRAM)
            if (cur->remote_node == remote_node && cur->remote_port == remote_port &&
                (cur->state == DANP_SOCK_ESTABLISHED || cur->state == DANP_SOCK_SYN_SENT ||
                 cur->state == DANP_SOCK_SYN_RECEIVED))
            {
                ret = cur;
                break;
            }

            // Priority 2: Wildcard Match (Listening stream or Open/Bound DGRAM socket)
            if (cur->state == DANP_SOCK_LISTENING || (cur->type == DANP_TYPE_DGRAM && cur->state == DANP_SOCK_OPEN))
            {
                ret = cur;
                break;
            }
        }
        cur = cur->next;
        i++;
    }

    return ret;
}

/**
 * @brief Send a control packet.
 * @param sock Pointer to the socket.
 * @param flags Control flags to send.
 * @param seq_num Sequence number (for ACK packets).
 */
static void danp_send_control(danp_socket_t *sock, uint8_t flags, uint8_t seq_num)
{
    danp_packet_t *pkt = danp_buffer_allocate();

    for (;;)
    {
        if (!pkt)
        {
            danp_log_message(DANP_LOG_ERROR, "Failed to allocate control packet");
            break;
        }

        pkt->header_raw = danp_pack_header(
            0,
            sock->remote_node,
            sock->local_node,
            sock->remote_port,
            sock->local_port,
            flags);

        if ((flags & DANP_FLAG_ACK) && sock->type == DANP_TYPE_STREAM)
        {
            pkt->payload[0] = seq_num;
            pkt->length = 1;
        }
        else
        {
            pkt->length = 0;
        }

        danp_route_tx(pkt);
        danp_buffer_free(pkt);

        break;
    }
}

/**
 * @brief Initialize the socket subsystem.
 * @return int32_t
 */
int32_t danp_socket_init(void)
{
    osal_mutex_attr_t attr = {
        .name = "danpSocketMutex",
        .attr_bits = OSAL_MUTEX_RECURSIVE,
        .cb_mem = NULL,
        .cb_size = 0,
    };
    mutex_socket = osal_mutex_create(&attr);
    if (!mutex_socket)
    {
        danp_log_message(DANP_LOG_ERROR, "Failed to create socket mutex");
        return -1;
    }

    // Initialize socket pool
    for (int i = 0; i < DANP_MAX_SOCKET_COUNT; i++)
    {
        socket_pool[i].state = DANP_SOCK_CLOSED;
        socket_pool[i].next = NULL;
    }

    socket_list = NULL;

    return 0;
}

/**
 * @brief Create a new DANP socket.
 * @param type Type of the socket (DGRAM or STREAM).
 * @return Pointer to the created socket, or NULL on failure.
 */
danp_socket_t *danp_socket(danp_socket_type_t type)
{
    osal_status_t osal_status;
    bool is_mutex_taken = false;
    danp_socket_t *created_socket = NULL;
    danp_socket_t *slot = NULL;
    osal_message_queue_handle_t rx_q = NULL;
    osal_message_queue_handle_t acc_q = NULL;
    osal_semaphore_handle_t sig = NULL;
    osal_message_queue_attr_t mq_attr = { .name = "danpSockRx", .mq_size = 0 /*...*/ };
    osal_semaphore_attr_t sem_attr   = { .name = "danpSockSig", .max_count = 1 /*...*/ };

    for (;;)
    {
        osal_status = osal_mutex_lock(mutex_socket, OSAL_WAIT_FOREVER);
        if (osal_status != OSAL_SUCCESS)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket allocation failed: Mutex Lock Error");
            break; // Jump to cleanup
        }
        is_mutex_taken = true;

        for (int i = 0; i < DANP_MAX_SOCKET_COUNT; i++)
        {
            if (socket_pool[i].state == DANP_SOCK_CLOSED && socket_pool[i].local_port == 0)
            {
                slot = &socket_pool[i];
                break;
            }
        }

        if (slot == NULL)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket allocation failed: No free slots");
            break; // Jump to cleanup
        }

        if (socket_list == slot)
        {
            socket_list = slot->next;
        }
        else
        {
            for (int i = 0; i < DANP_MAX_SOCKET_COUNT; i++)
            {
                if (socket_pool[i].next == slot)
                {
                    socket_pool[i].next = slot->next;
                    break;
                }
            }
        }

        rx_q = slot->rx_queue;
        acc_q = slot->accept_queue;
        sig = slot->signal;

        memset(slot, 0, sizeof(danp_socket_t));

        slot->rx_queue = rx_q;
        slot->accept_queue = acc_q;
        slot->signal = sig;

        slot->type = type;
        slot->state = DANP_SOCK_OPEN; // Temporarily mark open
        slot->local_node = danp_config.local_node;

        if (slot->rx_queue == NULL)
        {
            slot->rx_queue = osal_message_queue_create(10, sizeof(danp_packet_t *), &mq_attr);
        }
        if (slot->accept_queue == NULL)
        {
            slot->accept_queue = osal_message_queue_create(5, sizeof(danp_socket_t *), &mq_attr);
        }
        if (slot->signal == NULL)
        {
            slot->signal = osal_semaphore_create(&sem_attr);
        }

        if (slot->rx_queue == NULL || slot->accept_queue == NULL || slot->signal == NULL)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket allocation failed: OS Resource Error");

            slot->state = DANP_SOCK_CLOSED;

            break; // Jump to cleanup
        }

        danp_packet_t *garbage_pkt;
        danp_socket_t *garbage_sock;

        while (osal_message_queue_receive(slot->rx_queue, &garbage_pkt, 0) == 0)
        {
            danp_buffer_free(garbage_pkt);
        }
        while (osal_message_queue_receive(slot->accept_queue, &garbage_sock, 0) == 0)
        {
            // Just drain
        }

        slot->next = socket_list;
        socket_list = slot;

        created_socket = slot;

        break;
    }

    if (is_mutex_taken)
    {
        osal_mutex_unlock(mutex_socket);
    }

    return created_socket;
}

/**
 * @brief Bind a socket to a local port.
 * @param sock Pointer to the socket.
 * @param port Local port to bind to.
 * @return 0 on success, negative on error.
 */
int32_t danp_bind(danp_socket_t *sock, uint16_t port)
{
    int32_t ret = 0;
    osal_status_t osal_status;
    bool is_mutex_taken = false;

    for (;;)
    {
        osal_status = osal_mutex_lock(mutex_socket, OSAL_WAIT_FOREVER);
        if (osal_status != OSAL_SUCCESS)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket bind failed: Mutex Lock Error");
            ret = -1;
            break;
        }
        is_mutex_taken = true;

        if (port == 0)
        {
            uint16_t start_port = next_ephemeral_port;
            do
            {
                if (!danp_port_in_use(next_ephemeral_port))
                {
                    port = next_ephemeral_port;
                    next_ephemeral_port++;
                    if (next_ephemeral_port >= DANP_MAX_PORTS)
                    {
                        next_ephemeral_port = 1;
                    }
                    break;
                }

                next_ephemeral_port++;
                if (next_ephemeral_port >= DANP_MAX_PORTS)
                {
                    next_ephemeral_port = 1;
                }
            } while (next_ephemeral_port != start_port);

            if (port == 0)
            {
                danp_log_message(DANP_LOG_ERROR, "Socket bind failed: no ephemeral ports available");
                ret = -1;
                break;
            }
        }

        if (port >= DANP_MAX_PORTS)
        {
            ret = -1;
            break;
        }

        if (danp_port_in_use(port))
        {
            danp_log_message(DANP_LOG_ERROR, "Socket bind failed: port %u already in use", port);
            ret = -1;
            break;
        }

        sock->local_port = port;
        danp_log_message(DANP_LOG_INFO, "Socket bound to port %u", port);

        break;
    }

    if (is_mutex_taken)
    {
        osal_mutex_unlock(mutex_socket);
    }

    return ret;
}

/**
 * @brief Listen for incoming connections on a socket.
 * @param sock Pointer to the socket.
 * @param backlog Maximum number of pending connections (currently unused).
 * @return 0 on success, negative on error.
 */
int danp_listen(danp_socket_t *sock, int backlog)
{
    UNUSED(backlog);
    sock->state = DANP_SOCK_LISTENING;
    return 0;
}

/**
 * @brief Close a socket and release resources.
 * @param sock Pointer to the socket to close.
 * @return 0 on success, negative on error.
 */
int32_t danp_close(danp_socket_t *sock)
{
    osal_status_t osal_status;
    bool is_mutex_taken = false;

    osal_status = osal_mutex_lock(mutex_socket, OSAL_WAIT_FOREVER);
    if (osal_status != OSAL_SUCCESS)
    {
        danp_log_message(DANP_LOG_ERROR, "Socket close failed: Mutex Lock Error");
        return -1;
    }
    is_mutex_taken = true;

    // Only send RST for STREAM sockets or connected DGRAM sockets that are actually in a state where RST makes sense.
    // For DGRAM, we generally don't send RST on close unless we want to signal the peer to stop sending.
    // However, standard UDP doesn't do this. Let's restrict RST to STREAM.
    if (sock->type == DANP_TYPE_STREAM &&
        (sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_SENT ||
         sock->state == DANP_SOCK_SYN_RECEIVED))
    {
        danp_send_control(sock, DANP_FLAG_RST, 0);
    }

    // Unlink from global socket_list
    if (socket_list == sock)
    {
        socket_list = sock->next;
    }
    else
    {
        danp_socket_t *prev = socket_list;
        while (prev && prev->next != sock)
        {
            prev = prev->next;
        }
        if (prev && prev->next == sock)
        {
            prev->next = sock->next;
        }
    }
    sock->next = NULL;

    // Clean up state for slot recycling
    sock->state = DANP_SOCK_CLOSED;
    sock->local_port = 0;

    // Note: Queue and semaphore are kept alive for quick reuse of the slot.

    if (is_mutex_taken)
    {
        osal_mutex_unlock(mutex_socket);
    }

    return 0;
}


/**
 * @brief Connect a socket to a remote node and port.
 * @param sock Pointer to the socket.
 * @param node Remote node address.
 * @param port Remote port number.
 * @return 0 on success, negative on error.
 */
int32_t danp_connect(danp_socket_t *sock, uint16_t node, uint16_t port)
{
    int32_t ret = 0;

    for (;;)
    {
        if (sock->local_port == 0)
        {
            danp_bind(sock, 0);
        }
        sock->remote_node = node;
        sock->remote_port = port;

        if (sock->type == DANP_TYPE_DGRAM)
        {
            // For DGRAM, "Connect" just sets the default destination.
            // We use ESTABLISHED to indicate that the socket has a default peer,
            // allowing danp_find_socket to match incoming packets from this peer specifically.
            sock->state = DANP_SOCK_ESTABLISHED;
            break;
        }

        danp_log_message(
            DANP_LOG_INFO,
            "Connecting to Node %d Port %d from Local Port %d",
            node,
            port,
            sock->local_port);
        sock->state = DANP_SOCK_SYN_SENT;
        danp_send_control(sock, DANP_FLAG_SYN, 0);

        if (0 == osal_semaphore_take(sock->signal, DANP_ACK_TIMEOUT_MS))
        {
            danp_log_message(DANP_LOG_INFO, "Connection Established");
            break;
        }

        sock->state = DANP_SOCK_OPEN; // Reset state on timeout
        danp_log_message(DANP_LOG_WARN, "Connect Timeout");

        ret = -1;

        break;
    }

    return ret;
}

/**
 * @brief Accept a new connection on a listening socket.
 * @param server_sock Pointer to the listening socket.
 * @param timeout_ms Timeout in milliseconds.
 * @return Pointer to the new connected socket, or NULL on timeout/error.
 */
danp_socket_t *danp_accept(danp_socket_t *server_sock, uint32_t timeout_ms)
{
    danp_socket_t *client = NULL;

    for (;;)
    {
        if (0 == osal_message_queue_receive(server_sock->accept_queue, &client, timeout_ms))
        {
            break;
        }

        break;
    }

    return client;
}

/**
 * @brief Send data over a connected socket.
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @return Number of bytes sent, or negative on error.
 */
int32_t danp_send(danp_socket_t *sock, void *data, uint16_t len)
{
    int32_t ret = 0;
    int32_t retries = 0;
    bool ack_received = false;
    danp_packet_t *pkt = NULL;

    for (;;)
    {
        if (len > DANP_MAX_PACKET_SIZE - 1)
        {
            ret = -1;
            break;
        }

        if (sock->type == DANP_TYPE_DGRAM)
        {
            danp_packet_t *pkt = danp_buffer_allocate();
            if (!pkt)
            {
                ret = -1;
                break;
            }
            pkt->header_raw = danp_pack_header(
                0,
                sock->remote_node,
                sock->local_node,
                sock->remote_port,
                sock->local_port,
                DANP_FLAG_NONE);
            memcpy(pkt->payload, data, len);
            pkt->length = len;
            danp_route_tx(pkt);
            danp_buffer_free(pkt);
            ret = len;
            break;
        }

        while (retries < DANP_RETRY_LIMIT && !ack_received)
        {
            pkt = danp_buffer_allocate();
            if (!pkt)
            {
                osal_delay_ms(10);
                continue;
            }
            pkt->header_raw = danp_pack_header(
                0,
                sock->remote_node,
                sock->local_node,
                sock->remote_port,
                sock->local_port,
                DANP_FLAG_NONE);
            pkt->payload[0] = sock->tx_seq;
            memcpy(pkt->payload + 1, data, len);
            pkt->length = len + 1;
            danp_route_tx(pkt);
            danp_buffer_free(pkt);

            if (0 == osal_semaphore_take(sock->signal, DANP_ACK_TIMEOUT_MS))
            {
                ack_received = true;
            }
            retries++;
        }

        if (ack_received)
        {
            sock->tx_seq++;
            ret = len;
            break;
        }
        else
        {
            ret = -1;
            break;
        }

        break;
    }

    return ret;
}

/**
 * @brief Receive data from a connected socket.
 * @param sock Pointer to the socket.
 * @param buffer Pointer to the buffer to store received data.
 * @param max_len Maximum length of the buffer.
 * @param timeout_ms Timeout in milliseconds.
 * @return Number of bytes received, or negative on error/timeout.
 */
int32_t danp_recv(danp_socket_t *sock, void *buffer, uint16_t max_len, uint32_t timeout_ms)
{
    int32_t ret = 0;
    danp_packet_t *pkt = NULL;
    int32_t copy_len = 0;

    if (0 == osal_message_queue_receive(sock->rx_queue, &pkt, timeout_ms))
    {
        if (pkt == NULL)
        {
            // Socket closed or reset
            return 0;
        }

        if (sock->type == DANP_TYPE_DGRAM)
        {
            copy_len = (pkt->length > max_len) ? max_len : pkt->length;
            memcpy(buffer, pkt->payload, copy_len);
        }
        else
        {
            if (pkt->length > 0)
            {
                copy_len = (pkt->length - 1 > max_len) ? max_len : (pkt->length - 1);
                memcpy(buffer, pkt->payload + 1, copy_len);
            }
        }
        danp_buffer_free(pkt);
        ret = copy_len;
    }
    return ret;
}

/**
 * @brief Handle incoming packets for sockets.
 * @param pkt Pointer to the received packet.
 */
void danp_socket_input_handler(danp_packet_t *pkt)
{
    uint16_t dst = 0;
    uint16_t src = 0;
    uint8_t dst_port = 0;
    uint8_t src_port = 0;
    uint8_t flags = 0;
    uint8_t acked_seq = 0;
    uint8_t seq = 0;
    // bool isConnected = false;
    danp_socket_t *child = NULL;
    danp_packet_t *garbage;
    bool is_mutex_taken = false;
    osal_status_t osal_status;

    for (;;)
    {
        osal_status = osal_mutex_lock(mutex_socket, OSAL_WAIT_FOREVER);
        if (osal_status != OSAL_SUCCESS)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket Input Handler: Mutex Lock Error");
            break;
        }
        is_mutex_taken = true;

        danp_unpack_header(pkt->header_raw, &dst, &src, &dst_port, &src_port, &flags);
        danp_socket_t *sock = danp_find_socket(dst_port, src, src_port);

        if (flags == DANP_FLAG_RST)
        {
            if (sock)
            {
                // If remote_port is 0, it is a listener/unbound-source socket.
                // isConnected = (sock->remote_port != 0);

                if (sock->type == DANP_TYPE_STREAM)
                {
                    danp_log_message(
                        DANP_LOG_INFO,
                        "Received RST from peer. Closing socket to Port %u.",
                        dst_port);
                    sock->state = DANP_SOCK_CLOSED;
                    sock->local_port = 0;

                    // Wake up any waiters on recv
                    danp_packet_t *null_pkt = NULL;
                    osal_message_queue_send(sock->rx_queue, &null_pkt, 0);
                }
                else
                {
                    // For DGRAM, RST is advisory or ignored. We don't close the socket because DGRAM is connectionless.
                    danp_log_message(DANP_LOG_WARN, "Ignored RST on DGRAM socket Port %u", dst_port);
                }
            }
            danp_buffer_free(pkt);
            break;
        }

        if (!sock)
        {
            danp_log_message(DANP_LOG_WARN, "No socket found for Port %u", dst_port);
            danp_buffer_free(pkt);
            break;
        }

        if ((sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_RECEIVED) &&
            (flags & DANP_FLAG_SYN))
        {
            danp_log_message(DANP_LOG_WARN, "Received SYN on active socket. Peer restart/resync. State reset.");

            if (sock->type == DANP_TYPE_STREAM)
            {
                sock->tx_seq = 0;
                sock->rx_expected_seq = 0;

                while (0 == osal_message_queue_receive(sock->rx_queue, &garbage, 0))
                {
                    danp_buffer_free(garbage); // Clear out old data
                }
            }

            danp_send_control(sock, DANP_FLAG_ACK | DANP_FLAG_SYN, 0);
            sock->state = DANP_SOCK_SYN_RECEIVED;

            danp_buffer_free(pkt);
            break;
        }

        if (sock->state == DANP_SOCK_LISTENING && (flags & DANP_FLAG_SYN))
        {
            danp_log_message(DANP_LOG_INFO, "Received SYN from Node %d Port %d", src, src_port);

            child = danp_socket(sock->type);
            if (!child)
            {
                danp_buffer_free(pkt);
                break;
            }

            child->local_node = danp_config.local_node;
            child->local_port = dst_port;
            child->remote_node = src;
            child->remote_port = src_port;

            child->state = DANP_SOCK_SYN_RECEIVED; // Set state and wait for final ACK

            if (0 != osal_message_queue_send(sock->accept_queue, &child, 0))
            {
                child->state = DANP_SOCK_CLOSED;
                child->local_port = 0;
                danp_buffer_free(pkt);
                break;
            }

            danp_send_control(child, DANP_FLAG_ACK | DANP_FLAG_SYN, 0);
            danp_buffer_free(pkt);
            break;
        }

        if (sock->state == DANP_SOCK_SYN_SENT && (flags & DANP_FLAG_ACK))
        {
            sock->state = DANP_SOCK_ESTABLISHED;
            danp_send_control(sock, DANP_FLAG_ACK, 0); // Send final ACK
            osal_semaphore_give(sock->signal);
            danp_buffer_free(pkt);
            break;
        }

        if (sock->state == DANP_SOCK_SYN_RECEIVED && (flags & DANP_FLAG_ACK) &&
            !(flags & DANP_FLAG_SYN))
        {
            // Final ACK received from client/resync, connection is fully active
            sock->state = DANP_SOCK_ESTABLISHED;
            danp_buffer_free(pkt);
            break;
        }


        // --- 2. DATA HANDLING ---
        if ((flags & DANP_FLAG_ACK) && !(flags & DANP_FLAG_SYN) && pkt->length == 1)
        {
            if (sock->type == DANP_TYPE_STREAM)
            {
                acked_seq = pkt->payload[0];
                if (acked_seq == sock->tx_seq)
                {
                    osal_semaphore_give(sock->signal);
                }
            }
            danp_buffer_free(pkt);
            break;
        }

        if ((sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_RECEIVED ||
             (sock->type == DANP_TYPE_DGRAM && sock->state == DANP_SOCK_OPEN)) &&
            pkt->length > 0)
        {
            if (sock->type == DANP_TYPE_DGRAM)
            {
                osal_message_queue_send(sock->rx_queue, &pkt, 0);
                break;
            }
            else if (sock->type == DANP_TYPE_STREAM)
            {
                seq = pkt->payload[0];

                if (sock->state == DANP_SOCK_SYN_RECEIVED)
                {
                    sock->state = DANP_SOCK_ESTABLISHED;
                    danp_log_message(DANP_LOG_INFO, "Implicitly established connection via Data packet");
                }

                if (seq == sock->rx_expected_seq)
                {
                    sock->rx_expected_seq++;
                    danp_send_control(sock, DANP_FLAG_ACK, seq);
                    osal_message_queue_send(sock->rx_queue, &pkt, 0);
                }
                else
                {
                    danp_send_control(sock, DANP_FLAG_ACK, seq);
                    danp_buffer_free(pkt);
                }
            }

            break;
        }

        danp_buffer_free(pkt);

        break;
    }

    if (is_mutex_taken)
    {
        osal_mutex_unlock(mutex_socket);
    }
}

/**
 * @brief Send data to a specific destination (for DGRAM sockets).
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @param dst_node Destination node address.
 * @param dst_port Destination port number.
 * @return Number of bytes sent, or negative on error.
 */
int32_t danp_send_to(danp_socket_t *sock, void *data, uint16_t len, uint16_t dst_node, uint16_t dst_port)
{
    int32_t ret = 0;
    danp_packet_t *pkt;

    for (;;)
    {
        if (sock->type != DANP_TYPE_DGRAM)
        {
            ret = -1;
            break;
        }
        if (len > DANP_MAX_PACKET_SIZE - 1)
        {
            ret = -1;
            break;
        }
        pkt = danp_buffer_allocate();
        if (!pkt)
        {
            ret = -1;
            break;
        }

        pkt->header_raw = danp_pack_header(0, dst_node, sock->local_node, dst_port, sock->local_port, DANP_FLAG_NONE);
        memcpy(pkt->payload, data, len);
        pkt->length = len;
        danp_route_tx(pkt);
        danp_buffer_free(pkt);

        ret = len;

        break;
    }

    return ret;
}

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
    uint32_t timeout_ms)
{
    int32_t ret = -1;
    uint16_t dst = 0;
    uint16_t src = 0;
    uint8_t d_port = 0;
    uint8_t s_port = 0;
    uint8_t flags = 0;
    danp_packet_t *pkt;
    int copy_len;

    for (;;)
    {
        if (sock->type != DANP_TYPE_DGRAM)
        {
            break;
        }

        if (0 == osal_message_queue_receive(sock->rx_queue, &pkt, timeout_ms))
        {
            copy_len = (pkt->length > max_len) ? max_len : pkt->length;
            memcpy(buffer, pkt->payload, copy_len);

            danp_unpack_header(pkt->header_raw, &dst, &src, &d_port, &s_port, &flags);

            if (src_node)
            {
                *src_node = src;
            }
            if (src_port)
            {
                *src_port = s_port;
            }

            danp_buffer_free(pkt);
            ret = copy_len;
        }

        break;
    }

    return ret;
}

void danp_print_stats(void (*print_func)(const char *fmt, ...))
{
    if (print_func == NULL)
    {
        return;
    }

    print_func("DANP Socket Stats:\n");
    print_func("    Max Sockets: %d\n", DANP_MAX_SOCKET_COUNT);
    print_func("    Next Ephemeral Port: %u\n", next_ephemeral_port);
    print_func("    Active Sockets:\n");
    danp_socket_t *cur = socket_list;
    while (cur)
    {
        print_func("      Socket on Local Port %u - State: %d, Type: %d, Remote Node: %u, Remote Port: %u\n",
            cur->local_port,
            cur->state,
            cur->type,
            cur->remote_node,
            cur->remote_port);
        cur = cur->next;
    }
    print_func("\n");

    print_func("DANP Buffer Stats:\n");
    extern size_t danp_buffer_get_free_count(void);
    print_func("    Free Buffers: %zu\n", danp_buffer_get_free_count());

}
