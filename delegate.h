#ifndef _DELEGATE_H
#define _DELEGATE_H

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
 *
 * @param[in] what what action to delegate
 * @param[in] with what command is being delegated
 * @param[in] put_packet driver function for writing commands
 * @param[in] get_packet driver function for reading replies
 * @return list of replies from all delegates
 */
packet* delegate_action(action what, packet *with, packet_writer put_packet,
                        packet_reader get_packet);

/**
 * Clean up after an action.
 *
 * @param[in,out] replies pointer to a list of replies (will be freed!)
 */
void delegate_action_cleanup(packet *replies);

/**
 * Disconnect from all delegates.
 */
void delegate_disconnect(void);

#endif
