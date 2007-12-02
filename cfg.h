#ifndef __CFG_H
#define __CFG_H

/* 3rd party includes */
#include "confuse.h"

/* project includes */
#include "log.h"

#define CFG_LOG_FILE "log_file"
#define CFG_LOG_FILE_DEFAULT "pdb.log"

#define CFG_LOG_LEVEL "log_level"
#define CFG_LOG_LEVEL_DEFAULT LOG_DEBUG

#define CFG_LISTEN_PORT "listen_port"
#define CFG_LISTEN_PORT_DEFAULT 7668

#define CFG_LISTEN_QDEPTH "listen_qdepth"
#define CFG_LISTEN_QDEPTH_DEFAULT 0

static int log_level_parser(cfg_t * cfg, cfg_opt_t * opt, const char *value,
                            void *result)
{
    *(int *)result = log_level_from_string(value);
    return 0;
}

static cfg_opt_t options[] = {
    CFG_STR(CFG_LOG_FILE, CFG_LOG_FILE_DEFAULT, 0),
    CFG_INT_CB(CFG_LOG_LEVEL, CFG_LOG_LEVEL_DEFAULT, 0, log_level_parser),
    CFG_INT(CFG_LISTEN_PORT, CFG_LISTEN_PORT_DEFAULT, 0),
    CFG_INT(CFG_LISTEN_QDEPTH, CFG_LISTEN_QDEPTH_DEFAULT, 0),
    CFG_END()
};
static cfg_t *configuration = 0;

#define log_file() cfg_getstr(configuration, CFG_LOG_FILE)
#define log_level() cfg_getint(configuration, CFG_LOG_LEVEL)

#define listen_port() cfg_getint(configuration, CFG_LISTEN_PORT)
#define listen_qdepth() cfg_getint(configuration, CFG_LISTEN_QDEPTH)

#endif
