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
#include <unistd.h>

/* 3rd party includes */
#include "confuse.h"

/* project includes */
#include "component.h"
#include "concurrency.h"
#include "daemon.h"
#include "log.h"
#include "server.h"

static void usage(void)
{
    fprintf(stderr, "usage: pdb -c config_file\n");
}

static short dead;
static void sigterm_handler(int sig)
{
    dead = 1;
}

static void sigchld_handler(int sig)
{
    /* noop: just breaks out of the poll() */
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

#define CFG_LISTEN_PORT "listen_port"
#define CFG_LISTEN_PORT_DEFAULT 7668

#define CFG_LISTEN_QDEPTH "listen_qdepth"
#define CFG_LISTEN_QDEPTH_DEFAULT 0

static cfg_opt_t options[] = {
    CFG_INT(CFG_LISTEN_PORT, CFG_LISTEN_PORT_DEFAULT, 0),
    CFG_INT(CFG_LISTEN_QDEPTH, CFG_LISTEN_QDEPTH_DEFAULT, 0),
    CFG_END()
};

static int listen_port;
static int listen_qdepth;
static int pdb_initialize(cfg_t * configuration)
{
    listen_port = cfg_getint(configuration, CFG_LISTEN_PORT);
    listen_qdepth = cfg_getint(configuration, CFG_LISTEN_QDEPTH);
    return 1;
}

static component *pdb_subcomponents[] = {
    SUBCOMPONENT(log),
    SUBCOMPONENT(server),
    SUBCOMPONENT_END()
};

static component pdb_component = {
    pdb_initialize,
    SHUTDOWN_NONE,
    options,
    pdb_subcomponents
};

int main(int argc, char **argv)
{
    char *configuration_filename = 0;

    char c;
    /* Flawfinder: ignore getopt */
    while ((c = getopt(argc, argv, "c:h")) != EOF) {
        switch (c) {
        case 'c':
            configuration_filename = optarg;
            break;
        case 'h':
        default:
            usage();
            exit(0);
            break;
        };
    }

    if (!configuration_filename) {
        usage();
        exit(1);
    }

    if (daemon_begin() == -1) {
        fprintf(stderr, "error in daemon_begin(): %s\n", strerror(errno));
        exit(1);
    }

    if (!component_configure(configuration_filename, &pdb_component)) {
        daemon_error("error configuring components\n");
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
        close(socket_fd);
        exit(1);
    }

    struct sockaddr_in bind_addr;
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(listen_port);
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_fd, (struct sockaddr *)&bind_addr,
             sizeof(bind_addr)) == -1) {
        daemon_error("can't bind socket: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }

    if (listen(socket_fd, listen_qdepth) == -1) {
        daemon_error("can't listen to socket: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }

    lo(LOG_DEBUG, "pdb: booting...");

    daemon_done();

    concurrency_setup();

    /* set up signal handling */
    signal(SIGTERM, sigterm_handler);
    signal(SIGCHLD, sigchld_handler);

    /* wait for connections; child processes handle each connection */
    dead = 0;
    lo(LOG_INFO, "pdb: entering main loop");
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
                                                          server);
                if (child == -1) {
                    lo(LOG_ERROR, "pdb: unable to handle connection: %s",
                       strerror(errno));
                    dead = 1;
                }
            }
        }
        concurrency_join_finished();
    }
    lo(LOG_INFO, "pdb: shutting down (waiting for children)...");

    concurrency_teardown();

    lo(LOG_INFO, "pdb: done.");

    close(socket_fd);
    component_unconfigure(&pdb_component);
    exit(0);
}
