#ifndef __LOG_H
#define __LOG_H

/**
 * @file log.h
 * @brief Logging API
 *
 * This is a very simple logging subsystem. It uses a global handle, so isn't
 * thread-safe, but this makes it easy to use from any code...
 */

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Logging levels.
 */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_ERROR,
    LOG_NONE
} log_level;

/**
 * Open log file.
 *
 * @param[in] filename the name of the log file to open.
 * @param[in] level the level of detail at which to log. If LOG_NONE is
 * specified, no logs are written.
 @ @return 1 on success, 0 on failure
 */
int log_open(char *filename, log_level level);

/**
 * Reopen log file. This should be called after forking.
 *
 * @return 1 on success, 0 on failure
 */
int log_reopen(void);

/**
 * Write to the log
 *
 * @param[in] level the severity of the log
 * @param[in] format printf-style format
 * @param[in] ... printf-style arguments
 */
void lo(log_level level, char *format, ...);

/**
 * Close the log.
 */
void log_close(void);

/**
 * Convert from a string to a log level.
 *
 * @param[in] string string description of the log level
 * @return a log_level
 */
log_level log_level_from_string(const char *string);

#endif
