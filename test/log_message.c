/**
 * @file logMessage.c
 * @brief Test logging implementation for DANP
 *
 * This file provides a color-coded console logging implementation for
 * DANP tests. It outputs formatted log messages with timestamps, function
 * names, and log levels to help with debugging during test execution.
 */

#include <stdarg.h>
#include <stdio.h>

#include "osal/osal_time.h"

#include "danp/danp_types.h"

/* ============================================================================
 * ANSI Color Codes
 * ============================================================================
 */

/* ANSI color codes for terminal output */
#define COLOR_RESET "\033[0m"   /* Reset to default */
#define COLOR_CYAN "\033[36m"   /* Cyan for debug */
#define COLOR_GREEN "\033[32m"  /* Green for info */
#define COLOR_YELLOW "\033[33m" /* Yellow for warning */
#define COLOR_RED "\033[31m"    /* Red for error */

/* ============================================================================
 * Public Functions
 * ============================================================================
 */

/**
 * @brief Log a message with function name, level, and timestamp
 *
 * This function formats and prints log messages with color coding based
 * on severity level. The format is:
 * [timestamp][function_name][level] message
 *
 * Example output:
 * [1234][danp_socket][Info] Socket created successfully
 *
 * @param level Log severity level (DEBUG, INFO, WARN, ERROR)
 * @param func_name Name of the function generating the log
 * @param message Format string for the log message
 * @param ... Variable arguments for format string
 */
void danp_log_message_with_func_name(
    danp_log_level_t level,
    const char *func_name,
    const char *message,
    va_list args)
{
    const char *level_str = "UNKNOWN";
    const char *color_start = "";
    const char *color_end = COLOR_RESET;

    /* Get system timestamp in milliseconds */
    uint32_t system_time_ms = osal_get_tick_ms();

    /* Select color and label based on log level */
    switch (level)
    {
    case DANP_LOG_VERBOSE:
        level_str = "Verbose";
        color_start = COLOR_CYAN;
        break;

    case DANP_LOG_DEBUG:
        level_str = "Debug";
        color_start = COLOR_CYAN;
        break;

    case DANP_LOG_INFO:
        level_str = "Info";
        color_start = COLOR_GREEN;
        break;

    case DANP_LOG_WARN:
        level_str = "Warn";
        color_start = COLOR_YELLOW;
        break;

    case DANP_LOG_ERROR:
        level_str = "Error";
        color_start = COLOR_RED;
        break;
    }

    /* Print the log header with color */
    printf(
        "%s[%u][%s][%s]%s ",
        color_start,
        system_time_ms,
        func_name,
        level_str,
        color_end);

    /* Print the actual log message with variable arguments */
    vprintf(message, args);

    /* Add newline */
    printf("\n");
}
