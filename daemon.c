/* system includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* project includes */
#include "daemon.h"

#define ERROR_PIPE_BUFFER_SIZE 512
static int error_pipe[2];

/* daemon_begin
 * become a background process. Users of this function should use the
 * daemon_error() function to report problems encountered before they open
 * a log or other channel for error reporting, at which point they should call
 * daemon_done(). */
int daemon_begin(void)
{
    /* create a pipe so daemon can report errors before it opens the log */
    if (pipe(error_pipe) == -1) {
        return -1;
    }

    /* fork */
    switch (fork()) {
    case 0:
        break;
    case -1:
        return -1;
    default:
        {
            /* Flawfinder: ignore */
            char buffer[ERROR_PIPE_BUFFER_SIZE];

            close(error_pipe[1]);

            switch (read(error_pipe[0], buffer, sizeof(buffer))) {
            case 0:
                /* success: error pipe closed without a message */
                exit(0);
            case -1:
                /* read error, should never happen.... */
                exit(1);
            default:
                /* child didn't initialize successully */
                fprintf(stderr, "error: %s\n", buffer);
                exit(1);
            }
        }
    }

    /* become session leader */
    if (setsid() == -1) {
        daemon_error("%s", strerror(errno));
        return -1;
    }

    /* close all file descriptors except the pipe to parent */
    for (int fd = 0; fd < getdtablesize(); ++fd) {
        if (fd != error_pipe[1]) {
            close(fd);
        }
    }

    /* clear umask */
    umask(0);

    return 0;
}

/* daemon_error
 * used to report problems between forking and opening some other form of
 * communication for error reporting */
void daemon_error(const char *format, ...)
{
    /* Flawfinder: ignore */
    char buffer[ERROR_PIPE_BUFFER_SIZE];
    va_list args;

    va_start(args, format);
    /* Flawfinder: ignore */
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    write(error_pipe[1], buffer, strlen(buffer));
}

/* daemon_done
 * informs the parent process that the daemon is able to cleanly communicate
 * by another mechanism, so the parent can exit */
void daemon_done(void)
{
    close(error_pipe[1]);
}
