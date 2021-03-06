#ifndef __DELEGATE_H
#define __DELEGATE_H

/**
 * @file delegate.h
 * @brief Communication with delegate databases.
 * 
 * This API hides the details of managing the pool of delegate database
 * connections.
 *
 * The delegate component should be exclusively used by the server component.
 */

#include "packet.h"
#include "component.h"
#include "delegate_filter.h"

/** @cond */
DECLARE_COMPONENT(delegate);
/** @endcond */

/**
 * Connect to all delegates.
 *
 * @return 0 on success, -1 on failure (and errno will be set)
 */
int delegate_connect(void);

/**
 * Get delegate count.
 *
 * @return the number of delegates.
 */
delegate_id delegate_get_count(void);

/**
 * Get master delegate_id.
 *
 * @return the master delegate_id.
 */
delegate_id delegate_master_id(void);

/**
 * Parallel read of a set of packets from a set of delegate servers.
 *
 * @param[in] filters set of filters to apply to the list of delegates
 * @param[in] get_packet function for reading a single packet
 * @return a list of replies gathered from delegate servers; the caller is
 * responsible for freeing this list!
 */
packet_set *delegate_get(delegate_filter * filters, packet_reader get_packet);

/**
 * Parallel write of a packet to a set of delegate servers.
 *
 * @param[in] filters set of filters to apply to the list of delegates
 * @param[in] put_packet function for writing a single packet.
 * @param[in] rewrite_command function for per-delegate command rewriting.
 * @param[in] command the packet to write to all delegates.
 * @return 1 on success, 0 on failure
 */
int delegate_put(delegate_filter * filters, packet_writer put_packet,
                 int (*rewrite_command) (packet *, packet *, const char *),
                 packet * command);

/**
 * Disconnect from all delegates.
 */
void delegate_disconnect(void);

#endif
