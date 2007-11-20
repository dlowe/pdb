/* system includes */
#include <sys/types.h>
#include <sys/uio.h>
#include <syslog.h>
#include <stdarg.h>
#include <unistd.h>

/* project includes */
#include "mysql_driver.h"

static short done = 1;

short mysql_driver_done(void)
{
    return done;
}

command mysql_driver_get_next_command(int fd)
{
    long command_length;
    char buffer[4096];
    int len;

    len = read(fd, &buffer, 4);
    if (len == 4) {
        command_length = (long)(*((unsigned int *)(&buffer)) & 0xFFFFFF);
        syslog(LOG_INFO, "read header for command %c of length %ld",
               buffer[3], command_length);
        len = read(fd, &buffer, command_length);
        if (len == command_length) {
            syslog(LOG_INFO, "command: %s", buffer);
        }
    }
    return 0;
}

reply_status mysql_driver_get_next_reply(int fd, reply * r)
{
    return REPLY_COMPLETE;
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
