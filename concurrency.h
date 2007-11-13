#ifndef __CONCURRENCY_H
#define __CONCURRENCY_H

/**
 * @file concurrency.h
 * @brief Concurrently handle network connections.
 *
 * A simple API which (hopefully) keeps the nuts and bolts of managing a bunch
 * of concurrent network connections abstract enough that the implementation
 * can be varied in the future if need be.
 */

#include <netinet/in.h>

/**
 * Set up concurrency strategy
 */
void concurrency_setup(void);

/**
 * Clean up concurrent work (joins all children).
 */
void concurrency_teardown(void);

/**
 * Spawn a new context to handle a network connection.
 *
 * @param[in] connection_fd Connected file descriptor
 * @param[in] connection_addr Structure describing the connection
 * @param[in] handler Function which will be called in the a concurrent context
 * @return A unique identifier for the child's context. On failure, returns -1
 *   and sets errno.
 */
int concurrency_handle_connection(int connection_fd,
                                  struct sockaddr_in *connection_addr,
                                  void (*handler)(int, struct sockaddr_in *));

/**
 * Join all children which have finished work, i.e. those which can be
 * joined without waiting.
 */
void concurrency_join_finished(void);

#endif
