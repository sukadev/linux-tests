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

void log_socket_option(int sock, int option, char *msg)
{
	struct timeval timeo;
	int level;
	int slen;
	int rc;

	level = 6;	// tcp, CHECK

	if (option == SO_RCVTIMEO) {

		memset(&timeo, 0, sizeof(timeo));

		rc = getsockopt(sock, level, SO_RCVTIMEO, &timeo, &slen);
		if (rc < 0) {
			printf("%s(%s): getsockopt(Rx) failed, %s\n",
					__func__, msg, strerror(errno));
			goto out;
		}
		printf("Slen %d, Rx Timeout: %d.%d\n", slen, timeo.tv_sec,
				timeo.tv_usec);

	} else if (option == SO_SNDTIMEO) {
		rc = getsockopt(sock, level, SO_SNDTIMEO, &timeo, &slen);
		if (rc < 0) {
			printf("%s(%s): getsockopt(Tx) failed, %s\n",
				__func__, msg, strerror(errno));
			goto out;
		}

		printf("%s(%s): Slen %d, Tx Timeout: %d.%d\n", __func__,
				msg, slen, timeo.tv_sec, timeo.tv_usec);
	} else {
		printf("%s(%s): Unknown option %d\n", __func__, msg, option);
		return;
	}
out:
	return;
}

void set_socket_option(int sock, int option, int val, char *msg)
{
	struct timeval timeo;
	const char *optstr;
	int level;
	int slen;
	int rc;

	level = 6;	// tcp, CHECK
	if (option == SO_RCVTIMEO || option == SO_SNDTIMEO) {
		timeo.tv_sec = val;
		timeo.tv_usec = 0;
		slen = sizeof(timeo);

		optstr = "Rx Timeout";
		if (option == SO_SNDTIMEO)
			optstr = "Tx Timeout";

		rc = setsockopt(sock, level, option, &timeo, slen);
		if (rc < 0) {
			printf("%s(%s): setsockopt(%s) failed, %s\n",
				__func__, msg, optstr, strerror(errno));
			goto out;
		}

		printf("%s(%s): %s set to %d seconds\n",
				__func__, msg, optstr, val);
	} else {
		printf("%s(%s) Unknown option %d\n", __func__, msg, option);
	}
out:
	return;
}

