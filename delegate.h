#ifndef __DELEGATE_H
#define __DELEGATE_H

/**
 * @file delegate.h
 * @brief Communication with delegate databases.
 * 
 * This API hides the details of managing the pool of delegate database
 * connections.
 */

#include "packet.h"
#include "component.h"

extern component delegate_component;

/**
 * Connect to all delegates.
 *
 * @return 0 on success, -1 on failure (and errno will be set)
 */
int delegate_connect(void);

/**
 * Parallel read of a set of packets from a set of delegate servers.
 *
 * @param[in] get_packet function for reading a single packet
 * @return a list of replies gathered from delegate servers; the caller is
 * responsible for freeing this list!
 */
packet_set* delegate_get(packet_reader get_packet);

/**
 * Parallel write of a packet to a set of delegate servers.
 *
 * @param[in] put_packet function for writing a single packet.
 * @param[in] command the packet to write to all delegates.
 * @return 1 on success, 0 on failure
 */
int delegate_put(packet_writer put_packet, packet *command);

/**
 * Disconnect from all delegates.
 */
void delegate_disconnect(void);

#endif
