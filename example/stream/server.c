/* server.c - STREAM Server Example */

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

void taskStreamServer(void *arg)
{
    printf("[Server] Stream server starting...\n");

    // Create Listening Socket (STREAM)
    danpSocket_t *sockStream = danpSocket(DANP_TYPE_STREAM);
    int32_t bindResult = danpBind(sockStream, STREAM_PORT);
    assert(bindResult == 0);
    danpListen(sockStream, 5);

    printf("[Server] Listening stream on port %d\n", STREAM_PORT);

    while (1)
    {
        printf("[Server] Waiting for client...\n");
        danpSocket_t *client = danpAccept(sockStream, 5000); // 5s timeout
        if (client)
        {
            printf("[Server] Client Accepted! Node: %d\n", client->remoteNode);

            char buf[64];
            while (1)
            {
                // printf("[Server] Waiting for data from client...\n");
                // Use a short timeout to prevent excessive blocking, allowing internal DANP processing to run.
                int32_t len = danpRecv(client, buf, sizeof(buf) - 1, 5000); // 1000ms timeout
                if (len > 0)
                {
                    buf[len] = 0;
                    printf("[Server] Recv: %s\n", buf);

                    // Echo back
                    // Check danpSend result for errors/disconnects
                    if (danpSend(client, "ACK from Server", 15) < 0)
                    {
                        printf("[Server] danpSend failed. Closing client socket.\n");
                        break;
                    }
                }
                else if (len < 0)
                { // Error or Disconnect
                    printf("[Server] danpRecv failed (ret=%d). Closing client socket.\n", len);
                    break;
                }
                else
                { // len == 0 (Timeout or Graceful Close)
                    // Sleep briefly to yield CPU time to DANP internal processing/packet management tasks
                    osalDelayMs(1);
                }
            }

            danpClose(client); // Close the accepted socket
            printf("[Server] Client socket closed.\n");
        }

        osalDelayMs(100);
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
        .name = "StreamServer",
        .stackSize = 2048,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
    };
    osalThreadCreate(taskStreamServer, NULL, &threadAttr);

    while (1)
    {
        osalDelayMs(1000);
    }

    return 0;
}
