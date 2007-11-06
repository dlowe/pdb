/* system includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* project includes */
#include "concurrency.h"
#include "daemon.h"

static void connection_handler(int fd, struct sockaddr_in *addr)
{
    write(fd, "poot!\n", 6);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return;
}

static short dead;
static void sigterm_handler(int sig)
{
    dead = 1;
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

    /* wait for connections; child processes handle each connection */
    dead = 0;
    while (!dead) {
        struct pollfd socket_poll;

        socket_poll.fd = socket_fd;
        socket_poll.events = POLLIN;
        socket_poll.revents = 0;
        if (poll(&socket_poll, 1, -1) > 0) {
            struct sockaddr_in connection_addr;
            socklen_t connection_addr_length;
            int connection_fd =
                accept(socket_fd, (struct sockaddr *)&connection_addr,
                       &connection_addr_length);

            if (connection_fd > 0) {
                int child = concurrency_handle_connection(connection_fd,
                                                          &connection_addr,
                                                          connection_handler);
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
