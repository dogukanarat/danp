/* danpDebug.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_DEBUG_H
#define INC_DANP_DEBUG_H

/* Includes */


#ifdef __cplusplus
extern "C"
{
#endif

/* Configurations */


/* Definitions */

#define danp_log_message(level, message, ...)                                                        \
    danp_log_message_handler(level, __func__, message, ##__VA_ARGS__)

/* Types */


/* External Declarations */

extern void
danp_log_message_handler(danp_log_level_t level, const char *func_name, const char *message, ...);


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_DEBUG_H */
