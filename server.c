/* system includes */
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* project includes */
#include "db_driver.h"
#include "delegate.h"
#include "log.h"
#include "map.h"
#include "server.h"
#include "sql.h"

/**
 * Synchronously send a single reply.
 *
 * @param[in] fd connected file descriptor
 * @param[in] p packet to send
 * @param[in] put_packet function to use to send packet
 * @return 0 on success, -1 on failure
 */
static int send_reply(int fd, packet * p, packet_writer put_packet)
{
    int sent = 0;
    packet_status status;

    do {
        status = put_packet(fd, p, &sent);
    } while (status == PACKET_INCOMPLETE);

    if (status == PACKET_ERROR) {
        return -1;
    }

    return 0;
}

/**
 * Synchronously read a single command.
 *
 * @param[in] fd a connected file descriptor
 * @param[in,out] p packet buffer to fill
 * @param[in] get_packet function to use to read packet
 * @return 0 on success, -1 on failure
 */
static int read_command(int fd, packet * p, packet_reader get_packet)
{
    short read_complete = 0;
    p->bytes = 0;

    while (!read_complete) {
        switch (get_packet(fd, p)) {
        case PACKET_EOF:
        case PACKET_ERROR:
            return -1;
        case PACKET_INCOMPLETE:
            read_complete = 0;
            break;
        case PACKET_COMPLETE:
            read_complete = 1;
            break;
        };
    }

    return 0;
}

static delegate_filter_result *command_delegate_mask;
static delegate_filter_result command_delegate_filter(delegate_id id)
{
    return command_delegate_mask[id];
}
static void command_delegate_init(void)
{
    command_delegate_mask = malloc(sizeof(short) * delegate_get_count());
    for (delegate_id i = 0; i < delegate_get_count(); ++i) {
        command_delegate_mask[i] = DELEGATE_FILTER_USE;
    }
}
static void command_delegate_all(void)
{
    for (delegate_id i = 0; i < delegate_get_count(); ++i) {
        command_delegate_mask[i] = DELEGATE_FILTER_USE;
    }
}
static void command_delegate_master(void)
{
    for (delegate_id i = 0; i < delegate_get_count(); ++i) {
        if (i == delegate_master_id()) {
            command_delegate_mask[i] = DELEGATE_FILTER_USE;
        } else {
            command_delegate_mask[i] = DELEGATE_FILTER_DONT_USE;
        }
    }
}
static void command_delegate_random_partition(void)
{
    delegate_id random_id;
    do {
        /* Flawfinder: ignore random */
        random_id = random() % delegate_get_count();
    } while (random_id == delegate_master_id());

    for (delegate_id i = 0; i < delegate_get_count(); ++i) {
        if (i == random_id) {
            command_delegate_mask[i] = DELEGATE_FILTER_USE;
        } else {
            command_delegate_mask[i] = DELEGATE_FILTER_DONT_USE;
        }
    }
}

void server(int fd, struct sockaddr_in *addr)
{
    delegate_filter put_filters[] = { command_delegate_filter, 0 };
    delegate_filter get_filters[] =
        { command_delegate_filter, db_driver_delegate_filter, 0 };

    command_delegate_init();

    if (!db_driver_initialize(delegate_get_count())) {
        lo(LOG_ERROR, "server: error initializing database driver");
        return;
    }

    /* establish network-level connections to all delegate databases */
    if (delegate_connect() == -1) {
        lo(LOG_ERROR, "server: error connecting to a delegate: %s",
           strerror(errno));
        return;
    }

    /* loop over conversation between client and delegates */
    while (!db_driver_done()) {
        /* read commands and delegate them */
        while (db_driver_expect_commands()) {
            packet *in_command = packet_new();
            if (!in_command) {
                lo(LOG_ERROR, "server: out of memory!");
                delegate_disconnect();
                return;
            }

            lo(LOG_DEBUG, "server: waiting for next command...");
            if (read_command(fd, in_command, db_driver_get_packet) == -1) {
                if ((errno != ECONNRESET) && (errno != EINPROGRESS)) {
                    lo(LOG_ERROR, "server: error reading command: %s",
                       strerror(errno));
                } else {
                    lo(LOG_DEBUG, "server: client went away");
                }
                packet_delete(in_command);
                delegate_disconnect();
                return;
            }

            /* default is to proxy command to all delegates */
            command_delegate_all();

            switch (db_driver_command(in_command)) {
            case DB_DRIVER_COMMAND_TYPE_SQL:
                {
                    char *sql = db_driver_sql_extract(in_command);
                    if (!sql) {
                        lo(LOG_ERROR, "server: error extracting SQL");
                        packet_delete(in_command);
                        delegate_disconnect();
                        return;
                    }

                    lo(LOG_DEBUG, "server: query '%s'", sql);

                    switch (sql_get_type(sql)) {
                    case SQL_TYPE_MASTER:
                        command_delegate_master();
                        break;
                    case SQL_TYPE_PARTITIONED:
                        {
                            long *foo = sql_get_map_keys(sql);
                            if (foo) {
                            } else {
                                lo(LOG_INFO,
                                   "server: XX: not doing the right thing");
                                command_delegate_all();
                            }
                            break;
                        }
                    }

                    free(sql);
                    break;
                }
            case DB_DRIVER_COMMAND_TYPE_TABLE_META:
                {
                    char *table = db_driver_table_extract(in_command);
                    if (!table) {
                        lo(LOG_ERROR, "server: error extracting table");
                        packet_delete(in_command);
                        delegate_disconnect();
                        return;
                    }

                    lo(LOG_ERROR, "server: table '%s'", table);

                    switch (sql_get_table_type(table)) {
                    case SQL_TABLE_TYPE_MASTER:
                        command_delegate_master();
                        break;
                    case SQL_TABLE_TYPE_PARTITIONED:
                        command_delegate_random_partition();
                        break;
                    }

                    free(table);
                    break;
                }
            case DB_DRIVER_COMMAND_TYPE_UNSUPPORTED:
                {
                    lo(LOG_ERROR, "server: got unsupported command");
                    packet_delete(in_command);
                    delegate_disconnect();
                    return;
                }
            case DB_DRIVER_COMMAND_TYPE_OTHER:
                break;
            };

            lo(LOG_DEBUG, "server: delegating command...");
            if (!delegate_put(put_filters, db_driver_put_packet,
                              db_driver_rewrite_command, in_command)) {
                lo(LOG_ERROR, "server: error delegating command");
                packet_delete(in_command);
                delegate_disconnect();
                return;
            }

            packet_delete(in_command);
        }

        db_driver_command_done(put_filters);

        /* read replies from delegates, reduce and return them */
        while (db_driver_expect_replies()) {
            lo(LOG_DEBUG, "server: waiting for reply...");

            packet_set *replies = delegate_get(get_filters,
                                               db_driver_get_packet);
            if (!replies) {
                lo(LOG_ERROR, "server: error getting delegate replies");
                delegate_disconnect();
                return;
            }

            /* let the db driver know about each reply packet */
            for (delegate_id i = 0; i < delegate_get_count(); ++i) {
                packet *p = packet_set_get(replies, i);
                if ((p) && (p->size)) {
                    db_driver_reply(i, p);
                }
            }

            /* If there's been a driver-level error in at least one
               delegate, we still have to remain in the while (expecting
               replies) loop to keep the other delegates' states consistent.
               The error will be returned to the client afterwards. */
            if (!db_driver_got_error()) {
                packet *final_reply = db_driver_reduce_replies(replies);

                lo(LOG_DEBUG, "server: returning reply...");

                if (send_reply(fd, final_reply, db_driver_put_packet) == -1) {
                    lo(LOG_ERROR, "server: error sending reply: %s",
                       strerror(errno));
                    packet_set_delete(replies);
                    packet_delete(final_reply);
                    delegate_disconnect();
                    return;
                }

                packet_delete(final_reply);
            }

            packet_set_delete(replies);
        }
        if (db_driver_got_error()) {
            packet *error = db_driver_error_packet();
            if (send_reply(fd, error, db_driver_put_packet) == -1) {
                lo(LOG_ERROR, "server: error sending reply: %s",
                   strerror(errno));
                packet_delete(error);
                return;
            }
            packet_delete(error);
        }

        lo(LOG_DEBUG, "server: done with this conversation.");
    }

    /* teardown all the delegate connections */
    delegate_disconnect();

    lo(LOG_DEBUG, "server: finished work on fd %d", fd);
    return;
}

static component *server_subcomponents[] = {
    SUBCOMPONENT(db_driver),
    SUBCOMPONENT(delegate),
    SUBCOMPONENT(map),
    SUBCOMPONENT(sql),
    SUBCOMPONENT_END()
};

/** @ingroup components */
component server_component = {
    INITIALIZE_NONE,
    SHUTDOWN_NONE,
    OPTIONS_NONE,
    server_subcomponents
};
