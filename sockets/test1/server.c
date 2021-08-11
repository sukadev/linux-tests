#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>

int main()
{
	struct sockaddr_in my_addr, peer_addr;
	struct in_addr inaddr;
	struct timeval timeo;
	socklen_t slen;
	char buf[256];
	int level;
	int flag;
	int sock;
	int pfd;
	int rc;

	level = SOL_SOCKET;
	level = 6;	// tcp

	sock = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (sock < 0) {
		perror("socket()");
		exit(1);
	}

	log_socket_option(sock, SO_RCVTIMEO, "Default");
	log_socket_option(sock, SO_SNDTIMEO, "Default");

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(9876);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	rc = bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));
	if (rc < 0) {
		perror("bind()");
		exit(1);
	}

	printf("Bound to Addresss: 0x%x\n", my_addr.sin_addr.s_addr);
	printf("Listening...\n");

	if (listen(sock, 20) < 0) {
		perror("listen()");
		exit(1);
	}

	printf("Accepting...\n");
	slen = sizeof(peer_addr);
	pfd = accept(sock, (struct sockaddr *)&peer_addr, &slen);
	if (pfd < 0) {
		perror("accept()");
		exit(1);
	}

	set_socket_option(pfd, SO_RCVTIMEO, 5, "RxTimeo");
	set_socket_option(pfd, SO_SNDTIMEO, 5, "TxTimeo");

	printf("Reading...\n");
	rc = read(pfd, buf, sizeof(buf));
	if (rc < 0) {
		perror("recv()");
		exit(1);
	}

	printf("Rx: rc %d, data %.16s\n", rc, buf);
	sleep(3);

	strcpy(buf, "Yesterday was Aug 9\n");
	printf("Writing...\n");

	write(pfd, buf, strlen(buf));

	close(pfd);
	close(sock);
}
