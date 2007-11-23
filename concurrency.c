/* system includes */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
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
    pid_t child_pid = fork();

    switch (child_pid) {
    case -1:
        return -1;
    case 0:
        signal(SIGTERM, SIG_DFL);
        handler(connection_fd, connection_addr);
        shutdown(connection_fd, SHUT_RDWR);
        close(connection_fd);
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
