#ifndef _PACKET_H
#define _PACKET_H

typedef struct {
    char *bytes;
    int allocated;
    int size;
} packet;

typedef enum {
    PACKET_ERROR,
    PACKET_INCOMPLETE,
    PACKET_COMPLETE
} packet_status;

typedef packet_status (*packet_reader)(int, packet *);
typedef packet_status (*packet_writer)(int, packet *, int *);

packet *packet_null(void);

#endif
