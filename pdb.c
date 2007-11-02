#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void connection_handler(int fd, struct sockaddr_in addr) {
    FILE *f = fdopen(fd, "r+");
    if (f) {
       fprintf(f, "poot!\n");
       fclose(f);
    }
    return;
}

void sigchld_handler(int sig) {
    int status;
    wait4(-1, &status, WNOHANG, 0);
}

int main(int argc, char **argv) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "can't create socket: %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_in bind_addr;
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = 5032;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_fd, (struct sockaddr *)&bind_addr,
        sizeof(bind_addr)) == -1) {
        fprintf(stderr, "can't bind socket: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }
    if (listen(socket_fd, 0) == -1) {
        fprintf(stderr, "can't listen to socket: %s\n", strerror(errno));
        close(socket_fd);
        exit(1);
    }

    signal(SIGCHLD, sigchld_handler);

    while (1) {
        struct pollfd socket_poll;
        socket_poll.fd = socket_fd;
        socket_poll.events = POLLIN;
        socket_poll.revents = 0;
        if (poll(&socket_poll, 1, -1) > 0) {
            struct sockaddr_in connection_addr;
            socklen_t connection_addr_length;
        
            int connection_fd = accept(socket_fd,
                (struct sockaddr *)&connection_addr, &connection_addr_length);
            if (connection_fd > 0) {
                pid_t child_pid = vfork(); // Flawfinder: ignore
                switch (child_pid) {
                    case -1:
                        fprintf(stderr, "can't fork: %s\n", strerror(errno));
                        exit(1);
                    case 0:
                        connection_handler(connection_fd, connection_addr);
                        exit(0);
                }

                close(connection_fd);
            }
        }
    }

    close(socket_fd);
    exit(0);
}
