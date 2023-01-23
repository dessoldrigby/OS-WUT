#include "utils.h"
#include <netinet/in.h>

#define _GNU_SOURCE
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int make_socket(void)
{
	int socketfd;
	if ((socketfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		ERR("socket");

	return socketfd;
}

struct sockaddr_in make_address(char* address, char* port)
{
    int ret;
    struct sockaddr_in addr;
    struct addrinfo *res;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;

    if((ret = getaddrinfo(address, port, &hints, &res))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    addr = *(struct sockaddr_in*)(res->ai_addr);
    freeaddrinfo(res);

    return addr;
}

int connect_socket(char* name, char* port)
{
	struct sockaddr_in addr;
	int socketfd;
	socketfd = make_socket();
    addr = make_address(name, port);

	if (connect(socketfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		if (errno != EINTR)
			ERR("connect");
		else {
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0)
				ERR("select");
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
				ERR("getsockopt");
			if (0 != status)
				ERR("connect");
		}
	}

	return socketfd;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s socket operand1 operand2 operation \n", name);
}

void prepare_request(char **argv, int32_t data[5])
{
	data[0] = htonl(atoi(argv[3]));
	data[1] = htonl(atoi(argv[4]));
	data[2] = htonl(0);
	data[3] = htonl((int32_t)(argv[5][0]));
	data[4] = htonl(1);
}

void print_answer(int32_t data[5])
{
	if (ntohl(data[4]))
		printf("%d %c %d = %d\n", ntohl(data[0]), (char)ntohl(data[3]), ntohl(data[1]), ntohl(data[2]));
	else
		printf("Operation impossible\n");
}

int main(int argc, char **argv)
{
	int fd;
	int32_t data[5];
	if (argc != 6) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (set_sig_handler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
	fd = connect_socket(argv[1], argv[2]);
	prepare_request(argv, data);
	if (bulk_write(fd, (char *)data, sizeof(int32_t[5])) < 0)
		ERR("write:");
	if (bulk_read(fd, (char *)data, sizeof(int32_t[5])) < (int)sizeof(int32_t[5]))
		ERR("read:");
	print_answer(data);
	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
		ERR("close");
	return EXIT_SUCCESS;
}