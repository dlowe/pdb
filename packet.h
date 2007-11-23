#ifndef __PACKET_H
#define __PACKET_H

typedef struct {
    char *bytes;
    int allocated;
    int size;
} packet;

typedef struct {
    packet *packets;
    int count;
} packet_set;

typedef enum {
    PACKET_ERROR,
    PACKET_INCOMPLETE,
    PACKET_COMPLETE
} packet_status;

typedef packet_status (*packet_reader)(int, packet *);
typedef packet_status (*packet_writer)(int, packet *, int *);

packet *packet_NULL(void);

packet *packet_new();
packet *packet_copy(packet *p);
void packet_delete(packet *p);

packet_set *packet_set_new(int count);
packet *packet_set_get(packet_set *p, int index);
void packet_set_delete(packet_set *p);

#endif
