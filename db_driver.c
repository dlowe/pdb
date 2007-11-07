/* project includes */
#include "db_driver.h"
#include "mysql_driver.h"

db_driver db_driver_load(char *db_driver_name)
{
    db_driver db;

    db.done = mysql_driver_done;
    db.get_next_command = mysql_driver_get_next_command;
    db.actions_from = mysql_driver_actions_from;
    db.reduce_replies = mysql_driver_reduce_replies;
    db.send_reply = mysql_driver_send_reply;

    return db;
}
