/* project includes */
#include "component.h"
#include "db_driver.h"
#include "mysql_driver.h"

short (*db_driver_initialize) (delegate_id) = 0;
short (*db_driver_done) (void) = 0;
short (*db_driver_expect_commands) (void) = 0;
short (*db_driver_expect_replies) (void) = 0;
short (*db_driver_got_error) (void);
packet *(*db_driver_error_packet) (void);
delegate_filter db_driver_delegate_filter = 0;
packet_reader db_driver_get_packet = 0;
packet_writer db_driver_put_packet = 0;
db_driver_command_type(*db_driver_command) (packet *) = 0;
void (*db_driver_command_done) (delegate_filter *) = 0;
void (*db_driver_reply) (delegate_id, packet *);
packet *(*db_driver_reduce_replies) (packet_set *) = 0;
int (*db_driver_rewrite_command) (packet *, packet *, const char *) = 0;
char *(*db_driver_sql_extract) (packet *) = 0;
char *(*db_driver_table_extract) (packet *) = 0;

static int db_driver_load(cfg_t * configuration)
{
    db_driver_initialize = mysql_driver_initialize;
    db_driver_done = mysql_driver_done;
    db_driver_expect_commands = mysql_driver_expect_commands;
    db_driver_expect_replies = mysql_driver_expect_replies;
    db_driver_got_error = mysql_driver_got_error;
    db_driver_error_packet = mysql_driver_error_packet;
    db_driver_delegate_filter = mysql_driver_delegate_filter;
    db_driver_get_packet = mysql_driver_get_packet;
    db_driver_put_packet = mysql_driver_put_packet;
    db_driver_command = mysql_driver_command;
    db_driver_command_done = mysql_driver_command_done;
    db_driver_reply = mysql_driver_reply;
    db_driver_reduce_replies = mysql_driver_reduce_replies;
    db_driver_rewrite_command = mysql_driver_rewrite_command;
    db_driver_sql_extract = mysql_driver_sql_extract;
    db_driver_table_extract = mysql_driver_table_extract;

    return 1;
}

#define CFG_DB_TYPE "db_type"
#define CFG_DB_TYPE_DEFAULT "mysql"

static cfg_opt_t db_driver_options[] = {
    CFG_STR(CFG_DB_TYPE, CFG_DB_TYPE_DEFAULT, 0),
    CFG_END()
};

/** @ingroup components */
component db_driver_component = {
    db_driver_load,
    SHUTDOWN_NONE,
    db_driver_options,
    SUBCOMPONENTS_NONE
};
