#ifndef __CONCURRENCY_H
#define __CONCURRENCY_H

#include <netinet/in.h>

void concurrency_setup(void);
void concurrency_teardown(void);
int concurrency_handle_connection(int connection_fd,
                                  struct sockaddr_in *connection_addr,
                                  void (*handler)(int, struct sockaddr_in *));
void concurrency_join_finished(void);

#endif
