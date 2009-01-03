/* system includes */
#include <sys/types.h>
#include <sys/uio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
static short command_is_client_auth;

enum expect_reply_state {
    REP_NONE,
    REP_GREETING,
    REP_SIMPLE,
    REP_TABLE_FIELDS,
    REP_TABLE_ROWS
};

typedef struct {
    short expecting_rows;
    enum expect_reply_state expect_replies;
} delegate_state;
delegate_state *delegate_states;
delegate_id delegate_states_count;

short mysql_driver_initialize(delegate_id max_delegate_id)
{
    done = 0;
    waiting_for_client_auth = 0;
    command_is_client_auth = 0;

    delegate_states = malloc(sizeof(delegate_state) * (max_delegate_id + 1));
    if (!delegate_states) {
        return 0;
    }

    delegate_states_count = max_delegate_id + 1;
    for (delegate_id i = 0; i < delegate_states_count; ++i) {
        delegate_states[i].expecting_rows = 0;
        delegate_states[i].expect_replies = REP_GREETING;
    }
    return 1;
}

short mysql_driver_done(void)
{
    return done;
}

short mysql_driver_expect_replies(void)
{
    if (!done) {
        for (delegate_id i = 0; i < delegate_states_count; ++i) {
            if (delegate_states[i].expect_replies != REP_NONE)
                return 1;
        }
    }
    return 0;
}

short mysql_driver_expect_commands(void)
{
    if (!done) {
        for (delegate_id i = 0; i < delegate_states_count; ++i) {
            if (delegate_states[i].expect_replies == REP_NONE)
                return 1;
        }
    }
    return 0;
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
            long packet_length =
                ((unsigned char)p->bytes[0]) +
                ((unsigned char)p->bytes[1] << 8) +
                ((unsigned char)p->bytes[2] << 16);
            lo(LOG_DEBUG, "mysql_driver_get_packet: read header for packet "
               "number %d, expected to be %ld bytes", p->bytes[3],
               packet_length);
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

db_driver_command_type mysql_driver_command(packet * in_command)
{
    command_is_client_auth = 0;

    for (delegate_id i = 0; i < delegate_states_count; ++i) {
        delegate_states[i].expecting_rows = 0;
        delegate_states[i].expect_replies = REP_SIMPLE;
    }

    if (waiting_for_client_auth) {
        waiting_for_client_auth = 0;
        command_is_client_auth = 1;
        return DB_DRIVER_COMMAND_TYPE_OTHER;
    }

    enum enum_server_command command =
        (enum enum_server_command)(unsigned char)in_command->bytes[4];
    lo(LOG_DEBUG, "mysql_driver_command: I've got a %u packet...", command);

    db_driver_command_type type = DB_DRIVER_COMMAND_TYPE_OTHER;

    /* if we see a QUIT command, don't expect the delegates to respond */
    if (command == COM_QUIT) {
        for (delegate_id i = 0; i < delegate_states_count; ++i) {
            delegate_states[i].expect_replies = REP_NONE;
        }
        done = 1;
    } else {
        lo(LOG_DEBUG, "mysql_driver_command(*): REP_NONE -> REP_SIMPLE");
    }

    if (command == COM_QUERY) {
        for (delegate_id i = 0; i < delegate_states_count; ++i) {
            delegate_states[i].expecting_rows = 1;
        }
        type = DB_DRIVER_COMMAND_TYPE_SQL;
    }

    return type;
}

short mysql_driver_delegate_filter(delegate_id id)
{
    lo(LOG_DEBUG, "mysql_driver_delegate_filter(%hu): %d", id,
       delegate_states[id].expect_replies);
    return (delegate_states[id].expect_replies == REP_NONE);
}

void mysql_driver_reply(delegate_id id, packet * p)
{
    if ((unsigned char)(p->bytes[4]) == 0xff) {
        lo(LOG_ERROR, "mysql_driver_reply(%hu): ERROR!!", id);

        char *error = malloc(p->size - 7 + 1);
        if (error) {
            strncpy(error, p->bytes + 7, p->size - 7);
            error[p->size - 7] = 0;
            lo(LOG_ERROR, "mysql_driver_reply(%hu): %s", id, error);
        }
    }

    switch (delegate_states[id].expect_replies) {
    case REP_GREETING:
        waiting_for_client_auth = 1;
        lo(LOG_DEBUG, "mysql_driver_reply(%hu): REP_GREETING -> REP_NONE",
           id);
        delegate_states[id].expect_replies = REP_NONE;
        break;
    case REP_SIMPLE:
        if (p->bytes[4] == 0) {
            lo(LOG_DEBUG, "mysql_driver_reply(%hu): REP_SIMPLE -> REP_NONE",
               id);
            delegate_states[id].expect_replies = REP_NONE;
        } else {
            lo(LOG_DEBUG,
               "mysql_driver_reply(%hu): REP_SIMPLE -> REP_TABLE_FIELDS", id);
            delegate_states[id].expect_replies = REP_TABLE_FIELDS;
        }
        break;
    case REP_TABLE_FIELDS:
        if ((unsigned char)(p->bytes[4]) == 0xfe) {
            if (delegate_states[id].expecting_rows) {
                lo(LOG_DEBUG,
                   "mysql_driver_reply(%hu): REP_TABLE_FIELDS "
                   "-> REP_TABLE_ROWS", id);
                delegate_states[id].expect_replies = REP_TABLE_ROWS;
            } else {
                lo(LOG_DEBUG,
                   "mysql_driver_reply(%hu): REP_TABLE_FIELDS -> REP_NONE",
                   id);
                delegate_states[id].expect_replies = REP_NONE;
            }
        } else {
            lo(LOG_DEBUG, "mysql_driver_reply(%hu): field", id);
        }
        break;
    case REP_TABLE_ROWS:
        if ((unsigned char)(p->bytes[4]) == 0xfe) {
            lo(LOG_DEBUG,
               "mysql_driver_reply(%hu): REP_TABLE_ROWS -> REP_NONE", id);
            delegate_states[id].expect_replies = REP_NONE;
        } else {
            lo(LOG_DEBUG, "mysql_driver_reply(%hu): row", id);
        }
        break;
    case REP_NONE:
        lo(LOG_ERROR, "mysql_driver_reply(%hu): I wasn't expecting"
           "any replies!", id);
        break;
    };
}

packet *mysql_driver_reduce_replies(packet_set * replies)
{
    lo(LOG_DEBUG, "mysql_driver_reduce_replies: hack hack hack");
    return packet_copy(packet_set_get(replies, 0));
}

static void edit_packet_length(packet * p)
{
    long packet_length = p->size - HEADER_SIZE;
    p->bytes[0] = (unsigned char)(packet_length);
    p->bytes[1] = (unsigned char)(packet_length >> 8);
    p->bytes[2] = (unsigned char)(packet_length >> 16);
}

int mysql_driver_rewrite_command(packet * in, packet * out,
                                 const char *db_name)
{
    if (command_is_client_auth) {
        int db_name_offset = 36 + strlen(in->bytes + 36) + 2;
        out->size = out->allocated = db_name_offset + strlen(db_name) + 1;
        out->bytes = malloc(out->size);
        if (!out->bytes) {
            out->size = out->allocated = 0;
            return 0;
        }
        bcopy(in->bytes, out->bytes, db_name_offset);
        bcopy(db_name, out->bytes + db_name_offset, strlen(db_name) + 1);
        edit_packet_length(out);
    } else {
        packet *p = packet_copy(in);
        if (!p) {
            return 0;
        }
        out->bytes = p->bytes;
        out->allocated = p->allocated;
        out->size = p->size;
        free(p);
    }

    return 1;
}

char *mysql_driver_sql_extract(packet * in_command)
{
    char *sql = malloc(in_command->size - 5 + 1);
    if (sql) {
        strncpy(sql, in_command->bytes + 5, in_command->size - 5);
        sql[in_command->size - 5] = 0;
    }
    return sql;
}
