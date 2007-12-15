#ifndef __DB_DRIVER_H
#define __DB_DRIVER_H

/**
 * @file db_driver.h
 * @brief "Interface" for database drivers
 *
 * This file describes the interface which database drivers must satisfy to
 * plug into the proxy framework.
 */

#include "packet.h"

/**
 * db_driver is the interface for all database-specific functions.
 */
typedef struct {
    void (*initialize)(void);
    short (*done)(void);
    short (*expect_commands)(void);
    short (*expect_replies)(void);
    packet_reader get_packet;
    packet_writer put_packet;
    void (*got_command)(packet *);
    packet *(*reduce_replies)(packet_set *);
} db_driver;

/**
 * Load a particular database driver.
 *
 * @param[in] db_driver_name The name of the driver to load.
 * @return A db_driver for the named database.
 */
db_driver db_driver_load(char *db_driver_name);

#endif
