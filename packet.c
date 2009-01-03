/* system includes */
#include <stdlib.h>
#include <string.h>

/* project includes */
#include "packet.h"

/**
 * Initialize a packet to "empty". Will leak memory if you pass a packet with
 * allocated bytes.
 *
 * @param p the packet to initialize.
 */
static void packet_initialize(packet * p)
{
    p->bytes = 0;
    p->allocated = 0;
    p->size = 0;
}

packet *packet_new(void)
{
    packet *p = malloc(sizeof(packet));
    if (!p) {
        return 0;
    }
    packet_initialize(p);
    return p;
}

packet *packet_copy(packet * p)
{
    packet *copy = packet_new();
    if (!copy) {
        return 0;
    }
    copy->bytes = malloc(p->size);
    if (!copy->bytes) {
        packet_delete(copy);
        return 0;
    }
    memcpy(copy->bytes, p->bytes, p->size);
    copy->allocated = p->size;
    copy->size = p->size;
    return copy;
}

/**
 * Free the memory associated with a given packet.
 *
 * @param p The packet to clean up.
 */
static void packet_free_bytes(packet * p)
{
    if (p->bytes) {
        free(p->bytes);
        packet_initialize(p);
    }
}

void packet_delete(packet * p)
{
    if (p) {
        packet_free_bytes(p);
        free(p);
    }
}

packet_set *packet_set_new(delegate_id count)
{
    packet_set *p = malloc(sizeof(packet_set));
    if (!p) {
        return 0;
    }
    p->count = 0;
    p->packets = malloc(sizeof(packet) * count);
    if (!p->packets) {
        packet_set_delete(p);
        return 0;
    }
    p->count = count;
    for (int i = 0; i < p->count; ++i) {
        packet_initialize(&p->packets[i]);
    }
    return p;
}

packet *packet_set_get(packet_set * p, delegate_id index)
{
    return &p->packets[index];
}

void packet_set_delete(packet_set * p)
{
    if (p) {
        if (p->count) {
            for (int i = 0; i < p->count; ++i) {
                packet_free_bytes(&p->packets[i]);
            }
            free(p->packets);
            p->count = 0;
        }
        free(p);
    }
}
