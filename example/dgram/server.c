/* server.c - DGRAM Server Example */

#include "../exampleDefinitions.h"
#include "danp/danp.h"
#include "osal/osal.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern void danpZmqInit(
    const char *pubBindEndpoint,
    const char **subConnectEndpoints,
    int subCount,
    uint16_t nodeId);
extern void danpLogMessageCallback(
    danpLogLevel_t level,
    const char *funcName,
    const char *message,
    va_list args);

void taskEchoServer(void *arg)
{
    printf("[Server] Echo server starting...\n");

    // Create Socket (DGRAM)
    danpSocket_t *sockDgram = danpSocket(DANP_TYPE_DGRAM);
    int32_t bindResult = danpBind(sockDgram, DGRAM_PORT);
    assert(bindResult == 0);

    printf("[Server] Listening dgram on port %d\n", DGRAM_PORT);

    while (1)
    {
        // Handle DGRAM messages
        char dgramBuf[64];
        uint16_t dgramSrcNode, dgramSrcPort;
        // printf("[Server] Waiting for message...\n");
        int32_t dgramLen = danpRecvFrom(
            sockDgram,
            dgramBuf,
            sizeof(dgramBuf) - 1,
            &dgramSrcNode,
            &dgramSrcPort,
            5000); // 5s timeout
        if (dgramLen > 0)
        {
            dgramBuf[dgramLen] = 0;
            printf("[Server] Recv DGRAM: %s\n", dgramBuf);

            // Echo back to sender
            danpSendTo(sockDgram, dgramBuf, dgramLen, dgramSrcNode, dgramSrcPort);
        }
        else
        {
            // Sleep briefly to yield CPU time
            osalDelayMs(1);
        }
    }
}

int main(int argc, char **argv)
{
    const char *subEndpoints[] = {"tcp://localhost:5556"};
    danpZmqInit("tcp://*:5555", subEndpoints, 1, NODE_SERVER);
    danpConfig_t config = {
        .localNode = NODE_SERVER,
        .logFunction = danpLogMessageCallback,
    };
    danpInit(&config);
    osalThreadAttr_t threadAttr = {
        .name = "EchoServer",
        .stackSize = 2048,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
    };
    osalThreadCreate(taskEchoServer, NULL, &threadAttr);

    while (1)
    {
        osalDelayMs(1000);
    }

    return 0;
}
