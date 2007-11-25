/* system includes */
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* project includes */
#include "db_driver.h"
#include "delegate.h"
#include "log.h"
#include "server.h"

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

void server(int fd, struct sockaddr_in *addr)
{
    lo(LOG_DEBUG, "server: handling connection on fd %d", fd);

    db_driver db;

    /* XXX: obviously.... */
    db = db_driver_load("mysql");

    /* establish network-level connections to all delegate databases */
    if (delegate_connect() == -1) {
        lo(LOG_ERROR, "error connecting to a delegate: %s", strerror(errno));
        return;
    }

    /* for the initial part of the connection, the server-side drives the
       conversation */
    packet_set *greetings = delegate_action(ACTION_NOOP_ALL, packet_NULL(),
                                            db.put_packet, db.get_packet);
    packet *greeting = db.reduce_replies(greetings);
    packet_set_delete(greetings);

    if (send_reply(fd, greeting, db.put_packet) == -1) {
        lo(LOG_ERROR, "error sending reply: %s", strerror(errno));
        packet_delete(greeting);
        delegate_disconnect();
        return;
    }

    packet_delete(greeting);

    /* loop over input stream */
    while (!db.done()) {
        packet *in_command = packet_new();
        if (!in_command) {
            delegate_disconnect();
            return;
        }

        if (read_command(fd, in_command, db.get_packet) == -1) {
            lo(LOG_ERROR, "error reading command: %s", strerror(errno));
            packet_delete(in_command);
            delegate_disconnect();
            return;
        }

        packet_set *replies = delegate_action(db.actions_from(in_command),
                                              in_command, db.put_packet,
                                              db.get_packet);
        packet_delete(in_command);
        packet *final_reply = db.reduce_replies(replies);
        packet_set_delete(replies);

        if (send_reply(fd, final_reply, db.put_packet) == -1) {
            lo(LOG_ERROR, "error sending reply: %s", strerror(errno));
            packet_delete(final_reply);
            delegate_disconnect();
            return;
        }

        packet_delete(final_reply);
    }

    /* teardown all the delegate connections */
    delegate_disconnect();

    lo(LOG_DEBUG, "server: finished work on fd %d", fd);
    return;
}