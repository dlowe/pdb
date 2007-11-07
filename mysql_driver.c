/* project includes */
#include "mysql_driver.h"

short mysql_driver_done(void)
{
    return 1;
}

command mysql_driver_get_next_command(int fd)
{
    return 0;
}

action *mysql_driver_actions_from(command in_command)
{
    return 0;
}

reply mysql_driver_reduce_replies(reply * replies)
{
    return 0;
}

void mysql_driver_send_reply(int fd, reply in_reply)
{
}
