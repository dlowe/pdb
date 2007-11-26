#ifndef __PACKET_H
#define __PACKET_H

/**
 * A single "packet" of communication.
 */
typedef struct {
    char *bytes;
    int allocated;
    int size;
} packet;

/**
 * A set of packets received by delegates.
 */
typedef struct {
    packet *packets;
    int count;
} packet_set;

/**
 * Possible statuses for a non-blocking I/O operation.
 * XXX: may not be quite the right level of abstraction for this...
 */
typedef enum {
    PACKET_ERROR,
    PACKET_INCOMPLETE,
    PACKET_COMPLETE
} packet_status;

/**
 * packet reader function type.
 */
typedef packet_status (*packet_reader)(int, packet *);

/**
 * packet writer function type.
 */
typedef packet_status (*packet_writer)(int, packet *, int *);

/**
 * NULL packet object
 *
 * @return a static pointer to a NULL packet
 */
packet *packet_NULL(void);

packet *packet_new();
packet *packet_copy(packet *p);
void packet_delete(packet *p);

packet_set *packet_set_new(int count);
packet *packet_set_get(packet_set *p, int index);
void packet_set_delete(packet_set *p);

#endif
