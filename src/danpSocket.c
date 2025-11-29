/* danpSocket.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "danpDebug.h"
#include <stdio.h>

/* Imports */

extern danpConfig_t DanpConfig;

/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief List of active sockets. */
static danpSocket_t *SocketList = NULL;

/** @brief Next available ephemeral port. */
static uint16_t NextEphemeralPort = 1;

/* Functions */

/**
 * @brief Find a socket matching the given parameters.
 * @param localPort Local port number.
 * @param remoteNode Remote node address.
 * @param remotePort Remote port number.
 * @return Pointer to the matching socket, or NULL if not found.
 */
static danpSocket_t *danpFindSocket(uint16_t localPort, uint16_t remoteNode, uint16_t remotePort)
{
    danpSocket_t *cur = SocketList;
    danpSocket_t *ret = NULL;

    while (cur)
    {
        if (cur->localPort == localPort)
        {
            // Priority 1: Exact Match (Established/Connecting streams or Connected DGRAM)
            if (cur->remoteNode == remoteNode && cur->remotePort == remotePort &&
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
            }
        }
        cur = cur->next;
    }

    return ret;
}

/**
 * @brief Send a control packet.
 * @param sock Pointer to the socket.
 * @param flags Control flags to send.
 * @param seqNum Sequence number (for ACK packets).
 */
static void danpSendControl(danpSocket_t *sock, uint8_t flags, uint8_t seqNum)
{
    danpPacket_t *pkt = danpAllocPacket();

    for (;;)
    {
        if (!pkt)
        {
            danpLogMessage(DANP_LOG_ERROR, "Failed to allocate control packet");
            break;
        }

        pkt->headerRaw = danpPackHeader(
            0,
            sock->remoteNode,
            sock->localNode,
            sock->remotePort,
            sock->localPort,
            flags);

        if ((flags & DANP_FLAG_ACK) && sock->type == DANP_TYPE_STREAM)
        {
            pkt->payload[0] = seqNum;
            pkt->length = 1;
        }
        else
        {
            pkt->length = 0;
        }

        danpRouteTx(pkt);
        danpFreePacket(pkt);

        break;
    }
}

/**
 * @brief Create a new DANP socket.
 * @param type Type of the socket (DGRAM or STREAM).
 * @return Pointer to the created socket, or NULL on failure.
 */
danpSocket_t *danpSocket(danpSocketType_t type)
{
    static danpSocket_t staticSockets[20] = {0};
    danpSocket_t *s = NULL;
    osalQueueHandle_t rxQ;
    osalQueueHandle_t accQ;
    osalSemaphoreHandle_t sig;
    danpPacket_t *garbagePkt;
    danpSocket_t *garbageSock;
    osalMessageQueueAttr_t mqAttr = {
        .name = "danpSocketRx",
        .cbMem = NULL,
        .cbSize = 0,
        .mqMem = NULL,
        .mqSize = 0,
    };
    osalSemaphoreAttr_t semAttr = {
        .name = "danpSocketSig",
        .maxCount = 1,
        .initialCount = 0,
        .cbMem = NULL,
        .cbSize = 0,
    };

    for (;;)
    {
        for (int i = 0; i < 20; i++)
        {
            if (staticSockets[i].state == DANP_SOCK_CLOSED && staticSockets[i].localPort == 0)
            {
                s = &staticSockets[i];
                break;
            }
        }

        if (!s)
        {
            danpLogMessage(DANP_LOG_ERROR, "No free socket slots");
            break;
        }

        rxQ = s->rxQueue;
        accQ = s->acceptQueue;
        sig = s->signal;

        memset(s, 0, sizeof(danpSocket_t));

        // Restore handles
        s->rxQueue = rxQ;
        s->acceptQueue = accQ;
        s->signal = sig;

        s->type = type;
        s->state = DANP_SOCK_OPEN;
        s->txSeq = 0;
        s->rxExpectedSeq = 0;
        s->localNode = DanpConfig.localNode;

        if (!s->rxQueue)
        {
            s->rxQueue = osalMessageQueueCreate(10, sizeof(danpPacket_t *), &mqAttr);
        }
        if (!s->acceptQueue)
        {
            s->acceptQueue = osalMessageQueueCreate(5, sizeof(danpSocket_t *), &mqAttr);
        }
        if (!s->signal)
        {
            s->signal = osalSemaphoreCreate(&semAttr);
        }

        // Flush queues to ensure clean state
        while (0 == osalMessageQueueReceive(s->rxQueue, &garbagePkt, 0))
        {
            danpFreePacket(garbagePkt);
        }
        while (0 == osalMessageQueueReceive(s->acceptQueue, &garbageSock, 0))
        {
            // Just drain the queue, the sockets themselves should have been closed/handled elsewhere
        }

        s->next = SocketList;
        SocketList = s;

        break;
    }

    return s;
}

/**
 * @brief Bind a socket to a local port.
 * @param sock Pointer to the socket.
 * @param port Local port to bind to.
 * @return 0 on success, negative on error.
 */
int32_t danpBind(danpSocket_t *sock, uint16_t port)
{
    int32_t ret = 0;

    for (;;)
    {
        if (port == 0)
        {
            port = NextEphemeralPort++;
            if (NextEphemeralPort >= DANP_MAX_PORTS)
            {
                NextEphemeralPort = 1;
            }
        }
        if (port >= DANP_MAX_PORTS)
        {
            ret = -1;
            break;
        }
        sock->localPort = port;
        danpLogMessage(DANP_LOG_INFO, "Socket bound to port %u", port);

        break;
    }

    return ret;
}

/**
 * @brief Listen for incoming connections on a socket.
 * @param sock Pointer to the socket.
 * @param backlog Maximum number of pending connections (currently unused).
 * @return 0 on success, negative on error.
 */
int danpListen(danpSocket_t *sock, int backlog)
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
int32_t danpClose(danpSocket_t *sock)
{
    // Only send RST for STREAM sockets or connected DGRAM sockets that are actually in a state where RST makes sense.
    // For DGRAM, we generally don't send RST on close unless we want to signal the peer to stop sending.
    // However, standard UDP doesn't do this. Let's restrict RST to STREAM.
    if (sock->type == DANP_TYPE_STREAM &&
        (sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_SENT ||
         sock->state == DANP_SOCK_SYN_RECEIVED))
    {
        danpSendControl(sock, DANP_FLAG_RST, 0);
    }

    // Clean up state for slot recycling
    sock->state = DANP_SOCK_CLOSED;
    sock->localPort = 0;

    // Note: Queue and semaphore are kept alive for quick reuse of the slot.

    return 0;
}


/**
 * @brief Connect a socket to a remote node and port.
 * @param sock Pointer to the socket.
 * @param node Remote node address.
 * @param port Remote port number.
 * @return 0 on success, negative on error.
 */
int32_t danpConnect(danpSocket_t *sock, uint16_t node, uint16_t port)
{
    int32_t ret = 0;

    for (;;)
    {
        if (sock->localPort == 0)
        {
            danpBind(sock, 0);
        }
        sock->remoteNode = node;
        sock->remotePort = port;

        if (sock->type == DANP_TYPE_DGRAM)
        {
            // For DGRAM, "Connect" just sets the default destination.
            // We use ESTABLISHED to indicate that the socket has a default peer,
            // allowing danpFindSocket to match incoming packets from this peer specifically.
            sock->state = DANP_SOCK_ESTABLISHED;
            break;
        }

        danpLogMessage(
            DANP_LOG_INFO,
            "Connecting to Node %d Port %d from Local Port %d",
            node,
            port,
            sock->localPort);
        sock->state = DANP_SOCK_SYN_SENT;
        danpSendControl(sock, DANP_FLAG_SYN, 0);

        if (0 == osalSemaphoreTake(sock->signal, DANP_ACK_TIMEOUT_MS))
        {
            danpLogMessage(DANP_LOG_INFO, "Connection Established");
            break;
        }

        sock->state = DANP_SOCK_OPEN; // Reset state on timeout
        danpLogMessage(DANP_LOG_WARN, "Connect Timeout");

        ret = -1;

        break;
    }

    return ret;
}

/**
 * @brief Accept a new connection on a listening socket.
 * @param serverSock Pointer to the listening socket.
 * @param timeoutMs Timeout in milliseconds.
 * @return Pointer to the new connected socket, or NULL on timeout/error.
 */
danpSocket_t *danpAccept(danpSocket_t *serverSock, uint32_t timeoutMs)
{
    danpSocket_t *client = NULL;

    for (;;)
    {
        if (0 == osalMessageQueueReceive(serverSock->acceptQueue, &client, timeoutMs))
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
int32_t danpSend(danpSocket_t *sock, void *data, uint16_t len)
{
    int32_t ret = 0;
    int32_t retries = 0;
    bool ackReceived = false;
    danpPacket_t *pkt = NULL;

    for (;;)
    {
        if (len > DANP_MAX_PACKET_SIZE - 1)
        {
            ret = -1;
            break;
        }

        // DGRAM (Unreliable)
        if (sock->type == DANP_TYPE_DGRAM)
        {
            danpPacket_t *pkt = danpAllocPacket();
            if (!pkt)
            {
                ret = -1;
                break;
            }
            pkt->headerRaw = danpPackHeader(
                0,
                sock->remoteNode,
                sock->localNode,
                sock->remotePort,
                sock->localPort,
                DANP_FLAG_NONE);
            memcpy(pkt->payload, data, len);
            pkt->length = len;
            danpRouteTx(pkt);
            danpFreePacket(pkt);
            ret = len;
            break;
        }

        // STREAM (Stop-and-Wait)
        while (retries < DANP_RETRY_LIMIT && !ackReceived)
        {
            pkt = danpAllocPacket();
            if (!pkt)
            {
                osalDelayMs(10);
                continue;
            }
            pkt->headerRaw = danpPackHeader(
                0,
                sock->remoteNode,
                sock->localNode,
                sock->remotePort,
                sock->localPort,
                DANP_FLAG_NONE);
            pkt->payload[0] = sock->txSeq;
            memcpy(pkt->payload + 1, data, len);
            pkt->length = len + 1;
            danpRouteTx(pkt);
            danpFreePacket(pkt);

            if (0 == osalSemaphoreTake(sock->signal, DANP_ACK_TIMEOUT_MS))
            {
                ackReceived = true;
            }
            retries++;
        }

        if (ackReceived)
        {
            sock->txSeq++;
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
 * @param maxLen Maximum length of the buffer.
 * @param timeoutMs Timeout in milliseconds.
 * @return Number of bytes received, or negative on error/timeout.
 */
int32_t danpRecv(danpSocket_t *sock, void *buffer, uint16_t maxLen, uint32_t timeoutMs)
{
    int32_t ret = 0;
    danpPacket_t *pkt = NULL;
    int32_t copyLen = 0;

    if (0 == osalMessageQueueReceive(sock->rxQueue, &pkt, timeoutMs))
    {
        if (sock->type == DANP_TYPE_DGRAM)
        {
            copyLen = (pkt->length > maxLen) ? maxLen : pkt->length;
            memcpy(buffer, pkt->payload, copyLen);
        }
        else
        {
            if (pkt->length > 0)
            {
                copyLen = (pkt->length - 1 > maxLen) ? maxLen : (pkt->length - 1);
                memcpy(buffer, pkt->payload + 1, copyLen);
            }
        }
        danpFreePacket(pkt);
        ret = copyLen;
    }
    return ret;
}

/* ========================================================================
 * INPUT HANDLER
 * ======================================================================== */
/**
 * @brief Handle incoming packets for sockets.
 * @param pkt Pointer to the received packet.
 */
void danpSocketInputHandler(danpPacket_t *pkt)
{
    uint16_t dst = 0;
    uint16_t src = 0;
    uint8_t dstPort = 0;
    uint8_t srcPort = 0;
    uint8_t flags = 0;
    uint8_t ackedSeq = 0;
    uint8_t seq = 0;
    // bool isConnected = false;
    danpSocket_t *child = NULL;
    danpPacket_t *garbage;

    for (;;)
    {
        danpUnpackHeader(pkt->headerRaw, &dst, &src, &dstPort, &srcPort, &flags);
        danpSocket_t *sock = danpFindSocket(dstPort, src, srcPort);

        if (flags == DANP_FLAG_RST)
        {
            if (sock)
            {
                // If remotePort is 0, it is a listener/unbound-source socket.
                // isConnected = (sock->remotePort != 0);

                if (sock->type == DANP_TYPE_STREAM)
                {
                    danpLogMessage(
                        DANP_LOG_INFO,
                        "Received RST from peer. Closing socket to Port %u.",
                        dstPort);
                    sock->state = DANP_SOCK_CLOSED;
                    sock->localPort = 0;
                }
                else
                {
                    // For DGRAM, RST is advisory or ignored. We don't close the socket because DGRAM is connectionless.
                    danpLogMessage(DANP_LOG_WARN, "Ignored RST on DGRAM socket Port %u", dstPort);
                }
            }
            danpFreePacket(pkt);
            break;
        }

        if (!sock)
        {
            danpLogMessage(DANP_LOG_WARN, "No socket found for Port %u", dstPort);
            danpFreePacket(pkt);
            break;
        }

        if ((sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_RECEIVED) &&
            (flags & DANP_FLAG_SYN))
        {
            danpLogMessage(DANP_LOG_WARN, "Received SYN on active socket. Peer restart/resync. State reset.");

            if (sock->type == DANP_TYPE_STREAM)
            {
                sock->txSeq = 0;
                sock->rxExpectedSeq = 0;

                while (0 == osalMessageQueueReceive(sock->rxQueue, &garbage, 0))
                {
                    danpFreePacket(garbage); // Clear out old data
                }
            }

            danpSendControl(sock, DANP_FLAG_ACK | DANP_FLAG_SYN, 0);
            sock->state = DANP_SOCK_SYN_RECEIVED;

            danpFreePacket(pkt);
            return;
        }

        if (sock->state == DANP_SOCK_LISTENING && (flags & DANP_FLAG_SYN))
        {
            danpLogMessage(DANP_LOG_INFO, "Received SYN from Node %d Port %d", src, srcPort);

            child = danpSocket(sock->type);
            if (!child)
            {
                danpFreePacket(pkt);
                break;
            }

            child->localNode = DanpConfig.localNode;
            child->localPort = dstPort;
            child->remoteNode = src;
            child->remotePort = srcPort;

            child->state = DANP_SOCK_SYN_RECEIVED; // Set state and wait for final ACK

            if (0 != osalMessageQueueSend(sock->acceptQueue, &child, 0))
            {
                child->state = DANP_SOCK_CLOSED;
                child->localPort = 0;
                danpFreePacket(pkt);
                break;
            }

            danpSendControl(child, DANP_FLAG_ACK | DANP_FLAG_SYN, 0);
            danpFreePacket(pkt);
            break;
        }

        if (sock->state == DANP_SOCK_SYN_SENT && (flags & DANP_FLAG_ACK))
        {
            sock->state = DANP_SOCK_ESTABLISHED;
            danpSendControl(sock, DANP_FLAG_ACK, 0); // Send final ACK
            osalSemaphoreGive(sock->signal);
            danpFreePacket(pkt);
            break;
        }

        if (sock->state == DANP_SOCK_SYN_RECEIVED && (flags & DANP_FLAG_ACK) &&
            !(flags & DANP_FLAG_SYN))
        {
            // Final ACK received from client/resync, connection is fully active
            sock->state = DANP_SOCK_ESTABLISHED;
            danpFreePacket(pkt);
            break;
        }


        // --- 2. DATA HANDLING ---
        if ((flags & DANP_FLAG_ACK) && !(flags & DANP_FLAG_SYN) && pkt->length == 1)
        {
            if (sock->type == DANP_TYPE_STREAM)
            {
                ackedSeq = pkt->payload[0];
                if (ackedSeq == sock->txSeq)
                {
                    osalSemaphoreGive(sock->signal);
                }
            }
            danpFreePacket(pkt);
            break;
        }

        if ((sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_RECEIVED ||
             (sock->type == DANP_TYPE_DGRAM && sock->state == DANP_SOCK_OPEN)) &&
            pkt->length > 0)
        {
            if (sock->type == DANP_TYPE_DGRAM)
            {
                osalMessageQueueSend(sock->rxQueue, &pkt, 0);
                break;
            }
            else if (sock->type == DANP_TYPE_STREAM)
            {
                seq = pkt->payload[0];

                if (sock->state == DANP_SOCK_SYN_RECEIVED)
                {
                    sock->state = DANP_SOCK_ESTABLISHED;
                    danpLogMessage(DANP_LOG_INFO, "Implicitly established connection via Data packet");
                }

                if (seq == sock->rxExpectedSeq)
                {
                    sock->rxExpectedSeq++;
                    danpSendControl(sock, DANP_FLAG_ACK, seq);
                    osalMessageQueueSend(sock->rxQueue, &pkt, 0);
                }
                else
                {
                    danpSendControl(sock, DANP_FLAG_ACK, seq);
                    danpFreePacket(pkt);
                }
            }

            break;
        }

        danpFreePacket(pkt);

        break;
    }
}

/**
 * @brief Send data to a specific destination (for DGRAM sockets).
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @param dstNode Destination node address.
 * @param dstPort Destination port number.
 * @return Number of bytes sent, or negative on error.
 */
int32_t danpSendTo(danpSocket_t *sock, void *data, uint16_t len, uint16_t dstNode, uint16_t dstPort)
{
    int32_t ret = 0;
    danpPacket_t *pkt;

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
        pkt = danpAllocPacket();
        if (!pkt)
        {
            ret = -1;
            break;
        }

        pkt->headerRaw = danpPackHeader(0, dstNode, sock->localNode, dstPort, sock->localPort, DANP_FLAG_NONE);
        memcpy(pkt->payload, data, len);
        pkt->length = len;
        danpRouteTx(pkt);
        danpFreePacket(pkt);

        ret = len;

        break;
    }

    return ret;
}

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
    uint32_t timeoutMs)
{
    int32_t ret = -1;
    uint16_t dst = 0;
    uint16_t src = 0;
    uint8_t dPort = 0;
    uint8_t sPort = 0;
    uint8_t flags = 0;
    danpPacket_t *pkt;
    int copyLen;

    for (;;)
    {
        if (sock->type != DANP_TYPE_DGRAM)
        {
            break;
        }

        if (0 == osalMessageQueueReceive(sock->rxQueue, &pkt, timeoutMs))
        {
            copyLen = (pkt->length > maxLen) ? maxLen : pkt->length;
            memcpy(buffer, pkt->payload, copyLen);

            danpUnpackHeader(pkt->headerRaw, &dst, &src, &dPort, &sPort, &flags);

            if (srcNode)
            {
                *srcNode = src;
            }
            if (srcPort)
            {
                *srcPort = sPort;
            }

            danpFreePacket(pkt);
            ret = copyLen;
        }

        break;
    }

    return ret;
}
