#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>

#ifndef SOCK_CONNECT_H
#define SOCK_CONNECT_H

int connect_wait (
        int sockno,
        struct sockaddr * addr,
        size_t addrlen,
		int timeout);

#endif
