/* logMessage.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "danp/danp.h"
#include "osal/osal_time.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

danp_log_level_t LogSeverity = DANP_LOG_DEBUG;

/* Functions */

void danpLogMessageCallback(
    danp_log_level_t level,
    const char *funcName,
    const char *message,
    va_list args)
{
    if (level < LogSeverity)
    {
        return;
    }
    const char *levelStr = "UNKNOWN";
    const char *colorStart = "";
    const char *colorEnd = "\033[0m";
    uint32_t sysTimeMs = osal_get_tick_ms();
    switch (level)
    {
    case DANP_LOG_VERBOSE:
        levelStr = "Verbose";
        colorStart = "\033[37m"; // White
        break;
    case DANP_LOG_DEBUG:
        levelStr = "Debug";
        colorStart = "\033[36m"; // Cyan
        break;
    case DANP_LOG_INFO:
        levelStr = "Info";
        colorStart = "\033[32m"; // Green
        break;
    case DANP_LOG_WARN:
        levelStr = "Warn";
        colorStart = "\033[33m"; // Yellow
        break;
    case DANP_LOG_ERROR:
        levelStr = "Error";
        colorStart = "\033[31m"; // Red
        break;
    }
    printf("%s[%u][%s][%s] %s", colorStart, sysTimeMs, funcName, levelStr, colorEnd);
    vprintf(message, args);
    printf("\n");
    va_end(args);
}
