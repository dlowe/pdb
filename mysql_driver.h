#ifndef __MYSQL_DRIVER_H
#define __MYSQL_DRIVER_H

/**
 * @file mysql_driver.h
 * @brief MySQL database driver.
 *
 * Implements the db_driver interface for mysql.
 */

#include "db_driver.h"
#include "packet.h"
#include "delegate_id.h"

short mysql_driver_initialize(delegate_id max_delegate_id);

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

short mysql_driver_got_error(void);

packet *mysql_driver_error_packet(void);

short mysql_driver_delegate_filter(delegate_id id);

/**
 * Read the next packet from a file descriptor. This function is intended
 * to be called repeatedly until the packet is fully read.
 *
 * @param[in] fd a connected file descriptor from which to read packet.
 * @param[in,out] p a packet buffer
 * @return the status of the read
 */
packet_status mysql_driver_get_packet(int fd, packet * p);

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
packet_status mysql_driver_put_packet(int fd, packet * p, int *sent);

/**
 * Note the receipt of a command.
 *
 * @param[in] in_command the command received.
 * @return high level command "type"
 */
db_driver_command_type mysql_driver_command(packet * in_command);

void mysql_driver_reply(delegate_id id, packet * in_reply);

/**
 * Reduce a set of replies into a single packet.
 * 
 * @param[in] replies a list of replies from all of the delegates.
 * @return the reduced packet.
 */
packet *mysql_driver_reduce_replies(packet_set * replies);

/**
 * Rewrite a command for a specific delegate.
 *
 * @param[in] in the original command packet
 * @param[in,out] out the rewritten packet
 * @param[in] db_name the name of the delegate database.
 * @return 1 on success, 0 on failure
 */
int mysql_driver_rewrite_command(packet * in, packet * out,
                                 const char *db_name);

char *mysql_driver_sql_extract(packet * in);

#endif
