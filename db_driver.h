#ifndef __DB_DRIVER_H
#define __DB_DRIVER_H

/**
 * @file db_driver.h
 * @brief "Interface" for database drivers
 *
 * This file describes the interface which database drivers must satisfy to
 * plug into the proxy framework.
 *
 * The db_driver component should be exclusively used by the server component.
 */

#include "packet.h"
#include "component.h"
#include "delegate_id.h"

DECLARE_COMPONENT(db_driver);

typedef enum {
    DB_DRIVER_COMMAND_TYPE_SQL,
    DB_DRIVER_COMMAND_TYPE_OTHER
} db_driver_command_type;

extern short (*db_driver_initialize) (delegate_id);
extern short (*db_driver_done) (void);
extern short (*db_driver_expect_commands) (void);
extern short (*db_driver_expect_replies) (void);
extern short (*db_driver_delegate_filter) (delegate_id);
extern packet_reader db_driver_get_packet;
extern packet_writer db_driver_put_packet;
extern db_driver_command_type(*db_driver_command) (packet *);
extern void (*db_driver_reply) (delegate_id, packet *);
extern packet *(*db_driver_reduce_replies) (packet_set *);
extern int (*db_driver_rewrite_command) (packet *, packet *, const char *);
extern char *(*db_driver_sql_extract) (packet *);

#endif
