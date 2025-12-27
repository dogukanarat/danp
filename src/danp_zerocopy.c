/* danpZerocopy.c - Zero-copy and SFP (Small Fragmentation Protocol) implementation */

/* All Rights Reserved */

/* Includes */

#include <errno.h>
#include <string.h>

#include "osal/osal_message_queue.h"

#include "danp/danp.h"
#include "danp/danp_buffer.h"
#include "danp_debug.h"



/* Imports */

/* Definitions */

/* Types */

/**
 * @brief SFP header structure (1 byte).
 */
typedef struct danp_sfp_header_s
{
    uint8_t flags_and_id; /**< Bits[7:6]=flags, Bits[5:0]=fragment_id */
} danp_sfp_header_t;

/* Forward Declarations */

/* Variables */

/* Functions */

/**
 * @brief Pack SFP header into a byte.
 * @param is_begin True if this is the first fragment.
 * @param has_more True if more fragments follow.
 * @param fragment_id Fragment identifier (0-63).
 * @return Packed SFP header byte.
 */
static uint8_t danp_sfp_pack_header(bool is_begin, bool has_more, uint8_t fragment_id)
{
    uint8_t header = fragment_id & 0x3F; /* Bits[5:0] = fragment_id */

    if (is_begin)
    {
        header |= DANP_SFP_FLAG_BEGIN; /* Bit 6 */
    }

    if (has_more)
    {
        header |= DANP_SFP_FLAG_MORE; /* Bit 7 */
    }

    return header;
}

/**
 * @brief Unpack SFP header from a byte.
 * @param header Packed SFP header byte.
 * @param is_begin Pointer to store begin flag.
 * @param has_more Pointer to store more-fragments flag.
 * @param fragment_id Pointer to store fragment identifier.
 */
static void danp_sfp_unpack_header(uint8_t header, bool *is_begin, bool *has_more, uint8_t *fragment_id)
{
    if (is_begin)
    {
        *is_begin = (header & DANP_SFP_FLAG_BEGIN) != 0;
    }

    if (has_more)
    {
        *has_more = (header & DANP_SFP_FLAG_MORE) != 0;
    }

    if (fragment_id)
    {
        *fragment_id = header & 0x3F;
    }
}

/**
 * @brief Send a packet directly without copying (zero-copy TX).
 * @param sock Pointer to the socket.
 * @param pkt Pointer to the packet to send (ownership transfers to stack).
 * @return 0 on success, negative on error.
 */
int32_t danp_send_packet(danp_socket_t *sock, danp_packet_t *pkt)
{
    int32_t ret = -1;

    for (;;)
    {
        if (!sock || !pkt)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid socket or packet");
            break;
        }

        if (sock->type != DANP_TYPE_DGRAM && sock->state != DANP_SOCK_ESTABLISHED)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket not connected");
            break;
        }

        /* Set header */
        pkt->header_raw = danp_pack_header(
            DANP_PRIORITY_NORMAL,
            sock->remote_node,
            sock->local_node,
            sock->remote_port,
            sock->local_port,
            DANP_FLAG_NONE);

        /* Route and transmit */
        if (danp_route_tx(pkt) < 0)
        {
            danp_log_message(DANP_LOG_ERROR, "Failed to route packet");
            danp_buffer_free(pkt);
            break;
        }

        /* For DGRAM, packet is sent immediately */
        if (sock->type == DANP_TYPE_DGRAM)
        {
            danp_buffer_free(pkt);
            ret = 0;
            break;
        }

        /* For STREAM, implement stop-and-wait ARQ (simplified) */
        /* NOTE: This is a simplified version. Full ARQ is in danp_send(). */
        danp_buffer_free(pkt);
        ret = 0;
        break;
    }

    return ret;
}

/**
 * @brief Send a packet to specific destination without copying (zero-copy TX for DGRAM).
 * @param sock Pointer to the socket.
 * @param pkt Pointer to the packet to send (ownership transfers to stack).
 * @param dst_node Destination node address.
 * @param dst_port Destination port number.
 * @return 0 on success, negative on error.
 */
int32_t danp_send_packet_to(danp_socket_t *sock, danp_packet_t *pkt, uint16_t dst_node, uint16_t dst_port)
{
    int32_t ret = -1;

    for (;;)
    {
        if (!sock || !pkt)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid socket or packet");
            break;
        }

        if (sock->type != DANP_TYPE_DGRAM)
        {
            danp_log_message(DANP_LOG_ERROR, "send_packet_to only valid for DGRAM sockets");
            danp_buffer_free(pkt);
            break;
        }

        /* Set header with explicit destination */
        pkt->header_raw = danp_pack_header(
            DANP_PRIORITY_NORMAL,
            dst_node,
            sock->local_node,
            dst_port,
            sock->local_port,
            DANP_FLAG_NONE);

        /* Route and transmit */
        if (danp_route_tx(pkt) < 0)
        {
            danp_log_message(DANP_LOG_ERROR, "Failed to route packet");
            danp_buffer_free(pkt);
            break;
        }

        danp_buffer_free(pkt);
        ret = 0;
        break;
    }

    return ret;
}

/**
 * @brief Receive a packet directly without copying (zero-copy RX).
 * @param sock Pointer to the socket.
 * @param timeout_ms Timeout in milliseconds.
 * @return Pointer to received packet (caller must free), or NULL on timeout/error.
 */
danp_packet_t *danp_recv_packet(danp_socket_t *sock, uint32_t timeout_ms)
{
    danp_packet_t *pkt = NULL;

    for (;;)
    {
        if (!sock)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid socket");
            break;
        }

        if (!sock->rx_queue)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket not initialized");
            break;
        }

        /* Receive packet pointer from queue */
        if (0 != osal_message_queue_receive(sock->rx_queue, &pkt, timeout_ms))
        {
            /* Timeout or error */
            break;
        }

        /* Packet successfully received */
        danp_log_message(DANP_LOG_VERBOSE, "Received packet (zero-copy) length=%u", pkt->length);
        break;
    }

    return pkt;
}

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
    uint32_t timeout_ms)
{
    danp_packet_t *pkt = NULL;
    uint16_t dst = 0;
    uint16_t src = 0;
    uint8_t d_port = 0;
    uint8_t s_port = 0;
    uint8_t flags = 0;

    for (;;)
    {
        if (!sock)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid socket");
            break;
        }

        if (sock->type != DANP_TYPE_DGRAM)
        {
            danp_log_message(DANP_LOG_ERROR, "recv_packet_from only valid for DGRAM sockets");
            break;
        }

        if (!sock->rx_queue)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket not initialized");
            break;
        }

        /* Receive packet pointer from queue */
        if (0 != osal_message_queue_receive(sock->rx_queue, &pkt, timeout_ms))
        {
            /* Timeout or error */
            break;
        }

        /* Extract source information from packet header */
        danp_unpack_header(pkt->header_raw, &dst, &src, &d_port, &s_port, &flags);

        if (src_node)
        {
            *src_node = src;
        }

        if (src_port)
        {
            *src_port = s_port;
        }

        /* Packet successfully received */
        danp_log_message(DANP_LOG_VERBOSE, "Received packet (zero-copy) from node=%u port=%u, length=%u", src, s_port, pkt->length);
        break;
    }

    return pkt;
}

/**
 * @brief Send data with automatic fragmentation for large messages (SFP).
 * @param sock Pointer to the socket.
 * @param data Pointer to data to send.
 * @param len Length of data (can exceed DANP_MAX_PACKET_SIZE).
 * @return Total bytes sent on success, negative on error.
 */
int32_t danp_send_sfp(danp_socket_t *sock, void *data, uint16_t len)
{
    int32_t ret = -1;
    uint16_t bytes_sent = 0;
    uint16_t offset = 0;
    uint8_t fragment_id = 0;
    /* Maximum user data per fragment (MTU - DANP header - SFP header) */
    uint16_t payload_size_per_fragment = DANP_SFP_MAX_DATA_PER_FRAGMENT;

    for (;;)
    {
        if (!sock || !data || len == 0)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid parameters");
            break;
        }

        if (sock->type == DANP_TYPE_DGRAM)
        {
            danp_log_message(DANP_LOG_ERROR, "SFP requires reliable STREAM sockets (DGRAM is unreliable)");
            ret = -EINVAL;
            break;
        }

        if (sock->state != DANP_SOCK_ESTABLISHED)
        {
            danp_log_message(DANP_LOG_ERROR, "Socket not connected");
            break;
        }

        /* Calculate number of fragments needed */
        uint16_t total_fragments = (len + payload_size_per_fragment - 1) / payload_size_per_fragment;

        if (total_fragments > DANP_SFP_MAX_FRAGMENTS)
        {
            danp_log_message(DANP_LOG_ERROR, "Message too large for SFP fragmentation");
            break;
        }

        danp_log_message(DANP_LOG_DEBUG, "Fragmenting %u bytes into %u fragments", len, total_fragments);

        /* Send fragments */
        while (offset < len)
        {
            danp_packet_t *pkt = danp_buffer_get();
            if (!pkt)
            {
                danp_log_message(DANP_LOG_ERROR, "Failed to allocate packet for fragment");
                ret = -ENOMEM;
                break;
            }

            /* Calculate chunk size for this fragment */
            uint16_t chunk_size = len - offset;
            if (chunk_size > payload_size_per_fragment)
            {
                chunk_size = payload_size_per_fragment;
            }

            /* Pack SFP header */
            bool is_begin = (fragment_id == 0);
            bool has_more = (offset + chunk_size < len);
            pkt->payload[0] = danp_sfp_pack_header(is_begin, has_more, fragment_id);

            /* Copy data chunk */
            memcpy(pkt->payload + 1, (uint8_t *)data + offset, chunk_size);
            pkt->length = chunk_size + 1; /* +1 for SFP header */

            /* Set packet header */
            pkt->header_raw = danp_pack_header(
                DANP_PRIORITY_NORMAL,
                sock->remote_node,
                sock->local_node,
                sock->remote_port,
                sock->local_port,
                DANP_FLAG_NONE);

            /* Route and transmit */
            if (danp_route_tx(pkt) < 0)
            {
                danp_log_message(DANP_LOG_ERROR, "Failed to route fragment %u", fragment_id);
                danp_buffer_free(pkt);
                ret = -EIO;
                break;
            }

            danp_buffer_free(pkt);

            bytes_sent += chunk_size;
            offset += chunk_size;
            fragment_id++;
        }

        if (bytes_sent == len)
        {
            ret = bytes_sent;
            danp_log_message(DANP_LOG_DEBUG, "Successfully sent %u bytes in %u fragments", bytes_sent, fragment_id);
        }

        break;
    }

    return ret;
}

/**
 * @brief Receive and reassemble fragmented message (SFP).
 * @param sock Pointer to the socket.
 * @param timeout_ms Timeout in milliseconds.
 * @return Pointer to first packet in chain (caller must free all), or NULL on timeout/error.
 */
danp_packet_t *danp_recv_sfp(danp_socket_t *sock, uint32_t timeout_ms)
{
    danp_packet_t *head = NULL;
    danp_packet_t *tail = NULL;
    bool has_more = true;
    uint8_t expected_fragment_id = 0;

    for (;;)
    {
        if (!sock)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid socket");
            break;
        }

        if (sock->type == DANP_TYPE_DGRAM)
        {
            danp_log_message(DANP_LOG_ERROR, "SFP requires reliable STREAM sockets (DGRAM is unreliable)");
            break;
        }

        /* Receive fragments until we get the last one */
        while (has_more)
        {
            danp_packet_t *pkt = danp_recv_packet(sock, timeout_ms);
            if (!pkt)
            {
                danp_log_message(DANP_LOG_WARN, "Timeout waiting for fragment");
                /* Free any partial chain */
                if (head)
                {
                    danp_buffer_free_chain(head);
                    head = NULL;
                }
                break;
            }

            /* Parse SFP header */
            bool is_begin = false;
            uint8_t fragment_id = 0;
            danp_sfp_unpack_header(pkt->payload[0], &is_begin, &has_more, &fragment_id);

            /* Validate fragment order */
            if (fragment_id != expected_fragment_id)
            {
                danp_log_message(DANP_LOG_ERROR, "Fragment out of order: expected %u, got %u", expected_fragment_id, fragment_id);
                danp_buffer_free(pkt);
                if (head)
                {
                    danp_buffer_free_chain(head);
                    head = NULL;
                }
                break;
            }

            /* Remove SFP header from payload (shift left by 1 byte) */
            memmove(pkt->payload, pkt->payload + 1, pkt->length - 1);
            pkt->length -= 1;

            /* Add to chain */
            if (!head)
            {
                head = pkt;
                tail = pkt;
            }
            else
            {
                tail->next = pkt;
                tail = pkt;
            }

            pkt->next = NULL;
            expected_fragment_id++;

            danp_log_message(DANP_LOG_VERBOSE, "Received fragment %u (more=%d)", fragment_id, has_more);
        }

        if (head)
        {
            danp_log_message(DANP_LOG_DEBUG, "Reassembled message with %u fragments", expected_fragment_id);
        }

        break;
    }

    return head;
}
