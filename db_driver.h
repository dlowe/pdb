#ifndef _DB_DRIVER_H
#define _DB_DRIVER_H

/**
 * @file db_driver.h
 * @brief "Interface" for database drivers
 *
 * This file describes the interface which database drivers must satisfy to
 * plug into the proxy framework.
 */

#include "command.h"
#include "reply.h"
#include "action.h"

/**
 * db_driver is the interface for all database-specific functions.
 */
typedef struct {
    short (*done)(void);
    command (*get_next_command)(int);
    reply_status (*get_next_reply)(int, reply *);
    action* (*actions_from)(command);
    reply (*reduce_replies)(reply[]);
    void (*send_reply)(int, reply);
} db_driver;

/**
 * Load a particular database driver.
 *
 * @param[in] db_driver_name The name of the driver to load.
 * @return A db_driver for the named database.
 */
db_driver db_driver_load(char *db_driver_name);

#endif
