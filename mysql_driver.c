/* system includes */
#include <sys/types.h>
#include <sys/uio.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdlib.h>
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
    if (r->bytes == 0) {
        r->size = 0;
        r->allocated = 4;
        r->bytes = malloc(r->allocated);
        if (!r->bytes) {
            return REPLY_ERROR;
        }
    }

    /* reading the header */
    if (r->size < 4) {
        int len = read(fd, r->bytes + r->size, 4 - r->size);
        if (len == -1) {
            free(r->bytes);
            r->bytes = 0;
            r->allocated = 0;
            r->size = 0;
            return REPLY_ERROR;
        }
        r->size += len;
        return REPLY_INCOMPLETE;
    }

    /* reading the body */
    long command_length = (long)(*((unsigned int *)(&r->bytes)) & 0xFFFFFF);

    syslog(LOG_INFO, "read header for reply %c of length %ld", r->bytes[3],
           command_length);

    if (r->allocated < (command_length + 4)) {
        r->allocated = (command_length + 4);
        r->bytes = realloc(r->bytes, r->allocated);
        if (!r->bytes) {
            r->allocated = 0;
            r->size = 0;
            return REPLY_ERROR;
        }
    }

    int len = read(fd, r->bytes + r->size, command_length - 4 - r->size);
    if (len <= 0) {
        free(r->bytes);
        r->bytes = 0;
        r->allocated = 0;
        r->size = 0;
        return REPLY_ERROR;
    }

    r->size += len;

    if (r->size < (command_length + 4)) {
        return REPLY_INCOMPLETE;
    }
    return REPLY_COMPLETE;
}

action *mysql_driver_actions_from(command in_command)
{
    return 0;
}

reply mysql_driver_reduce_replies(reply * replies)
{
    reply XX;
    return XX;
}

void mysql_driver_send_reply(int fd, reply in_reply)
{
}
