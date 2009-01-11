#ifndef __SERVER_H
#define __SERVER_H

/**
 * @file server.h
 * @brief Generic server masquerading.
 *
 * This API implements the basic network-level db-server masquerading logic,
 * i.e. the function server() which handles a single connection from beginning
 * to end.
 *
 * The server component should be exclusively used by the main component.
 *
 * The server component is aware of the 'delegate' and 'db_driver' components,
 * and is responsible for the relationship between those two components.
 */

#include <sys/types.h>
#include <netinet/in.h>

#include "component.h"

/** @cond */
DECLARE_COMPONENT(server);
/** @endcond */

/**
 * Top-level sequencing of a single connection.
 *
 * @param[in] fd connected file descriptor.
 * @param[in] addr information about the connection.
 */
void server(int fd, struct sockaddr_in *addr);

#endif
