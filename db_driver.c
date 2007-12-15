/* project includes */
#include "db_driver.h"
#include "mysql_driver.h"

db_driver db_driver_load(char *db_driver_name)
{
    db_driver db;

    db.initialize = mysql_driver_initialize;
    db.done = mysql_driver_done;
    db.expect_commands = mysql_driver_expect_commands;
    db.expect_replies = mysql_driver_expect_replies;
    db.get_packet = mysql_driver_get_packet;
    db.put_packet = mysql_driver_put_packet;
    db.actions_from = mysql_driver_actions_from;
    db.reduce_replies = mysql_driver_reduce_replies;

    return db;
}
