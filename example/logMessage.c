/* logMessage.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include "danp/danp.h"

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

danpLogLevel_t LogSeverity = DANP_LOG_DEBUG;

/* Functions */

void danpLogMessageCallback(danpLogLevel_t level, const char *funcName, const char* message, va_list args)
{
    if (level < LogSeverity) {
        return;
    }
    const char *levelStr = "UNKNOWN";
    const char *colorStart = "";
    const char *colorEnd = "\033[0m";
    uint32_t sysTimeMs = danpOsGetTickMs();
    switch(level) {
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
