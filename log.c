/* system includes */
#include <sys/time.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* 3rd party includes */
#include "confuse.h"

/* project includes */
#include "log.h"

#define CFG_LOG_FILE "log_file"
#define CFG_LOG_FILE_DEFAULT "pdb.log"

#define CFG_LOG_LEVEL "log_level"
#define CFG_LOG_LEVEL_DEFAULT LOG_DEBUG

typedef struct {
    char *filename;
    FILE *file;
    log_level level;
    pid_t pid;
} log_info;

static log_info l;

/**
 * Close the log file.
 */
static void log_close(void)
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

/**
 * Open a log file.
 *
 * @param[in] configuration The global configuration.
 * @return 1 on success, 0 on failure.
 */
static int log_open(cfg_t * configuration)
{
    l.level = cfg_getint(configuration, CFG_LOG_LEVEL);
    if (l.level < LOG_NONE) {
        l.filename = strdup(cfg_getstr(configuration, CFG_LOG_FILE));
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

void lo(log_level level, const char *format, ...)
{
    if (level < l.level) {
        return;
    }
    if (level >= LOG_NONE) {
        return;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    va_list args;
    va_start(args, format);
    flockfile(l.file);
    fprintf(l.file, "%d %10ld.%06ld %6d ", level, tv.tv_sec, (long)tv.tv_usec,
            l.pid);
    /* Flawfinder: ignore format */
    vfprintf(l.file, format, args);
    fprintf(l.file, "\n");
    fflush(l.file);
    funlockfile(l.file);
    va_end(args);
}

/**
 * Convert from a string to a log level enumeration.
 *
 * @param[in] string A string description of a log level.
 * @return a log_level value (LOG_NONE is the default)
 */
static log_level log_level_from_string(const char *string)
{
    if (strcasecmp(string, "error") == 0) {
        return LOG_ERROR;
    }
    if (strcasecmp(string, "info") == 0) {
        return LOG_INFO;
    }
    if (strcasecmp(string, "debug") == 0) {
        return LOG_DEBUG;
    }
    return LOG_NONE;
}

static int log_level_parser(cfg_t * cfg, cfg_opt_t * opt, const char *value,
                            void *result)
{
    *(int *)result = log_level_from_string(value);
    return 0;
}

static cfg_opt_t log_options[] = {
    CFG_STR(CFG_LOG_FILE, CFG_LOG_FILE_DEFAULT, 0),
    CFG_INT_CB(CFG_LOG_LEVEL, CFG_LOG_LEVEL_DEFAULT, 0, log_level_parser),
    CFG_END()
};

/** @ingroup components */
component log_component = {
    log_open,
    log_close,
    log_options,
    SUBCOMPONENTS_NONE
};
