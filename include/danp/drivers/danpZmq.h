/* danpZmq.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_ZMQ_H
#define INC_DANP_ZMQ_H

/* Includes */

#include <stdint.h>
#include "danp/danp.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */

typedef struct danpZmqInterface_s
{
    danpInterface_t common;
    void *pubSock;
    void *subSock;
    uint32_t rxThreadId;
} danpZmqInterface_t;

/* External Declarations */

extern void danpZmqInit(
    danpZmqInterface_t *iface,
    const char *pubBindEndpoint,
    const char **subConnectEndpoints,
    int32_t subCount,
    uint16_t nodeId);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_ZMQ_H */
