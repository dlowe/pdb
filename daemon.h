#ifndef __DAEMON_H
#define __DAEMON_H

int  daemon_begin(void);

void daemon_error(const char *format, ...);
void daemon_done(void);

#endif
