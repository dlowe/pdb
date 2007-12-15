#ifndef __DELEGATE_H
#define __DELEGATE_H

/**
 * @file delegate.h
 * @brief Communication with delegate databases.
 * 
 * This API hides the details of managing the pool of delegate database
 * connections.
 */

#include "action.h"
#include "packet.h"

/**
 * Connect to all delegates.
 *
 * @return 0 on success, -1 on failure (and errno will be set)
 */
int delegate_connect(void);

/**
 * Parallel reading of a set of packets from a set of delegate servers.
 *
 * @param[in] get_packet function for reading a single packet
 * @return a list of replies gathered from delegate servers; the caller is
 * responsible for freeing this list!
 */
packet_set* delegate_get(packet_reader get_packet);

/**
 * Parallel writing of a packet to a set of delegate servers.
 *
 * @param[in] a action to perform with this packet
 * @param[in] command the packet to write to all delegates.
 * @param[in] put_packet function for writing a single packet.
 * @return 1 on success, 0 on failure
 */
int delegate_put(action a, packet *command, packet_writer put_packet);

/**
 * Disconnect from all delegates.
 */
void delegate_disconnect(void);

#endif
