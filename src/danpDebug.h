/* danpDebug.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_DEBUG_H
#define INC_DANP_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */


/* Configurations */


/* Definitions */

#define danpLogMessage(level, message, ...) danpLogMessageHandler(level, __func__, message, ##__VA_ARGS__)

/* Types */


/* External Declarations */

extern void danpLogMessageHandler(danpLogLevel_t level, const char *funcName, const char* message, ...);


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_DEBUG_H */