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
#include <netdb.h>
#include <string.h>

/* project includes */
#include "delegate.h"
#include "log.h"
#include "packet.h"

typedef struct {
    int partition_id;
    int fd;
    short connected;
    int port;
    struct in_addr ip;
    char *name;
    void *driver_state;
} delegate;

#define MASTER_PARTITION_ID -1
#define MASTER_PARTITION "master"

#define CFG_DELEGATE "delegate"

#define CFG_PARTITION_ID "partition_id"
#define CFG_PARTITION_ID_DEFAULT MASTER_PARTITION_ID

#define CFG_HOSTNAME "hostname"
#define CFG_HOSTNAME_DEFAULT 0

#define CFG_PORT "port"
#define CFG_PORT_DEFAULT 3306

#define CFG_NAME "name"
#define CFG_NAME_DEFAULT "pdb"

static delegate *delegates = 0;
static int delegate_count = 0;

/**
 * Component initialization for the delegate component.
 *
 * @param[in] configuration The current configuration.
 * @return 1 on success, 0 on failure
 */
static int delegate_initialize(cfg_t * configuration)
{
    delegate_count = cfg_size(configuration, CFG_DELEGATE);
    delegates = malloc(sizeof(delegate) * delegate_count);
    if (!delegates) {
        delegate_count = 0;
        return 0;
    }

    for (delegate_id i = 0; i < delegate_count; ++i) {
        cfg_t *delegate_config = cfg_getnsec(configuration, CFG_DELEGATE, i);

        delegates[i].partition_id = cfg_getint(delegate_config,
                                               CFG_PARTITION_ID);
        delegates[i].fd = -1;
        delegates[i].connected = 0;
        delegates[i].port = cfg_getint(delegate_config, CFG_PORT);
        delegates[i].ip.s_addr = cfg_getint(delegate_config, CFG_HOSTNAME);
        delegates[i].name = strdup(cfg_getstr(delegate_config, CFG_NAME));
        if (!delegates[i].name) {
            return 0;
        }
        lo(LOG_DEBUG, "delegate_initialize: %s(%d) at %d:%d",
           delegates[i].name, delegates[i].partition_id,
           delegates[i].ip.s_addr, delegates[i].port);
    }
    return 1;
}

delegate_id delegate_max(void)
{
    return delegate_count - 1;
}

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
 * @param[in] event the set of poll(2) events which this worker cares about
 * @param[in] worker the worker function
 * @param[in,out] worker_args extra arguments to the worker function
 * @return 0 on failure, 1 on success
 */
static int delegate_io(short event,
                       packet_status(*worker) (delegate_id, void *),
                       void *worker_args, delegate_filter filter)
{
    delegate_io_info *info =
        malloc(sizeof(delegate_io_info) * delegate_count);
    if (!info) {
        return 0;
    }

    int pending = 0;
    for (delegate_id i = 0; i < delegate_count; ++i) {
        if ((filter == NULL) || (!filter(i))) {
            ++pending;
            info[i].pending = 1;
            info[i].poll_index = -1;
        } else {
            info[i].pending = 0;
        }
    }

    while (pending > 0) {
        struct pollfd *pending_poll = malloc(sizeof(struct pollfd) * pending);
        if (!pending_poll) {
            free(info);
            return 0;
        }

        for (delegate_id i = 0, j = 0; i < delegate_count; ++i) {
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

        for (delegate_id i = 0; i < delegate_count; ++i) {
            if (info[i].pending) {
                if (pending_poll[info[i].poll_index].revents & event) {
                    switch (worker(i, worker_args)) {
                    case PACKET_ERROR:
                    case PACKET_EOF:
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
static packet_status delegate_connect_worker(delegate_id delegate_index,
                                             void *void_args)
{
    /* The connect() has already been issued; this function will only be
       called when the connection completes (according to poll()) */
    delegates[delegate_index].connected = 1;
    lo(LOG_DEBUG, "delegate_connect_worker: connection to %s:%d completed",
       inet_ntoa(delegates[delegate_index].ip),
       delegates[delegate_index].port);
    return PACKET_COMPLETE;
}

int delegate_connect(void)
{
    if (delegate_count == 0) {
        errno = EINVAL;
        return -1;
    }

    for (delegate_id i = 0; i < delegate_count; ++i) {
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
        connect_addr.sin_port = htons(delegates[i].port);
        connect_addr.sin_addr = delegates[i].ip;

        lo(LOG_DEBUG, "delegate_connect: starting connection to %s:%d",
           inet_ntoa(delegates[i].ip), delegates[i].port);

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
    if (!delegate_io(POLLOUT, delegate_connect_worker, 0, NULL)) {
        delegate_disconnect();
        return -1;
    }

    return 0;
}

void delegate_disconnect(void)
{
    for (delegate_id i = 0; i < delegate_count; ++i) {
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
    packet_set *replies;
    packet_reader get_packet;
    void (*register_reply) (delegate_id, packet *);
} gather_replies_worker_args;

/**
 * I/O 'worker' function for reading replies from a delegate.
 *
 * @param delgate_index index of the current delegate
 * @param void_args arguments as a void pointer
 * @return status of the operation
 */
static packet_status gather_replies_worker(delegate_id delegate_index,
                                           void *void_args)
{
    gather_replies_worker_args *args =
        (gather_replies_worker_args *) void_args;
    packet_status s = args->get_packet(delegates[delegate_index].fd,
                                       packet_set_get(args->replies,
                                                      delegate_index));
    if (s == PACKET_COMPLETE) {
        args->register_reply(delegate_index,
                             packet_set_get(args->replies, delegate_index));
    }
    return s;
}

packet_set *delegate_get(delegate_filter filter,
                         packet_reader get_packet,
                         void (*register_reply) (delegate_id, packet *))
{
    packet_set *replies = packet_set_new(delegate_count);
    if (!replies) {
        return 0;
    }

    /* read from all delegates in parallel */
    gather_replies_worker_args args;
    args.replies = replies;
    args.get_packet = get_packet;
    args.register_reply = register_reply;
    if (delegate_io(POLLIN, gather_replies_worker, &args, filter) == 0) {
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
    packet_set *commands;
} proxy_command_worker_args;

/**
 * I/O 'worker' function for writing command to a delegate.
 *
 * @param delegate_index index of the current delegate
 * @param void_args arguments, cast as a void pointer
 * @return status of the current write
 */
static packet_status proxy_command_worker(delegate_id delegate_index,
                                          void *void_args)
{
    proxy_command_worker_args *args = (proxy_command_worker_args *) void_args;
    return args->put_packet(delegates[delegate_index].fd,
                            packet_set_get(args->commands, delegate_index),
                            &(args->sent_list[delegate_index]));
}

int delegate_put(packet_writer put_packet,
                 int (*rewrite_command) (packet *, packet *, const char *),
                 packet * command)
{
    int *sent_list = malloc(sizeof(int) * delegate_count);
    if (!sent_list) {
        return 0;
    }

    packet_set *commands = packet_set_new(delegate_count);
    if (!commands) {
        free(sent_list);
        return 0;
    }

    for (delegate_id i = 0; i < delegate_count; ++i) {
        if (!rewrite_command(command, packet_set_get(commands, i),
                             delegates[i].name)) {
            packet_set_delete(commands);
            free(sent_list);
            return 0;
        }
        sent_list[i] = 0;
    }

    /* write to all delegates in parallel */
    proxy_command_worker_args args;
    args.sent_list = sent_list;
    args.put_packet = put_packet;
    args.commands = commands;
    if (!delegate_io(POLLOUT, proxy_command_worker, &args, NULL)) {
        packet_set_delete(commands);
        free(sent_list);
        return 0;
    }

    packet_set_delete(commands);
    free(sent_list);
    return 1;
}

static void delegate_shutdown(void)
{
    if (delegates) {
        for (delegate_id i = 0; i < delegate_count; ++i) {
            if (delegates[i].name) {
                free(delegates[i].name);
                delegates[i].name = 0;
            }
        }
        free(delegates);
        delegates = 0;
        delegate_count = 0;
    }
}

static int hostname_parser(cfg_t * cfg, cfg_opt_t * opt, const char *value,
                           void *result)
{
    struct hostent *he = gethostbyname(value);
    if (!he) {
        return 1;
    }

    *(int *)result = ((struct in_addr *)he->h_addr)->s_addr;
    return 0;
}

static int partition_id_parser(cfg_t * cfg, cfg_opt_t * opt,
                               const char *value, void *result)
{
    if (strncasecmp(value, MASTER_PARTITION, strlen(MASTER_PARTITION)) == 0) {
        *(int *)result = MASTER_PARTITION_ID;
    } else {
        *(int *)result = atoi(value);
    }
    return 0;
}

static cfg_opt_t delegate_options[] = {
    CFG_INT_CB(CFG_HOSTNAME, CFG_HOSTNAME_DEFAULT, 0, hostname_parser),
    CFG_INT_CB(CFG_PARTITION_ID, CFG_PARTITION_ID_DEFAULT, 0,
               partition_id_parser),
    CFG_INT(CFG_PORT, CFG_PORT_DEFAULT, 0),
    CFG_STR(CFG_NAME, CFG_NAME_DEFAULT, 0),
    CFG_END()
};

static cfg_opt_t options[] = {
    CFG_SEC(CFG_DELEGATE, delegate_options, CFGF_TITLE | CFGF_MULTI),
    CFG_END()
};

component delegate_component = {
    delegate_initialize,
    delegate_shutdown,
    options,
    SUBCOMPONENTS_NONE
};
