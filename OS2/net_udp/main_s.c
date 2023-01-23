#include "utils.h"

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define BACKLOG 3
#define MAX_BUFFER 576
#define MAX_ADDR 5

typedef struct sockaddr_in sockaddr_in_t;

typedef struct _connections {
    int free;
    int32_t chunk_no;
    sockaddr_in_t addr;

} connections_t;

int make_socket(int domain, int type)
{
	int socketfd;
	if ((socketfd = socket(domain, type, 0)) < 0)
		ERR("socket");

	return socketfd;
}

int bind_inet_socket(uint16_t port, int type)
{
    sockaddr_in_t addr;
    int socketfd, t = 1;
    socketfd = make_socket(PF_INET, type);
    memset(&addr, 0, sizeof(sockaddr_in_t));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("SETSOCKOPT");

    if(bind(socketfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        ERR("BIND");

    if(SOCK_STREAM == type)
        if(listen(socketfd, BACKLOG) < 0)
            ERR("LISTEN");

    return socketfd;
}

int find_index(sockaddr_in_t addr, connections_t con[MAX_ADDR])
{
    int empty = -1, pos = -1;

    for(int i = 0; i < MAX_ADDR; i++) {
        if(con[i].free)
            empty = 1;
        else if(0 == memcmp(&addr, &(con[i].addr), sizeof(sockaddr_in_t))) {
            pos = i;
            break;
        }
    }

    if(-1 == pos && -1 != empty) {
        con[empty].free = 0;
        con[empty].chunk_no = 0;
        con[empty].addr = addr;
        
        pos = empty;
    }

    return pos;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s socket port\n", name);
}

void launch_server(int fd)
{
	sockaddr_in_t addr;
	connections_t con[MAX_ADDR];
	socklen_t size = sizeof(addr);
	char buf[MAX_BUFFER];
	int i;
	int32_t chunkNo, last;
	for (i = 0; i < MAX_ADDR; i++)
		con[i].free = 1;
	while (true) {
		if (TEMP_FAILURE_RETRY(recvfrom(fd, buf, MAX_BUFFER, 0, &addr, &size) < 0))
			ERR("read:");
		if ((i = find_index(addr, con)) >= 0) {
			chunkNo = ntohl(*((int32_t *)buf));
			last = ntohl(*(((int32_t *)buf) + 1));
			if (chunkNo > con[i].chunk_no + 1)
				continue;
			else if (chunkNo == con[i].chunk_no + 1) {
				if (last) {
					printf("Last Part %d\n%s\n", chunkNo, buf + 2 * sizeof(int32_t));
					con[i].free = 1;
				} else
					printf("Part %d\n%s\n", chunkNo, buf + 2 * sizeof(int32_t));
				con[i].chunk_no++;
			}
			if (TEMP_FAILURE_RETRY(sendto(fd, buf, MAX_BUFFER, 0, &addr, size)) < 0) {
				if (EPIPE == errno)
					con[i].free = 1;
				else
					ERR("send:");
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int fd;
	if (argc != 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (set_sig_handler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
	fd = bind_inet_socket(atoi(argv[1]), SOCK_DGRAM);
	launch_server(fd);
	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
		ERR("close");
	fprintf(stderr, "Server has terminated.\n");
	return EXIT_SUCCESS;
}