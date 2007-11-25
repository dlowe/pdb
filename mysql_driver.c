/* system includes */
#include <sys/types.h>
#include <sys/uio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

/* project includes */
#include "log.h"
#include "mysql_driver.h"

#define HEADER_SIZE 4

static short done = 0;

short mysql_driver_done(void)
{
    return done;
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
            return PACKET_ERROR;
        }
        p->size += len;
        return PACKET_INCOMPLETE;
    }

    /* reading the body */
    long packet_length =
        ((unsigned char)p->bytes[0]) +
        ((unsigned char)p->bytes[1] << 8) +
        ((unsigned char)p->bytes[2] << 16);

    lo(LOG_DEBUG, "read header for packet type %c of length %ld",
       p->bytes[3], packet_length);

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
        return PACKET_ERROR;
    }

    p->size += len;

    if (p->size < (packet_length + HEADER_SIZE)) {
        return PACKET_INCOMPLETE;
    }
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
        return PACKET_INCOMPLETE;
    }
    return PACKET_COMPLETE;
}

action mysql_driver_actions_from(packet * in_command)
{
    return ACTION_PROXY_ALL;
}

packet *mysql_driver_reduce_replies(packet_set * replies)
{
    return packet_copy(packet_set_get(replies, 0));
}
