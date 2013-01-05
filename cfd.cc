#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/select.h>
#include "cf.h"
#include "rpchelper.hh"
#include "scopedlock.hh"

// the following lines added by YHF
#include <sys/types.h>
#include <sys/socket.h>

// Why doesn't rpcgen stick this in cf.h??
extern "C" void cfs_prog_1(struct svc_req *rqstp, register SVCXPRT *transp);

// Rather round-about way of figuring out when the rpc/svc library closes FDs
static uint32_t fd_gen[FD_SETSIZE];
static pthread_mutex_t fd_accept_lock;

static void *
client_thread(void *arg)
{
    int s = (int) (long) arg;
    SVCXPRT *svc = svcfd_create(s, 0, 0);
    uint32_t start_gen = fd_gen[s];

    if (!svc_register(svc, CFS_PROG, CFS_VERS1, cfs_prog_1, 0)) {
	fprintf(stderr, "Unable to register dispatcher for (CFS_PROG, CFS_VERS1).\n");
	svc_destroy(svc);
	return 0;
    }

    for (;;) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(s, &fds);
	svc_getreqset(&fds);

	scoped_pthread_lock mu(&fd_accept_lock);
	// Check if the socket got closed and reused already
	if (fd_gen[s] != start_gen)
	    break;

	// Check if the socket is simply closed..
	struct sockaddr_in sin;
	socklen_t sin_len = sizeof(sin);
	if (getpeername(s, (struct sockaddr *) &sin, &sin_len) < 0)
	    break;
    }

    return 0;
}

int
main(int ac, char **av)
{
    if (ac != 2) {
	printf("Usage: %s port-number\n", av[0]);
	return -1;
    }

    int port = atoi(av[1]);
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
	perror("socket");
	return -1;
    }

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (bind(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	perror("bind");
	return -1;
    }

    socklen_t len = sizeof(sin);
    if (getsockname(fd, (struct sockaddr *) &sin, &len) < 0) {
	perror("getsockname");
	return -1;
    }

    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0) < 0) ||
	fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
	perror("fcntl");
	return -1;
    }

    if (listen(fd, 5) < 0) {
	perror("listen");
	return -1;
    }

    printf("Listening on port %d\n", ntohs(sin.sin_port));
    for (;;) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	int r = select(fd + 1, &fds, 0, 0, 0);
	if (r <= 0) {
	    perror("select");
	    continue;
	}

	scoped_pthread_lock mu(&fd_accept_lock);
	len = sizeof(sin);
	int s = accept(fd, (struct sockaddr *) &sin, &len);
	if (s < 0) {
	    perror("accept");
	    continue;
	}

	fd_gen[s]++;
	pthread_t tid;
	if (pthread_create(&tid, 0, client_thread, (void *) s) < 0) {
	    perror("pthread_create");
	    close(s);
	} else {
	    pthread_detach(tid);
	}
    }
}
