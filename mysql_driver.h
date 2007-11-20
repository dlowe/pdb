#ifndef _MYSQL_DRIVER_H
#define _MYSQL_DRIVER_H

/** @file mysql_driver.h
 * @brief MySQL database driver.
 *
 * Implements the db_driver interface for mysql.
 */

#include "command.h"
#include "action.h"
#include "reply.h"

/**
 * Is the current connection ready to close?
 *
 * @return 1 if it is time to shut down this connection; 0 otherwise.
 */
short mysql_driver_done(void);

/**
 * Read the next command from a file descriptor.
 *
 * @param[in] fd a connected file descriptor from which to read command.
 * @return the next command read from the input.
 */
command mysql_driver_get_next_command(int fd);

/**
 * Read the next reply from a file descriptor.
 *
 * @param[in] fd a connected file descriptor from which to read reply.
 * @param[in,out] r a reply buffer
 * @return the status of the read
 */
reply_status mysql_driver_get_next_reply(int fd, reply *r);

/**
 * Map a command into a list of actions.
 *
 * @param[in] in_command the command from which to determine an action.
 * @return a list of actions
 */
action* mysql_driver_actions_from(command in_command);

/**
 * Reduce a set of replies into a single reply.
 * 
 * @param[in] replies a list of replies from all of the delegates.
 * @return the reduced reply.
 */
reply mysql_driver_reduce_replies(reply* replies);

/**
 * Send a reply to a file desciptor.
 *
 * @param[in] fd a connected file descriptor to which to write reply.
 * @param[in] in_reply a reply to send.
 */
void mysql_driver_send_reply(int fd, reply in_reply);

#endif
