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
#include "command.h"
#include "reply.h"

/**
 * Connect to all delegates.
 *
 * @return 0 on success, -1 on failure (and errno will be set)
 */
int delegate_connect(void);

reply* delegate_action(action what, command with);

/**
 * Clean up after an action.
 *
 * @param[in,out] replies pointer to a list of replies (will be freed!)
 */
void delegate_action_cleanup(reply *replies);

/**
 * Disconnect from all delegates.
 */
void delegate_disconnect(void);

#endif
