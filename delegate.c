/* system includes */
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
    short pending;
    int poll_index;
} delegate;

/* XXX: this is all runtime configuration... */
static delegate delegates[] = { {-1, 0, 3306, "127.0.0.1", 0, 0} };
static int delegate_count = 1;

int delegate_connect(void)
{
    for (int i = 0; i < delegate_count; ++i) {
        delegates[i].fd = socket(AF_INET, SOCK_STREAM, 0);
        if (delegates[i].fd == -1) {
            delegate_disconnect();
            return -1;
        }

        /* XXX: This should be parallelized via calls to fnctl (O_NONBLOCK) 
           and use of poll() */
        struct sockaddr_in connect_addr;
        connect_addr.sin_family = AF_INET;
        connect_addr.sin_port = delegates[i].port;
        inet_aton(delegates[i].ip, &(connect_addr.sin_addr));
        if (connect(delegates[i].fd, (struct sockaddr *)&connect_addr,
                    sizeof(connect_addr)) == -1) {
            delegate_disconnect();
            return -1;
        }

        delegates[i].connected = 1;
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
 * Parallel reading of a set of packets from a set of delegate servers.
 *
 * @param[in] get_packet function for reading a single packet
 * @return a list of replies gathered from delegate servers.
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

    int pending_replies = delegate_count;
    for (int i = 0; i < delegate_count; ++i) {
        delegates[i].pending = 1;
    }

    while (pending_replies > 0) {
        struct pollfd *reply_poll =
            malloc(sizeof(struct pollfd) * pending_replies);

        if (!reply_poll) {
            free(replies);
            return 0;
        }

        for (int i = 0, j = 0; i < delegate_count; ++i) {
            if (delegates[i].pending) {
                reply_poll[j].fd = delegates[i].fd;
                reply_poll[j].events = POLLIN;
                reply_poll[j].revents = 0;
                delegates[i].poll_index = j;
                ++j;
            }
        }

        if (poll(reply_poll, pending_replies, -1) <= 0) {
            free(replies);
            free(reply_poll);
            return 0;
        }

        for (int i = 0; i < delegate_count; ++i) {
            if (delegates[i].pending) {
                if (reply_poll[delegates[i].poll_index].revents & POLLIN) {
                    switch (get_packet(delegates[i].fd, &replies[i])) {
                    case PACKET_ERROR:
                        free(replies);
                        free(reply_poll);
                        return 0;
                    case PACKET_INCOMPLETE:
                        break;
                    case PACKET_COMPLETE:
                        --pending_replies;
                        delegates[i].pending = 0;
                        break;
                    };
                }
            }
        }

        free(reply_poll);
    }

    return replies;
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

    int pending_writes = delegate_count;
    for (int i = 0; i < delegate_count; ++i) {
        sent_list[i] = 0;
        delegates[i].pending = 1;
    }

    while (pending_writes > 0) {
        struct pollfd *proxy_poll =
            malloc(sizeof(struct pollfd) * pending_writes);

        if (!proxy_poll) {
            free(sent_list);
            return 0;
        }

        for (int i = 0, j = 0; i < delegate_count; ++i) {
            if (delegates[i].pending) {
                proxy_poll[j].fd = delegates[i].fd;
                proxy_poll[j].events = POLLOUT;
                proxy_poll[j].revents = 0;
                delegates[i].poll_index = j;
                ++j;
            }
        }

        if (poll(proxy_poll, pending_writes, -1) <= 0) {
            free(sent_list);
            free(proxy_poll);
            return 0;
        }

        for (int i = 0; i < delegate_count; ++i) {
            if (delegates[i].pending) {
                if (proxy_poll[delegates[i].poll_index].revents & POLLOUT) {
                    switch (put_packet(delegates[i].fd, command,
                                       &sent_list[i])) {
                    case PACKET_ERROR:
                        free(sent_list);
                        free(proxy_poll);
                        return 0;
                    case PACKET_INCOMPLETE:
                        break;
                    case PACKET_COMPLETE:
                        --pending_writes;
                        delegates[i].pending = 0;
                        break;
                    };
                }
            }
        }

        free(proxy_poll);
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
