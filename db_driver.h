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
#include "component.h"

DECLARE_COMPONENT(db_driver);

extern void (*db_driver_initialize)(void);
extern short (*db_driver_done)(void);
extern short (*db_driver_expect_commands)(void);
extern short (*db_driver_expect_replies)(void);
extern packet_reader db_driver_get_packet;
extern packet_writer db_driver_put_packet;
extern void (*db_driver_got_command)(packet *);
extern packet *(*db_driver_reduce_replies)(packet_set *);

#endif
