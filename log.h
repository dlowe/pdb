#ifndef __LOG_H
#define __LOG_H

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_ERROR,
    LOG_NONE
} log_level;

int log_open(char *filename, log_level level);
int log_reopen(void);
void lo(log_level level, char *format, ...);
void log_close(void);

#endif
