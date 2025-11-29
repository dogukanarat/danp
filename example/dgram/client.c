/* client.c - DGRAM Client Example */

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

void taskClient(void *arg)
{
    osalDelayMs(1000); // Wait for server to boot
    printf("[Client] Starting DGRAM Client...\n");

    // Create Socket (DGRAM)
    danpSocket_t *sockDgram = danpSocket(DANP_TYPE_DGRAM);

    // Connect to Server (DGRAM) - sets default destination
    int32_t connectResult = danpConnect(sockDgram, NODE_SERVER, DGRAM_PORT);
    assert(connectResult == 0);

    for (int i = 0; i < 5; i++)
    {
        char msg[32];
        sprintf(msg, "Ping %d", i);
        printf("[Client] Sending DGRAM: %s\n", msg);

        if (danpSend(sockDgram, msg, strlen(msg)) < 0)
        {
            printf("[Client] DGRAM Send Failed!\n");
        }

        // Wait for reply
        char reply[64];
        int32_t replyLen = danpRecv(sockDgram, reply, sizeof(reply) - 1, 1000);

        if (replyLen > 0)
        {
            reply[replyLen] = '\0';
            printf("[Client] Got DGRAM Reply: %s\n", reply);
        }
        else
        {
            printf("[Client] DGRAM Receive Failed/Timeout (ret=%d)\n", replyLen);
        }

        osalDelayMs(1000);
    }

    danpClose(sockDgram); // Close the DGRAM socket
    printf("[Client] DGRAM socket closed.\n");
}

int main()
{
    const char *subEndpoints[] = {"tcp://localhost:5555"};
    danpZmqInit("tcp://*:5556", subEndpoints, 1, NODE_CLIENT);
    danpConfig_t config = {
        .localNode = NODE_CLIENT,
        .logFunction = danpLogMessageCallback,
    };
    danpInit(&config);
    osalThreadAttr_t threadAttr = {
        .name = "Client",
        .stackSize = 2048,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
    };
    osalThreadCreate(taskClient, NULL, &threadAttr);

    while (1)
    {
        osalDelayMs(1000);
    }

    return 0;
}
