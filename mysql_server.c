/* system includes */
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

/* project includes */
#include "mysql_server.h"

void mysql_server(int fd, struct sockaddr_in *addr)
{
    char buffer[4096];
    int bytes;

    while ((bytes = read(fd, buffer, sizeof(buffer))) != 0) {
        if (bytes != -1) {
        }
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return;
}
