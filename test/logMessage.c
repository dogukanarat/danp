#include "danp/danp.h"
#include "osal/osal.h"
#include <stdarg.h>
#include <stdio.h>

// Implementation of danpLogMessage for logging
void danpLogMessageWithFuncName(
    danpLogLevel_t level,
    const char *funcName,
    const char *message,
    ...)
{
    const char *levelStr = "UNKNOWN";
    const char *colorStart = "";
    const char *colorEnd = "\033[0m";
    uint32_t sysTimeMs = osalGetTickMs();
    switch (level)
    {
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
    va_list args;
    va_start(args, message);
    printf("%s[%u][%s][%s] %s", colorStart, sysTimeMs, funcName, levelStr, colorEnd);
    vprintf(message, args);
    printf("\n");
    va_end(args);
}
