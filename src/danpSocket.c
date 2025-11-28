/* danpSocket.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <stdio.h> 
#include "danp/danp.h"
#include "danpDebug.h"

/* Imports */

extern danpConfig_t GDanpConfig;

/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief List of active sockets. */
static danpSocket_t* SocketList = NULL;

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
static danpSocket_t* danpFindSocket(uint16_t localPort, uint16_t remoteNode, uint16_t remotePort) {
    danpSocket_t* cur = SocketList;
    danpSocket_t* wildcardMatch = NULL;

    while(cur) {
        if(cur->localPort == localPort) {
            // Priority 1: Exact Match (Established/Connecting streams or Connected DGRAM)
            if (cur->remoteNode == remoteNode && cur->remotePort == remotePort && 
               (cur->state == DANP_SOCK_ESTABLISHED || cur->state == DANP_SOCK_SYN_SENT || cur->state == DANP_SOCK_SYN_RECEIVED)) {
                return cur; 
            }
            
            // Priority 2: Wildcard Match (Listening stream or Open/Bound DGRAM socket)
            if (cur->state == DANP_SOCK_LISTENING || 
               (cur->type == DANP_TYPE_DGRAM && cur->state == DANP_SOCK_OPEN)) {
                wildcardMatch = cur;
            }
        }
        cur = cur->next;
    }
    
    return wildcardMatch;
}

/**
 * @brief Send a control packet.
 * @param sock Pointer to the socket.
 * @param flags Control flags to send.
 * @param seqNum Sequence number (for ACK packets).
 */
static void danpSendControl(danpSocket_t* sock, uint8_t flags, uint8_t seqNum) {
    danpPacket_t* pkt = danpAllocPacket();
    if(!pkt) {
        danpLogMessage(DANP_LOG_ERROR, "Failed to allocate control packet");
        return;
    }

    pkt->headerRaw = danpPackHeader(0, sock->remoteNode, sock->localNode, sock->remotePort, sock->localPort, flags);
    
    if ((flags & DANP_FLAG_ACK) && sock->type == DANP_TYPE_STREAM) {
        pkt->payload[0] = seqNum;
        pkt->length = 1; 
    } else {
        pkt->length = 0;
    }
    
    danpRouteTx(pkt);
    danpFreePacket(pkt);
}

/**
 * @brief Create a new DANP socket.
 * @param type Type of the socket (DGRAM or STREAM).
 * @return Pointer to the created socket, or NULL on failure.
 */
danpSocket_t* danpSocket(danpSocketType_t type) {
    static danpSocket_t staticSockets[20];
    danpSocket_t* s = NULL;
    for(int i=0; i<20; i++) {
        if(staticSockets[i].state == DANP_SOCK_CLOSED && staticSockets[i].localPort == 0) {
            s = &staticSockets[i];
            break;
        }
    }
    
    if (!s) {
        danpLogMessage(DANP_LOG_ERROR, "No free socket slots");
        return NULL;
    }

    danpOsQueueHandle_t rxQ = s->rxQueue;
    danpOsQueueHandle_t accQ = s->acceptQueue;
    danpOsSemaphoreHandle_t sig = s->signal;

    memset(s, 0, sizeof(danpSocket_t));
    
    // Restore handles
    s->rxQueue = rxQ;
    s->acceptQueue = accQ;
    s->signal = sig;

    s->type = type;
    s->state = DANP_SOCK_OPEN;
    s->txSeq = 0;
    s->rxExpectedSeq = 0;
    s->localNode = GDanpConfig.localNode;
    
    if (!s->rxQueue) s->rxQueue = danpOsQueueCreate(10, sizeof(danpPacket_t*));
    if (!s->acceptQueue) s->acceptQueue = danpOsQueueCreate(5, sizeof(danpSocket_t*));
    if (!s->signal) s->signal = danpOsSemCreate();

    // Flush queues to ensure clean state
    danpPacket_t* garbagePkt;
    while(danpOsQueueReceive(s->rxQueue, &garbagePkt, 0)) {
        danpFreePacket(garbagePkt);
    }
    danpSocket_t* garbageSock;
    while(danpOsQueueReceive(s->acceptQueue, &garbageSock, 0)) {
        // Just drain the queue, the sockets themselves should have been closed/handled elsewhere
    }
    
    s->next = SocketList;
    SocketList = s;
    return s;
}

/**
 * @brief Bind a socket to a local port.
 * @param sock Pointer to the socket.
 * @param port Local port to bind to.
 * @return 0 on success, negative on error.
 */
int32_t danpBind(danpSocket_t* sock, uint16_t port) {
    if (port == 0) {
        port = NextEphemeralPort++;
        if (NextEphemeralPort >= DANP_MAX_PORTS) NextEphemeralPort = 1;
    }
    if (port >= DANP_MAX_PORTS) return -1;
    sock->localPort = port;
    danpLogMessage(DANP_LOG_INFO, "Socket bound to port %u", port);
    return 0;
}

/**
 * @brief Listen for incoming connections on a socket.
 * @param sock Pointer to the socket.
 * @param backlog Maximum number of pending connections (currently unused).
 * @return 0 on success, negative on error.
 */
int danpListen(danpSocket_t* sock, int backlog) {
    sock->state = DANP_SOCK_LISTENING;
    return 0;
}

/**
 * @brief Close a socket and release resources.
 * @param sock Pointer to the socket to close.
 * @return 0 on success, negative on error.
 */
int32_t danpClose(danpSocket_t* sock) {
    // Only send RST for STREAM sockets or connected DGRAM sockets that are actually in a state where RST makes sense.
    // For DGRAM, we generally don't send RST on close unless we want to signal the peer to stop sending.
    // However, standard UDP doesn't do this. Let's restrict RST to STREAM.
    if (sock->type == DANP_TYPE_STREAM && (sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_SENT || sock->state == DANP_SOCK_SYN_RECEIVED)) {
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
int32_t danpConnect(danpSocket_t* sock, uint16_t node, uint16_t port) {
    if (sock->localPort == 0) {
        danpBind(sock, 0);
    }
    sock->remoteNode = node;
    sock->remotePort = port;
    
    if (sock->type == DANP_TYPE_DGRAM) {
        // For DGRAM, "Connect" just sets the default destination. 
        // We use ESTABLISHED to indicate that the socket has a default peer, 
        // allowing danpFindSocket to match incoming packets from this peer specifically.
        sock->state = DANP_SOCK_ESTABLISHED;
        return 0;
    }

    danpLogMessage(DANP_LOG_INFO, "Connecting to Node %d Port %d from Local Port %d", node, port, sock->localPort);
    sock->state = DANP_SOCK_SYN_SENT;
    danpSendControl(sock, DANP_FLAG_SYN, 0);

    if (danpOsSemTake(sock->signal, DANP_ACK_TIMEOUT_MS)) {
        danpLogMessage(DANP_LOG_INFO, "Connection Established");
        return 0;
    }
    
    sock->state = DANP_SOCK_OPEN; // Reset state on timeout
    danpLogMessage(DANP_LOG_WARN, "Connect Timeout");
    return -1;
}

/**
 * @brief Accept a new connection on a listening socket.
 * @param serverSock Pointer to the listening socket.
 * @param timeoutMs Timeout in milliseconds.
 * @return Pointer to the new connected socket, or NULL on timeout/error.
 */
danpSocket_t* danpAccept(danpSocket_t* serverSock, uint32_t timeoutMs) {
    danpSocket_t* client = NULL;
    if (danpOsQueueReceive(serverSock->acceptQueue, &client, timeoutMs)) {
        return client;
    }
    return NULL;
}

/**
 * @brief Send data over a connected socket.
 * @param sock Pointer to the socket.
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 * @return Number of bytes sent, or negative on error.
 */
int32_t danpSend(danpSocket_t* sock, void* data, uint16_t len) {
    if (len > DANP_MAX_PACKET_SIZE - 1) return -1;

    if (sock->type == DANP_TYPE_DGRAM) {
        danpPacket_t* pkt = danpAllocPacket();
        if(!pkt) return -1;
        pkt->headerRaw = danpPackHeader(0, sock->remoteNode, sock->localNode, sock->remotePort, sock->localPort, DANP_FLAG_NONE);
        memcpy(pkt->payload, data, len);
        pkt->length = len;
        danpRouteTx(pkt);
        danpFreePacket(pkt);
        return len;
    }

    // STREAM (Stop-and-Wait)
    int retries = 0;
    bool ackReceived = false;

    while (retries < DANP_RETRY_LIMIT && !ackReceived) {
        danpPacket_t* pkt = danpAllocPacket();
        if(!pkt) {
            danpOsDelayMs(10); 
            continue;
        }
        pkt->headerRaw = danpPackHeader(0, sock->remoteNode, sock->localNode, sock->remotePort, sock->localPort, DANP_FLAG_NONE);
        pkt->payload[0] = sock->txSeq;
        memcpy(pkt->payload + 1, data, len);
        pkt->length = len + 1;
        danpRouteTx(pkt);
        danpFreePacket(pkt); 
        
        if (danpOsSemTake(sock->signal, DANP_ACK_TIMEOUT_MS)) {
            ackReceived = true;
        }
        retries++;
    }

    if (ackReceived) {
        sock->txSeq++; 
        return len;
    } else {
        return -1; 
    }
}

/**
 * @brief Receive data from a connected socket.
 * @param sock Pointer to the socket.
 * @param buffer Pointer to the buffer to store received data.
 * @param maxLen Maximum length of the buffer.
 * @param timeoutMs Timeout in milliseconds.
 * @return Number of bytes received, or negative on error/timeout.
 */
int32_t danpRecv(danpSocket_t* sock, void* buffer, uint16_t maxLen, uint32_t timeoutMs) {
    danpPacket_t* pkt;
    if (danpOsQueueReceive(sock->rxQueue, &pkt, timeoutMs)) {
        int copyLen = 0;
        if (sock->type == DANP_TYPE_DGRAM) {
            copyLen = (pkt->length > maxLen) ? maxLen : pkt->length;
            memcpy(buffer, pkt->payload, copyLen);
        } else {
            if (pkt->length > 0) {
                 copyLen = (pkt->length - 1 > maxLen) ? maxLen : (pkt->length - 1);
                 memcpy(buffer, pkt->payload + 1, copyLen);
            }
        }
        danpFreePacket(pkt);
        return copyLen;
    }
    return 0; 
}

/* ========================================================================
 * INPUT HANDLER
 * ======================================================================== */
/**
 * @brief Handle incoming packets for sockets.
 * @param pkt Pointer to the received packet.
 */
void danpSocketInputHandler(danpPacket_t* pkt) {
    uint16_t dst, src;
    uint8_t dstPort, srcPort, flags;
    danpUnpackHeader(pkt->headerRaw, &dst, &src, &dstPort, &srcPort, &flags);
    
    danpSocket_t* sock = danpFindSocket(dstPort, src, srcPort);
    
    if (flags == DANP_FLAG_RST) { 
        if(sock) {
            // If remotePort is 0, it is a listener/unbound-source socket.
            bool isConnected = (sock->remotePort != 0);
            
            if (sock->type == DANP_TYPE_STREAM) {
                danpLogMessage(DANP_LOG_INFO, "Received RST from peer. Closing socket to Port %u.", dstPort);
                sock->state = DANP_SOCK_CLOSED;
                sock->localPort = 0;
            } else {
                // For DGRAM, RST is advisory or ignored. We don't close the socket because DGRAM is connectionless.
                danpLogMessage(DANP_LOG_WARN, "Ignored RST on DGRAM socket Port %u", dstPort);
            }
        }
        danpFreePacket(pkt);
        return;
    }
    
    if (!sock) {
        danpLogMessage(DANP_LOG_WARN, "No socket found for Port %u", dstPort);
        danpFreePacket(pkt);
        return;
    }
    
    if ((sock->state == DANP_SOCK_ESTABLISHED || sock->state == DANP_SOCK_SYN_RECEIVED) && (flags & DANP_FLAG_SYN)) {
        danpLogMessage(DANP_LOG_WARN, "Received SYN on active socket. Peer restart/resync. State reset.");
        
        if (sock->type == DANP_TYPE_STREAM) {
            sock->txSeq = 0;
            sock->rxExpectedSeq = 0;
            danpPacket_t* garbage;
            while(danpOsQueueReceive(sock->rxQueue, &garbage, 0)) {
                danpFreePacket(garbage); // Clear out old data
            }
        }

        danpSendControl(sock, DANP_FLAG_ACK | DANP_FLAG_SYN, 0);
        sock->state = DANP_SOCK_SYN_RECEIVED; 
        
        danpFreePacket(pkt);
        return;
    }

    if (sock->state == DANP_SOCK_LISTENING && (flags & DANP_FLAG_SYN)) {
        danpLogMessage(DANP_LOG_INFO, "Received SYN from Node %d Port %d", src, srcPort);
        
        danpSocket_t* child = danpSocket(sock->type);
        if(!child) {
            danpFreePacket(pkt);
            return;
        }

        child->localNode = GDanpConfig.localNode;
        child->localPort = dstPort;   
        child->remoteNode = src;
        child->remotePort = srcPort;  
        
        child->state = DANP_SOCK_SYN_RECEIVED; // Set state and wait for final ACK
        
        if (!danpOsQueueSend(sock->acceptQueue, &child, 0)) {
            child->state = DANP_SOCK_CLOSED; 
            child->localPort = 0;
            danpFreePacket(pkt);
            return;
        }

        danpSendControl(child, DANP_FLAG_ACK | DANP_FLAG_SYN, 0);
        danpFreePacket(pkt);
        return;
    }

    if (sock->state == DANP_SOCK_SYN_SENT && (flags & DANP_FLAG_ACK)) {
        sock->state = DANP_SOCK_ESTABLISHED;
        danpSendControl(sock, DANP_FLAG_ACK, 0); // Send final ACK
        danpOsSemGive(sock->signal); 
        danpFreePacket(pkt);
        return;
    }
    
    if (sock->state == DANP_SOCK_SYN_RECEIVED && (flags & DANP_FLAG_ACK) && !(flags & DANP_FLAG_SYN)) {
        // Final ACK received from client/resync, connection is fully active
        sock->state = DANP_SOCK_ESTABLISHED;
        danpFreePacket(pkt);
        return;
    }


    // --- 2. DATA HANDLING ---
    if ((flags & DANP_FLAG_ACK) && !(flags & DANP_FLAG_SYN) && pkt->length == 1) {
        if (sock->type == DANP_TYPE_STREAM) {
            uint8_t ackedSeq = pkt->payload[0];
            if (ackedSeq == sock->txSeq) {
                danpOsSemGive(sock->signal); 
            }
        }
        danpFreePacket(pkt);
        return;
    }
    
    // [FIX] Expanded condition to include DANP_SOCK_SYN_RECEIVED
    if ((sock->state == DANP_SOCK_ESTABLISHED || 
         sock->state == DANP_SOCK_SYN_RECEIVED || 
         (sock->type == DANP_TYPE_DGRAM && sock->state == DANP_SOCK_OPEN)) && pkt->length > 0) {
        
        if (sock->type == DANP_TYPE_DGRAM) {
             danpOsQueueSend(sock->rxQueue, &pkt, 0);
             return;
        } 
        else if (sock->type == DANP_TYPE_STREAM) {
            uint8_t seq = pkt->payload[0];
            
            // [FIX] Implicitly transition to ESTABLISHED on first data packet
            if (sock->state == DANP_SOCK_SYN_RECEIVED) {
                sock->state = DANP_SOCK_ESTABLISHED;
                danpLogMessage(DANP_LOG_INFO, "Implicitly established connection via Data packet");
            }

            if (seq == sock->rxExpectedSeq) {
                sock->rxExpectedSeq++;
                danpSendControl(sock, DANP_FLAG_ACK, seq);
                danpOsQueueSend(sock->rxQueue, &pkt, 0);
            } else {
                danpSendControl(sock, DANP_FLAG_ACK, seq);
                danpFreePacket(pkt);
            }
        }
        return;
    }

    danpFreePacket(pkt);
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
int32_t danpSendTo(danpSocket_t* sock, void* data, uint16_t len, uint16_t dstNode, uint16_t dstPort) {
    if (sock->type != DANP_TYPE_DGRAM) return -1;
    if (len > DANP_MAX_PACKET_SIZE - 1) return -1;
    danpPacket_t* pkt = danpAllocPacket();
    if (!pkt) return -1;
    
    pkt->headerRaw = danpPackHeader(0, dstNode, sock->localNode, dstPort, sock->localPort, DANP_FLAG_NONE);
    
    memcpy(pkt->payload, data, len);
    pkt->length = len;
    danpRouteTx(pkt);
    danpFreePacket(pkt);
    return len;
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
int32_t danpRecvFrom(danpSocket_t* sock, void* buffer, uint16_t maxLen, uint16_t* srcNode, uint16_t* srcPort, uint32_t timeoutMs) {
    if (sock->type != DANP_TYPE_DGRAM) return -1;
    danpPacket_t* pkt;
    // Use timeoutMs in danpOsQueueReceive
    if (danpOsQueueReceive(sock->rxQueue, &pkt, timeoutMs)) {
        int copyLen = (pkt->length > maxLen) ? maxLen : pkt->length;
        memcpy(buffer, pkt->payload, copyLen);
        
        uint16_t dst, src;
        uint8_t dPort, sPort, flags;
        danpUnpackHeader(pkt->headerRaw, &dst, &src, &dPort, &sPort, &flags);
        
        if (srcNode) *srcNode = src;
        if (srcPort) *srcPort = sPort; 
        
        danpFreePacket(pkt);
        return copyLen;
    }
    return 0;
}