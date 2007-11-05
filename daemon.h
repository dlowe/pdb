#ifndef __DAEMON_H
#define __DAEMON_H

/** \file daemon.h
 * \brief Daemonize.
 *
 * Functions for cleanly detaching from the invoking terminal. This API allows
 * a process to fork early, but maintain the ability to report problems back
 * to the parent terminal until it is possible to open a log file or some other
 * method of communicating problems.
 */

/**
 * Become a background process. Users of this function should use the
 * daemon_error() function to report problems encountered before they open
 * a log or other channel for error reporting, at which point they should call
 * daemon_done().
 * @return 0 on success. On failure, returns -1 and sets errno.
 */
int  daemon_begin(void);

/**
 * Used to report problems between forking and opening some other form of
 * communication for error reporting.
 * @param format a printf() style format string
 * @param ... printf() style argument list
 */
void daemon_error(const char *format, ...);

/**
 * Informs the parent process that the daemon is able to cleanly communicate
 * by another mechanism, so the parent can exit.
 */
void daemon_done(void);

#endif
