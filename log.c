/* system includes */
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* project includes */
#include "log.h"

typedef struct {
    char *filename;
    FILE *file;
    log_level level;
    pid_t pid;
} log_info;

static log_info l;

int log_open(char *filename, log_level level)
{
    l.level = level;
    if (l.level < LOG_NONE) {
        l.filename = strdup(filename);
        if (!l.filename) {
            log_close();
            return 0;
        }

        l.pid = getpid();
        l.file = fopen(l.filename, "a");
        if (!l.file) {
            log_close();
            return 0;
        }
    }
    return 1;
}

int log_reopen(void)
{
    if (l.level < LOG_NONE) {
        l.pid = getpid();

        fclose(l.file);
        l.file = fopen(l.filename, "a");
        if (!l.file) {
            log_close();
            return 0;
        }
    }
    return 1;
}

void lo(log_level level, char *format, ...)
{
    if (level < l.level) {
        return;
    }
    if (level >= LOG_NONE) {
        return;
    }

    va_list args;
    va_start(args, format);
    flockfile(l.file);
    fprintf(l.file, "%d %9ld %8d ", level, time(0), l.pid);
    /* Flawfinder: ignore format */
    vfprintf(l.file, format, args);
    fprintf(l.file, "\n");
    fflush(l.file);
    funlockfile(l.file);
    va_end(args);
}

void log_close(void)
{
    if (l.filename) {
        free(l.filename);
        l.filename = 0;
    }
    if (l.file) {
        fclose(l.file);
        l.file = 0;
    }
}
