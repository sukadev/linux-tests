#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>

int main()
{
	struct sockaddr_in my_addr, peer_addr;
	struct timeval timeo;
	socklen_t slen;
	char buf[256];
	int level;
	int sock;
	int rc;

	sock = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (sock < 0) {
		perror("socket()");
		exit(1);
	}

	log_socket_option(sock, SO_RCVTIMEO, "Default");
	log_socket_option(sock, SO_SNDTIMEO, "Default");

	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(9876);
	rc = inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr);
	if (!rc) {
		printf("inet_aton: rc %d\n", rc);
		exit(1);
	}

	printf("Peer Addresss: 0x%x\n", peer_addr.sin_addr.s_addr);
	printf("Connecting...\n");

	rc = connect(sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
	if (rc) {
		printf("connect: rc %d, error %s\n", rc, strerror(errno));
		exit(1);
	}

	sleep(25);
	strcpy(buf, "Today is Aug 10\n");
	printf("Writing...\n");
	write(sock, buf, strlen(buf));
	sleep(5);

	printf("Reading...\n");
	recv(sock, buf, sizeof(buf), 0);
	printf("Client received %.16s\n", buf);
	printf("client exiting\n");

}
