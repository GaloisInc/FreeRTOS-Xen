#ifndef MINLIBC_SYS_SELECT_H
#define MINLIBC_SYS_SELECT_H

#include <time.h>

typedef struct {} fd_set;

int select(int nfds, fd_set *readfds, fd_set *writefds,
              fd_set *exceptfds, struct timeval *timeout);

void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);

#endif
