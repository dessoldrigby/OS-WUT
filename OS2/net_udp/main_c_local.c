#include "utils.h"
#include <netinet/in.h>
#include <stdint.h>

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

#define MAX_BUFFER 576

typedef struct sockaddr_in sockaddr_in_t;

volatile sig_atomic_t last_signal = 0;

void sig_alarm_handler(int sig) 
{
    last_signal = sig;
}

int make_socket(void)
{
	int sock;
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		ERR("socket");
	return sock;
}

sockaddr_in_t make_address(char* address, char* port)
{
    int ret;
    sockaddr_in_t addr;
    struct addrinfo *res, hints = {};
    hints.ai_family = AF_INET;

    if((ret = getaddrinfo(address, port, &hints, &res))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    addr = *(sockaddr_in_t*)(res->ai_addr);
    freeaddrinfo(res);
    return addr;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s domain port file \n", name);
}

void send_and_confirm(int fd, sockaddr_in_t addr, char* buf1, char* buf2, ssize_t size)
{
    struct itimerval ts;
    if(TEMP_FAILURE_RETRY(sendto(fd, buf1, size, 0, &addr, sizeof(addr))) < 0)
        ERR("sendto:");

    memset(&ts, 0, sizeof(struct itimerval));
    ts.it_value.tv_usec = 500000;
    setitimer(ITIMER_REAL, &ts, NULL);
    last_signal = 0;

    while(recv(fd, buf2, size, 0) < 0) {
        if(EINTR != errno)
            ERR("RECV:");
        
        if(SIGALRM == last_signal)
            break;
    }    
}

void do_client(int fd, sockaddr_in_t addr, int file)
{
    char buf[MAX_BUFFER], buf2[MAX_BUFFER];
    int offset = 2 * sizeof(int32_t);
    int32_t chunk_no = 0, last = 0;
    ssize_t size;
    int c;

    do {
        if((size = bulk_read(file, buf + offset, MAX_BUFFER - offset)) < 0)
            ERR("READ FROM FILE");

        *((int32_t*)buf) = htonl(++chunk_no);

        if(size < MAX_BUFFER - offset) {
            last = 1;
            memset(buf + offset + size, 0, MAX_BUFFER - offset - size);
        }

        *(((int32_t*)buf) + 1) = htonl(last);
        memset(buf2, 0, MAX_BUFFER);
        c = 0;

        do {
            c++;
            send_and_confirm(fd, addr, buf, buf2, MAX_BUFFER);
        } while (*((int32_t*)buf2) != htonl(chunk_no) && c <= 5);

        if(*((int32_t*)buf2) != htonl(chunk_no) && c > 5)
            break;
        
    } while(size == MAX_BUFFER - offset);
}

int main(int argc, char **argv)
{
	int fd, file;
	sockaddr_in_t addr;

	if (argc != 4) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (set_sig_handler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");

	if (set_sig_handler(sig_alarm_handler, SIGALRM))
		ERR("Seting SIGALRM:");
        
	if ((file = TEMP_FAILURE_RETRY(open(argv[3], O_RDONLY))) < 0)
		ERR("open:");

	fd = make_socket();
	addr = make_address(argv[1], argv[2]);

	do_client(fd, addr, file);

	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
		ERR("close");
	if (TEMP_FAILURE_RETRY(close(file)) < 0)
		ERR("close");
	return EXIT_SUCCESS;
}