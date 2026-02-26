/* danp_accessors.c - Implementation of opaque structure accessors */

#include "danp/danp.h"
#include "danp_internal_types.h"

uint16_t danp_socket_get_remote_node(danp_socket_t *sock)
{
    return sock ? sock->remote_node : 0;
}

uint16_t danp_socket_get_remote_port(danp_socket_t *sock)
{
    return sock ? sock->remote_port : 0;
}

uint16_t danp_socket_get_local_port(danp_socket_t *sock)
{
    return sock ? sock->local_port : 0;
}

danp_socket_state_t danp_socket_get_state(danp_socket_t *sock)
{
    return sock ? sock->state : DANP_SOCK_CLOSED;
}

uint8_t *danp_packet_get_payload(danp_packet_t *pkt)
{
    return pkt ? pkt->payload : NULL;
}

uint16_t danp_packet_get_length(danp_packet_t *pkt)
{
    return pkt ? pkt->length : 0;
}

void danp_packet_set_length(danp_packet_t *pkt, uint16_t len)
{
    if (pkt)
    {
        pkt->length = len;
    }
}

uint32_t danp_packet_get_header(danp_packet_t *pkt)
{
    return pkt ? pkt->header_raw : 0;
}

void danp_packet_set_header(danp_packet_t *pkt, uint32_t header)
{
    if (pkt)
    {
        pkt->header_raw = header;
    }
}
