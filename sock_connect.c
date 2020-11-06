#include "sock_connect.h"

int connect_wait (
		int sockno,
		struct sockaddr * addr,
		size_t addrlen,
		int timeout){
	/*	La función devuelve 0 si la conexión se pudo establecer
		dentro del tiempo dado. Devuelve 1 si la conexión agotó
		el tiempo de espera y -1 si ocurrió un error. */

	struct timeval t;
	int res, opt;
	t.tv_sec = timeout;	//en segundos
	t.tv_usec = 0;
	// get socket flags
	if ((opt = fcntl (sockno, F_GETFL, NULL)) < 0) {
		return -1;
	}

	setsockopt(sockno, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(struct timeval));

	// set socket non-blocking
	if (fcntl (sockno, F_SETFL, opt | O_NONBLOCK) < 0) {
		return -1;
	}

	// try to connect
	if ((res = connect (sockno, addr, addrlen)) < 0) {
		if (errno == EINPROGRESS) {
			fd_set wait_set;
			
			// make file descriptor set with socket
			FD_ZERO (&wait_set);
			FD_SET (sockno, &wait_set);

			// wait for socket to be writable; return after given timeout
			res = select (sockno + 1, NULL, &wait_set, NULL, &t);
		}
	} else {
		// connection was successful immediately
		res = 1;
	}

	// reset socket flags
	if (fcntl (sockno, F_SETFL, opt) < 0)
		return -1;

	// an error occured in connect or select
	if (res < 0)
		return -1;
	else if (res == 0){
		errno = ETIMEDOUT;
		return 1;
	} else {
		// almost finished...
		socklen_t len = sizeof (opt);

		// check for errors in socket layer
		if (getsockopt (sockno, SOL_SOCKET, SO_ERROR, &opt, &len) < 0) {
			return -1;
		}

		// there was an error
		if (opt) {
			errno = opt;
			return -1;
		}
	}
	return 0;
}
