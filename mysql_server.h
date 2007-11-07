#ifndef _MYSQL_SERVER_H
#define _MYSQL_SERVER_H

/** @file mysql_server.h
 * @brief MySQL server-side network protocol.
 *
 * Implements the mysql network protocol's server side.
 */

#include <netinet/in.h>

/**
 * Top-level interface to the mysql server.
 *
 * @param[in] fd connected socket
 * @param[in] addr information about the connection
 */
void mysql_server(int fd, struct sockaddr_in *addr);

#endif
