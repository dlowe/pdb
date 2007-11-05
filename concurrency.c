/* system includes */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* project includes */
#include "concurrency.h"

/**
 * Set up concurrency strategy
 */
void concurrency_setup(void)
{
}

/**
 * Clean up concurrent work (joins all children).
 */
void concurrency_teardown(void)
{
    int status;
    pid_t pid;

    do {
        pid = wait3(&status, 0, 0);
    } while (pid > -1 || errno != ECHILD);
}

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
                                  void (*handler) (int, struct sockaddr_in *))
{
    /* Flawfinder: ignore vfork */
    pid_t child_pid = vfork();

    switch (child_pid) {
    case -1:
        return -1;
    case 0:
        handler(connection_fd, connection_addr);
        exit(0);
    }

    close(connection_fd);
    return 0;
}

/**
 * Join all children which have finished work, i.e. those which can be
 * joined without waiting.
 */
void concurrency_join_finished(void)
{
    int status;
    pid_t pid;

    do {
        pid = wait3(&status, WNOHANG, 0);
    } while (pid > 0);
}
