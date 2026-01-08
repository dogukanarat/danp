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

#define DANP_LOG_VER(message, ...)                                                               \
    danp_log_message(DANP_LOG_VERBOSE, message, ##__VA_ARGS__)
#define DANP_LOG_DBG(message, ...)                                                               \
    danp_log_message(DANP_LOG_DEBUG, message, ##__VA_ARGS__)
#define DANP_LOG_INF(message, ...)                                                               \
    danp_log_message(DANP_LOG_INFO, message, ##__VA_ARGS__)
#define DANP_LOG_WRN(message, ...)                                                               \
    danp_log_message(DANP_LOG_WARN, message, ##__VA_ARGS__)
#define DANP_LOG_ERR(message, ...)                                                               \
    danp_log_message(DANP_LOG_ERROR, message, ##__VA_ARGS__)

/* Types */


/* External Declarations */

extern void
danp_log_message_handler(danp_log_level_t level, const char *func_name, const char *message, ...);


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_DEBUG_H */
