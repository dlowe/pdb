/* system includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

/* project includes */
#include "concurrency.h"
#include "daemon.h"
#include "db_driver.h"
#include "delegate.h"

static void driver(int fd, struct sockaddr_in *addr);

static short dead;
static void sigterm_handler(int sig)
{
    dead = 1;
}

static void sigchld_handler(int sig)
{
    /* noop: just break out of the poll() */
}

static void signal_block(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    sigprocmask(SIG_BLOCK, &set, 0);
}

static void signal_unblock(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    sigprocmask(SIG_UNBLOCK, &set, 0);
}

int main(int argc, char **argv)
{
    if (daemon_begin() == -1) {
        fprintf(stderr, "error in daemon_begin(): %s\n", strerror(errno));
        exit(1);
    }

    /* set up network for listening */
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        daemon_error("can't create socket: %s\n", strerror(errno));
        exit(1);
    }

    int one = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
                   sizeof(one)) == -1) {
        daemon_error("can't set reuseaddr: %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_in bind_addr;
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = 5032;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_fd, (struct sockaddr *)&bind_addr,
             sizeof(bind_addr)) == -1) {
        daemon_error("can't bind socket: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }

    if (listen(socket_fd, 0) == -1) {
        daemon_error("can't listen to socket: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }

    daemon_done();

    concurrency_setup();

    /* set up signal handling */
    signal(SIGTERM, sigterm_handler);
    signal(SIGCHLD, sigchld_handler);

    /* wait for connections; child processes handle each connection */
    dead = 0;
    while (!dead) {
        struct pollfd socket_poll;

        socket_poll.fd = socket_fd;
        socket_poll.events = POLLIN;
        socket_poll.revents = 0;

        signal_unblock(SIGCHLD);
        int r = poll(&socket_poll, 1, -1);
        signal_block(SIGCHLD);

        if (r > 0) {
            struct sockaddr_in connection_addr;
            socklen_t connection_addr_length;
            int connection_fd =
                accept(socket_fd, (struct sockaddr *)&connection_addr,
                       &connection_addr_length);

            if (connection_fd > 0) {
                int child = concurrency_handle_connection(connection_fd,
                                                          &connection_addr,
                                                          driver);
                if (child == -1) {
                    exit(1);
                }
            }
        }
        concurrency_join_finished();
    }

    concurrency_teardown();

    close(socket_fd);
    exit(0);
}

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

/**
 * Top-level sequencing of a single connection.
 *
 * @param[in] fd connected file descriptor
 * @param[in] addr information about the connection
 */
static void driver(int fd, struct sockaddr_in *addr)
{
    db_driver db;

    db = db_driver_load("mysql");

    /* establish network-level connections to all delegate databases */
    if (delegate_connect() == -1) {
        syslog(LOG_ERR, "error connecting to a delegate: %m");
        shutdown(fd, SHUT_RDWR);
        close(fd);
        return;
    }

    /* for the initial part of the connection, the server-side drives the
       conversation */
    packet *greetings = delegate_action(ACTION_NOOP_ALL, packet_null(),
                                        db.put_packet, db.get_packet);
    packet greeting = db.reduce_replies(greetings);
    if (send_reply(fd, &greeting, db.put_packet) == -1) {
        syslog(LOG_ERR, "error sending reply: %m");
        shutdown(fd, SHUT_RDWR);
        close(fd);
        return;
    }

    /* loop over input stream */
    while (!db.done()) {
        packet in_command;

        if (read_command(fd, &in_command, db.get_packet) == -1) {
            syslog(LOG_ERR, "error reading command: %m");
            shutdown(fd, SHUT_RDWR);
            close(fd);
            return;
        }

        packet *replies = delegate_action(db.actions_from(in_command),
                                          &in_command, db.put_packet,
                                          db.get_packet);
        packet final_reply = db.reduce_replies(replies);

        if (send_reply(fd, &final_reply, db.put_packet) == -1) {
            syslog(LOG_ERR, "error sending reply: %m");
            shutdown(fd, SHUT_RDWR);
            close(fd);
            return;
        }
    }

    /* teardown all the delegate connections */
    delegate_disconnect();

    shutdown(fd, SHUT_RDWR);
    close(fd);
    return;
}
