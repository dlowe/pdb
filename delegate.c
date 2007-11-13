/* system includes */
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

/* project includes */
#include "action.h"
#include "command.h"
#include "delegate.h"
#include "reply.h"

typedef struct {
    int fd;
    short connected;
    int port;
    char *ip;
} delegate;

/* XXX: this is all runtime configuration... */
static delegate delegates[] = { {-1, 0, 3306, "127.0.0.1"} };
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
 * @return a list of replies gathered from delegate servers.
 */
static reply *gather_replies(void)
{
    reply *replies = malloc(sizeof(reply) * delegate_count);
    return replies;
}

reply *delegate_action(action what, command with)
{
    switch (what) {
    case ACTION_NONE:
        return 0;
    case ACTION_NOOP_ALL:
        return gather_replies();
    };
    return 0;
}

void delegate_action_cleanup(reply * replies)
{
    if (replies) {
        free(replies);
    }
}
