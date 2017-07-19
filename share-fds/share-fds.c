#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

/*
 * Create an UNIX domain socket connection between a parent-child pair.
 * Send a file descriptor from child to parent.
 */

#define ERRMSG	strerror(errno)

#define FDSHARE_SOCKET	"/tmp/fdshare1.socket"
#define FDSHARE_DATA	"fdshare.data"
#define MAX_FDS		2

int send_fds(int sock, int *fdlist, int nfds)
{
	int rc, datafd, len;
	char iobuf[1];
	struct msghdr msg = { 0 };
	struct cmsghdr *cmsg;
	struct iovec io = {
			.iov_base = iobuf,
			.iov_len = sizeof(iobuf),
	};

	int *fdptr;
	union {
		char		buf[CMSG_SPACE(MAX_FDS * sizeof(int))];
		struct cmsghdr	align;
	} u;

	assert(nfds <= MAX_FDS);

	len = sizeof(int) * nfds;

	msg.msg_iov = &io;	/* Unused but needed? */
	msg.msg_iovlen = 1;
	msg.msg_control = u.buf;
	msg.msg_controllen = sizeof(u.buf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(len);

	fdptr = (int *)CMSG_DATA(cmsg);
	memcpy(fdptr, fdlist, len);

	rc = sendmsg(sock, &msg, 0);
	if (rc < 0) {
		printf("Child: sendmsg() %d, error %s\n", rc, ERRMSG);
		_Exit(1);
	} else if (rc != sizeof(iobuf)) {
		printf("Child: sendmsg() sent %d of %d bytes?\n", rc,
				sizeof(iobuf));
	}
}

int receive_fd(int sock)
{
	int n;
	struct msghdr msg =  { 0 };
	struct cmsghdr *cmsg;
	char ctlbuf[1024];
	char iobuf[1024];
	struct iovec io = {
			.iov_base = &iobuf,
			.iov_len = sizeof(iobuf)
	};
	int myfds[MAX_FDS];

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = ctlbuf;
	msg.msg_controllen = sizeof(ctlbuf);

	n = recvmsg(sock, &msg, 0);
	if (n <= 0) {
		printf("Parent recvmsg() %d, error %s\n", n, ERRMSG);
		return -1;
	}

	cmsg = CMSG_FIRSTHDR(&msg);
	printf("Parent: Got message, n %d\n", n); fflush(stdout);

	printf("Parent: CMSG level %d (%sSOL_SOCKET), type %d (%sSCM_RIGHTS)\n",
			cmsg->cmsg_level, cmsg->cmsg_level == SOL_SOCKET?"":"!",
			cmsg->cmsg_type, cmsg->cmsg_type == SCM_RIGHTS?"":"!");

	memcpy(myfds, (int *)CMSG_DATA(cmsg), MAX_FDS*sizeof(int));

	printf("fd[0] %d, fd[1] %d\n", myfds[0], myfds[1]);

	fflush(stdout);
	return myfds[0];
}

int do_child(void)
{
	int sock, datafd, myfds[2], nfds;
	char *fname;
	char buf[256];
	struct sockaddr_un addr;
	char *datastr;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	fname = FDSHARE_SOCKET;
	strncpy(addr.sun_path, fname, strlen(fname));

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		printf("Child: connect() error %s\n", ERRMSG);
		_Exit(1);
	}

	datafd = open(FDSHARE_DATA, O_RDWR|O_TRUNC|O_CREAT, 0640);
	if (datafd < 0) {
		printf("open(%s): error %s\n", FDSHARE_DATA, ERRMSG);
		_Exit(1);
	}
	datastr = "Hello world!";
	write(datafd, datastr, strlen(datastr));

	myfds[0] = datafd;
	myfds[1] = datafd;
	nfds = 2;

	printf("Child: sending %d fds to sock %d\n", nfds, sock);

	send_fds(sock, myfds, nfds);

	sleep(3);

	printf("Child: exiting\n");
	return 0;

}

int main(int argc, char *argv[])
{
	int rc;
	int sock;
	char *str, *fname;
	char buf[256];
	int datafd, cfd, status;
	struct sockaddr_un addr;
	struct stat stbuf;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("socketpair(): Error %s\n", ERRMSG);
		_Exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, FDSHARE_SOCKET, sizeof(addr.sun_path) - 1);

	unlink(addr.sun_path);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		printf("bind() Error %s\n", ERRMSG);
		_Exit(1);
	}

	rc = fork();
	if (rc < 0) {
		printf("fork(): Error %s\n", ERRMSG);
		_Exit(1);
	}

	if (rc == 0) {
		return do_child();
	}

	if (listen(sock, 5) < 0) {
		printf("listen(:) Error %s\n", ERRMSG);
		_Exit(1);
	}

	cfd = accept(sock, NULL, NULL);
	if (cfd < 0) {
		printf("accept(): Error %s\n", ERRMSG);
		_Exit(1);
	}

	datafd = receive_fd(cfd);

	/*
	 * Proccesses share file pointer, so we should position it!
	 */
	lseek(datafd, 0L, SEEK_SET);
	rc = read(datafd, buf, sizeof(buf));
	if (rc <= 0)
		printf("Parent: read() returns %d, error %s\n", rc, ERRMSG);
	else
		printf("Parent: Got data '%s'\n", buf);

	wait(&status);
	if (WIFEXITED(status))
		printf("Parent: child exit status %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		printf("Parent: child got signal %d\n", WTERMSIG(status));

	_Exit(0);

}

