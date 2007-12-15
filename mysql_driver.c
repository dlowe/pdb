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
static short waiting_for_client_auth;

enum expect_reply_state {
    REP_NONE,
    REP_GREETING,
    REP_SIMPLE,
    REP_TABLE_FIELDS,
    REP_TABLE_ROWS
} expect_replies;

void mysql_driver_initialize(void)
{
    done = 0;
    expect_replies = REP_GREETING;
    waiting_for_client_auth = 0;
}

short mysql_driver_done(void)
{
    return done;
}

short mysql_driver_expect_replies(void)
{
    return (!done) && (expect_replies != REP_NONE);
}

short mysql_driver_expect_commands(void)
{
    return (!done) && (expect_replies == REP_NONE);
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
    expect_replies = REP_SIMPLE;
    if (waiting_for_client_auth) {
        waiting_for_client_auth = 0;
        return ACTION_PROXY_ALL;
    }

    enum enum_server_command command =
        (enum enum_server_command)(unsigned char)in_command->bytes[4];
    lo(LOG_DEBUG, "mysql_driver_actions_from: I've got a %u packet...",
       command);

    /* if we see a QUIT command, don't expect the delegates to respond */
    if (command == COM_QUIT) {
        expect_replies = REP_NONE;
        done = 1;
    }
    if (command == COM_QUERY) {
        lo(LOG_DEBUG, "mysql_driver_actions_from: query: '%*s'",
           in_command->size - 5, in_command->bytes + 5);
    }

    return ACTION_PROXY_ALL;
}

packet *mysql_driver_reduce_replies(packet_set * replies)
{
    packet *p = packet_copy(packet_set_get(replies, 0));

    switch (expect_replies) {
    case REP_GREETING:
        waiting_for_client_auth = 1;
        expect_replies = REP_NONE;
        break;
    case REP_SIMPLE:
        lo(LOG_DEBUG, "mysql_driver_reduce_replies: packet of "
           "size %d says to expect %d fields", p->size, p->bytes[4]);
        if (p->bytes[4] == 0) {
            expect_replies = REP_NONE;
        } else {
            expect_replies = REP_TABLE_FIELDS;
        }
        break;
    case REP_TABLE_FIELDS:
        lo(LOG_DEBUG, "mysql_driver_reduce_replies: field");
        if ((unsigned char)(p->bytes[4]) == 0xfe) {
            if (p->size > 5) {
                expect_replies = REP_NONE;
            } else {
                expect_replies = REP_TABLE_ROWS;
            }
        }
        break;
    case REP_TABLE_ROWS:
        lo(LOG_DEBUG, "mysql_driver_reduce_replies: row");
        if ((unsigned char)(p->bytes[4]) == 0xfe) {
            expect_replies = REP_NONE;
        }
        break;
    case REP_NONE:
        lo(LOG_ERROR, "mysql_driver_reduce_replies: I wasn't expecting"
           "any replies!");
        return 0;
    };
    return packet_copy(packet_set_get(replies, 0));
}
