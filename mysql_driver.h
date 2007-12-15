#ifndef __MYSQL_DRIVER_H
#define __MYSQL_DRIVER_H

/** @file mysql_driver.h
 * @brief MySQL database driver.
 *
 * Implements the db_driver interface for mysql.
 */

#include "packet.h"

void mysql_driver_initialize(void);

/**
 * Is the current connection ready to close?
 *
 * @return 1 if it is time to shut down this connection; 0 otherwise.
 */
short mysql_driver_done(void);

/**
 * Expecting a new command?
 *
 * @return 1 if we're expecting a command; 0 otherwise.
 */
short mysql_driver_expect_commands(void);

/**
 * Expecting a reply?
 *
 * @return 1 if we're expecting a reply; 0 otherwise.
 */
short mysql_driver_expect_replies(void);

/**
 * Read the next packet from a file descriptor. This function is intended
 * to be called repeatedly until the packet is fully read.
 *
 * @param[in] fd a connected file descriptor from which to read packet.
 * @param[in,out] p a packet buffer
 * @return the status of the read
 */
packet_status mysql_driver_get_packet(int fd, packet *p);

/**
 * Write a packet to a file descriptor. This function is intended to be called
 * repeatedly until the packet is fully written.
 *
 * @param[in] fd a connected file descriptor to which to write packet.
 * @param[in] p a packet buffer
 * @param[in,out] sent the number of bytes already sent. The caller should set
 * this to 0 for the first call on a given packet.
 * @return the status of the write
 */
packet_status mysql_driver_put_packet(int fd, packet *p, int *sent);

/**
 * Note the receipt of a command.
 *
 * @param[in] in_command the command received.
 */
void mysql_driver_got_command(packet *in_command);

/**
 * Reduce a set of replies into a single packet.
 * 
 * @param[in] replies a list of replies from all of the delegates.
 * @return the reduced packet.
 */
packet *mysql_driver_reduce_replies(packet_set * replies);

#endif
