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

void concurrency_setup(void)
{
}

void concurrency_teardown(void)
{
    int status;
    pid_t pid;

    do {
        pid = wait3(&status, 0, 0);
    } while (pid > -1 || errno != ECHILD);
}

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

void concurrency_join_finished(void)
{
    int status;
    pid_t pid;

    do {
        pid = wait3(&status, WNOHANG, 0);
    } while (pid > 0);
}
