/* danpLo.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_LO_H
#define INC_DANP_LO_H

/* Includes */

#include "danp/danp.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */

typedef struct danpLoInterface_s
{
    danpInterface_t common;
} danpLoInterface_t;

/* External Declarations */

extern int32_t danpLoInit (danpLoInterface_t *iface, uint16_t address);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_LO_H */
