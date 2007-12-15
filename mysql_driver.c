/* system includes */
#include <sys/types.h>
#include <sys/uio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

/* project includes */
#include "log.h"
#include "mysql_driver.h"

/** XXX: crap that really should be used directly from mysql headers! */
#define HEADER_SIZE 4
enum enum_server_command {
    COM_SLEEP, COM_QUIT, COM_INIT_DB, COM_QUERY, COM_FIELD_LIST,
    COM_CREATE_DB, COM_DROP_DB, COM_REFRESH, COM_SHUTDOWN, COM_STATISTICS,
    COM_PROCESS_INFO, COM_CONNECT, COM_PROCESS_KILL, COM_DEBUG, COM_PING,
    COM_TIME, COM_DELAYED_INSERT, COM_CHANGE_USER, COM_BINLOG_DUMP,
    COM_TABLE_DUMP, COM_CONNECT_OUT, COM_REGISTER_SLAVE,
    COM_STMT_PREPARE, COM_STMT_EXECUTE, COM_STMT_SEND_LONG_DATA,
    COM_STMT_CLOSE, COM_STMT_RESET, COM_SET_OPTION, COM_STMT_FETCH,
    /* don't forget to update const char *command_name[] in sql_parse.cc */

    /* Must be last */
    COM_END
};

static short done;
static short expect_commands;
static short expect_replies;

enum established_state {
    EST_WAITING_FOR_GREETING,
    EST_WAITING_FOR_CLIENT_AUTH,
    EST_WAITING_FOR_RESPONSE,
    EST_ESTABLISHED
} established;

void mysql_driver_initialize(void)
{
    done = 0;
    expect_replies = 1;
    expect_commands = 0;
    established = EST_WAITING_FOR_GREETING;
}

short mysql_driver_done(void)
{
    return done;
}

short mysql_driver_expect_replies(void)
{
    return expect_replies;
}

short mysql_driver_expect_commands(void)
{
    return expect_commands;
}

packet_status mysql_driver_get_packet(int fd, packet * p)
{
    if (p->bytes == 0) {
        p->size = 0;
        p->allocated = HEADER_SIZE;
        p->bytes = malloc(p->allocated);
        if (!p->bytes) {
            return PACKET_ERROR;
        }
    }

    /* reading the header */
    if (p->size < HEADER_SIZE) {
        int len = read(fd, p->bytes + p->size, HEADER_SIZE - p->size);
        if (len <= 0) {
            free(p->bytes);
            p->bytes = 0;
            p->allocated = 0;
            p->size = 0;
            return (len == 0) ? PACKET_EOF : PACKET_ERROR;
        }
        p->size += len;
        if (p->size == HEADER_SIZE) {
            lo(LOG_DEBUG, "mysql_driver_get_packet: read header for packet "
               "number %d", p->bytes[3]);
        }
        return PACKET_INCOMPLETE;
    }

    /* reading the body */
    long packet_length =
        ((unsigned char)p->bytes[0]) +
        ((unsigned char)p->bytes[1] << 8) +
        ((unsigned char)p->bytes[2] << 16);

    if (p->allocated < (packet_length + HEADER_SIZE)) {
        p->allocated = (packet_length + HEADER_SIZE);
        p->bytes = realloc(p->bytes, p->allocated);
        if (!p->bytes) {
            p->allocated = 0;
            p->size = 0;
            return PACKET_ERROR;
        }
    }

    int len = read(fd, p->bytes + p->size,
                   packet_length - (p->size - HEADER_SIZE));
    if (len <= 0) {
        free(p->bytes);
        p->bytes = 0;
        p->allocated = 0;
        p->size = 0;
        return (len == 0) ? PACKET_EOF : PACKET_ERROR;
    }

    p->size += len;

    if (p->size < (packet_length + HEADER_SIZE)) {
        lo(LOG_DEBUG, "mysql_driver_get_packet: read %ld of %ld bytes",
           p->size, packet_length + HEADER_SIZE);
        return PACKET_INCOMPLETE;
    }
    lo(LOG_DEBUG, "mysql_driver_get_packet: completed packet of length %ld",
       packet_length + HEADER_SIZE);
    return PACKET_COMPLETE;
}

packet_status mysql_driver_put_packet(int fd, packet * p, int *sent)
{
    if (p->bytes == 0) {
        return PACKET_ERROR;
    }
    if (sent == 0) {
        return PACKET_ERROR;
    }
    if (*sent >= p->size) {
        return PACKET_ERROR;
    }

    int len = write(fd, p->bytes + *sent, p->size - *sent);
    if (len <= 0) {
        return PACKET_ERROR;
    }

    *sent += len;
    if (*sent < p->size) {
        lo(LOG_DEBUG, "mysql_driver_put_packet: wrote %ld of %ld bytes",
           *sent, p->size);
        return PACKET_INCOMPLETE;
    }
    lo(LOG_DEBUG, "mysql_driver_put_packet: completed packet of length %ld",
       p->size);
    return PACKET_COMPLETE;
}

action mysql_driver_actions_from(packet * in_command)
{
    expect_replies = 1;
    expect_commands = 0;
    if (established == EST_WAITING_FOR_CLIENT_AUTH) {
        established = EST_WAITING_FOR_RESPONSE;
        return ACTION_PROXY_ALL;
    }

    enum enum_server_command command =
        (enum enum_server_command)(unsigned char)in_command->bytes[4];
    lo(LOG_DEBUG, "mysql_driver_actions_from: I've got a %u packet...",
       command);

    /* if we see a QUIT command, don't expect the delegates to respond */
    if (command == COM_QUIT) {
        expect_replies = 0;
        done = 1;
    }

    return ACTION_PROXY_ALL;
}

packet *mysql_driver_reduce_replies(packet_set * replies)
{
    if (established == EST_WAITING_FOR_GREETING) {
        established = EST_WAITING_FOR_CLIENT_AUTH;
    } else if (established == EST_WAITING_FOR_RESPONSE) {
        established = EST_ESTABLISHED;
    }
    expect_replies = 0;
    expect_commands = 1;
    return packet_copy(packet_set_get(replies, 0));
}
