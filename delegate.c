/* system includes */
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

/* project includes */
#include "action.h"
#include "delegate.h"
#include "packet.h"

typedef struct {
    int fd;
    short connected;
    int port;
    char *ip;
} delegate;

/* XXX: this is all runtime configuration... */
static delegate delegates[] = { {-1, 0, 3306, "127.0.0.1"} };
static int delegate_count = 1;


/**
 * Per-delegate information used by delegate_io when multiplexing I/O work
 * across the set of delegates.
 */
typedef struct {
    short pending;
    int poll_index;
} delegate_io_info;

/**
 * This higher-order function orchestrates I/O work across the set of delegates
 * using a worker function to perform actual I/O.
 * 
 * @param event the set of poll(2) events which this worker cares about
 * @param worker the worker function
 * @param worker_args extra arguments to the worker function
 * @return 0 on failure, 1 on success
 */
static int delegate_io(short event, packet_status(*worker) (int, void *),
                       void *worker_args)
{
    delegate_io_info *info =
        malloc(sizeof(delegate_io_info) * delegate_count);
    if (!info) {
        return 0;
    }

    int pending = delegate_count;
    for (int i = 0; i < delegate_count; ++i) {
        info[i].pending = 1;
        info[i].poll_index = -1;
    }

    while (pending > 0) {
        struct pollfd *pending_poll = malloc(sizeof(struct pollfd) * pending);
        if (!pending_poll) {
            free(info);
            return 0;
        }

        for (int i = 0, j = 0; i < delegate_count; ++i) {
            if (info[i].pending) {
                pending_poll[j].fd = delegates[i].fd;
                pending_poll[j].events = event;
                pending_poll[j].revents = 0;
                info[i].poll_index = j;
                ++j;
            }
        }

        if (poll(pending_poll, pending, -1) <= 0) {
            free(pending_poll);
            free(info);
            return 0;
        }

        for (int i = 0; i < delegate_count; ++i) {
            if (info[i].pending) {
                if (pending_poll[info[i].poll_index].revents & event) {
                    switch (worker(i, worker_args)) {
                    case PACKET_ERROR:
                        free(pending_poll);
                        free(info);
                        return 0;
                    case PACKET_INCOMPLETE:
                        break;
                    case PACKET_COMPLETE:
                        --pending;
                        info[i].pending = 0;
                        break;
                    };
                }
            }
        }

        free(pending_poll);
    }

    free(info);
    return 1;
}

/**
 * I/O 'worker' function for asynchronously connecting to a delegate
 *
 * @param delegate_index index of the current delegate
 * @param void_args unused
 * @return PACKET_COMPLETE
 */
static packet_status delegate_connect_worker(int delegate_index,
                                             void *void_args)
{
    /* The connect() has already been issued; this function will only be
       called when the connection completes (according to poll()) */
    delegates[delegate_index].connected = 1;
    return PACKET_COMPLETE;
}

int delegate_connect(void)
{
    for (int i = 0; i < delegate_count; ++i) {
        delegates[i].fd = socket(AF_INET, SOCK_STREAM, 0);
        if (delegates[i].fd == -1) {
            delegate_disconnect();
            return -1;
        }

        /* set the socket to non-blocking */
        if (fcntl(delegates[i].fd, F_SETFL, O_NONBLOCK) == -1) {
            delegate_disconnect();
            return -1;
        }

        struct sockaddr_in connect_addr;

        connect_addr.sin_family = AF_INET;
        connect_addr.sin_port = delegates[i].port;
        inet_aton(delegates[i].ip, &(connect_addr.sin_addr));

        if (connect(delegates[i].fd, (struct sockaddr *)&connect_addr,
                    sizeof(connect_addr)) == -1) {
            if (errno != EINPROGRESS) {
                delegate_disconnect();
                return -1;
            }
        } else {
            delegates[i].connected = 1;
        }
    }

    /* Block until all delegates are connected */
    if (!delegate_io(POLLOUT, delegate_connect_worker, 0)) {
        delegate_disconnect();
        return -1;
    }

    return 0;
}

void delegate_disconnect(void)
{
    for (int i = 0; i < delegate_count; ++i) {
        if (delegates[i].fd > 0) {
            if (delegates[i].connected) {
                shutdown(delegates[i].fd, SHUT_RDWR);
                delegates[i].connected = 0;
            }
            close(delegates[i].fd);
            delegates[i].fd = -1;
        }
    }
}

/**
 * Arguments to gather_replies_worker
 */
typedef struct {
    packet *replies;
    packet_reader get_packet;
} gather_replies_worker_args;

/**
 * I/O 'worker' function for reading replies from a delegate.
 *
 * @param delgate_index index of the current delegate
 * @param void_args arguments as a void pointer
 * @return status of the operation
 */
static packet_status gather_replies_worker(int delegate_index,
                                           void *void_args)
{
    gather_replies_worker_args *args =
        (gather_replies_worker_args *) void_args;
    return args->get_packet(delegates[delegate_index].fd,
                            &(args->replies[delegate_index]));
}

/**
 * Parallel reading of a set of packets from a set of delegate servers.
 *
 * @param[in] get_packet function for reading a single packet
 * @return a list of replies gathered from delegate servers; the caller is
 * responsible for freeing this list!
 */
static packet *gather_replies(packet_reader get_packet)
{
    packet *replies = malloc(sizeof(packet) * delegate_count);
    if (!replies) {
        return 0;
    }

    for (int i = 0; i < delegate_count; ++i) {
        replies[i].bytes = 0;
    }

    /* read from all delegates in parallel */
    gather_replies_worker_args args;
    args.replies = replies;
    args.get_packet = get_packet;
    if (delegate_io(POLLIN, gather_replies_worker, &args) == 0) {
        return 0;
    }

    return replies;
}

/**
 * Arguments to proxy_command_worker.
 */
typedef struct {
    int *sent_list;
    packet_writer put_packet;
    packet *command;
} proxy_command_worker_args;

/**
 * I/O 'worker' function for writing command to a delegate.
 *
 * @param delegate_index index of the current delegate
 * @param void_args arguments, cast as a void pointer
 * @return status of the current write
 */
static packet_status proxy_command_worker(int delegate_index, void *void_args)
{
    proxy_command_worker_args *args = (proxy_command_worker_args *) void_args;
    return args->put_packet(delegates[delegate_index].fd, args->command,
                            &(args->sent_list[delegate_index]));
}

/**
 * Parallel writing of a packet to a set of delegate servers.
 *
 * @param[in] command the packet to write to all delegates.
 * @param[in] put_packet function for writing a single packet.
 * @return 1 on success, 0 on failure
 */
static int proxy_command(packet * command, packet_writer put_packet)
{
    int *sent_list = malloc(sizeof(int) * delegate_count);
    if (!sent_list) {
        return 0;
    }

    for (int i = 0; i < delegate_count; ++i) {
        sent_list[i] = 0;
    }

    /* write to all delegates in parallel */
    proxy_command_worker_args args;
    args.sent_list = sent_list;
    args.put_packet = put_packet;
    args.command = command;
    if (!delegate_io(POLLOUT, proxy_command_worker, &args)) {
        free(sent_list);
        return 0;
    }

    free(sent_list);
    return 1;
}

packet *delegate_action(action what, packet * command,
                        packet_writer put_packet, packet_reader get_packet)
{
    switch (what) {
    case ACTION_NONE:
        return 0;
    case ACTION_NOOP_ALL:
        return gather_replies(get_packet);
    case ACTION_PROXY_ALL:
        if (!proxy_command(command, put_packet)) {
            return 0;
        }
        return gather_replies(get_packet);
    };
    return 0;
}

void delegate_action_cleanup(packet * replies)
{
    if (replies) {
        free(replies);
    }
}
