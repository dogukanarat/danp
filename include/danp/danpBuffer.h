/* danpBuffer.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_BUFFER_H
#define INC_DANP_BUFFER_H

/* Includes */

#include "danp/danp.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */


/* External Declarations */

extern int32_t danpBufferInit(void);
extern danpPacket_t *danpBufferAllocate(void);
extern void danpBufferFree(danpPacket_t *pkt);
extern size_t danpBufferGetFreeCount(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_BUFFER_H */
