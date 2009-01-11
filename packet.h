#ifndef __PACKET_H
#define __PACKET_H

#include "delegate_filter.h"

/**
 * @file packet.h
 * @brief functions for working with logical communication packets
 *
 * A 'packet' in pdb is the unit of communication. These functions are for
 * dealing with packets in the abstract.
 */

/**
 * A single "packet" of communication.
 */
typedef struct {
    char *bytes;   /**< actual contents of the packet */
    int allocated; /**< current allocated byte count */
    int size;      /**< current used byte count */
} packet;

/**
 * A set of packets received by delegates.
 */
typedef struct {
    packet *packets; /**< list of packets in the set */
    int count;       /**< count of packets in the set */
} packet_set;

/**
 * Possible statuses for a non-blocking I/O operation.
 */
typedef enum {
    PACKET_ERROR,
    PACKET_INCOMPLETE,
    PACKET_COMPLETE,
    PACKET_EOF
} packet_status;

/**
 * packet reader function type.
 */
typedef packet_status(*packet_reader) (int, packet *);

/**
 * packet writer function type.
 */
typedef packet_status(*packet_writer) (int, packet *, int *);

/**
 * Allocate a new packet object.
 *
 * @return freshly allocated packet, initialized to empty
 */
packet *packet_new();

/**
 * Create a copy of a packet.
 *
 * @param[in] p original packet
 * @return freshly allocated packet, initialized to be identical to original
 */
packet *packet_copy(packet * p);

/**
 * Delete a packet
 *
 * @param[in,out] p a packet
 */
void packet_delete(packet * p);

/**
 * Allocate a new packet set.
 *
 * @param[in] delegate_count the number of packets to be contained in the set.
 * @return freshly allocated packet set of 'count' empty packets
 */
packet_set *packet_set_new(delegate_id delegate_count);

/**
 * Fetch a packet out of a set.
 *
 * @param[in] p a packet set
 * @param[in] index the index of the packet to fetch
 * @return pointer to the packet
 */
packet *packet_set_get(packet_set * p, delegate_id index);

/**
 * Delete a packet set (includes deleting all contained packets!)
 *
 * @param[in,out] p a packet set
 */
void packet_set_delete(packet_set * p);

#endif
