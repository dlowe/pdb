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

#include "component.h"

DECLARE_COMPONENT(log);

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

#endif
