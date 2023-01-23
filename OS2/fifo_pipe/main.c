

#define BUFF_SIZE 200

#define _UERRORS
#define _URND
#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

volatile sig_atomic_t last_signal = 0;

int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void sig_handler(int sig)
{
	last_signal = sig;
}

void sig_killme(int sig)
{
	if (rand() % 5 == 0)
		exit(EXIT_SUCCESS);
}

void sigchld_handler(int sig)
{
	pid_t pid;
	for (;;) {
		pid = waitpid(0, NULL, WNOHANG);
		if (0 == pid)
			return;
		if (0 >= pid) {
			if (ECHILD == errno)
				return;
			ERRS("waitpid:");
		}
	}
}

void child_work(int fd, int R)
{
	char c, buf[BUFF_SIZE + 1];
	unsigned char s;
	srand(getpid());
	if (sethandler(sig_killme, SIGINT))
		ERRS("Setting SIGINT handler in child");
	for (;;) {
		if (TEMP_FAILURE_RETRY(read(fd, &c, 1)) < 1)
			ERRS("read");
		s = 1 + rand() % BUFF_SIZE;
		buf[0] = s;
		memset(buf + 1, c, s);
		if (TEMP_FAILURE_RETRY(write(R, buf, s + 1)) < 0)
			ERRS("write to R");
	}
}

void parent_work(int n, int *fds, int R)
{
	unsigned char c;
	char buf[BUFF_SIZE];
	int status, i;
	srand(getpid());
	if (sethandler(sig_handler, SIGINT))
		ERRS("Setting SIGINT handler in parent");
	for (;;) {
		if (SIGINT == last_signal) {
			i = rand() % n;
			while (0 == fds[i % n] && i < 2 * n)
				i++;
			i %= n;
			if (fds[i]) {
				c = 'a' + rand() % ('z' - 'a');
				status = TEMP_FAILURE_RETRY(write(fds[i], &c, 1));
				if (status != 1) {
					if (TEMP_FAILURE_RETRY(close(fds[i])))
						ERRS("close");
					fds[i] = 0;
				}
			}
			last_signal = 0;
		}
		status = read(R, &c, 1);
		if (status < 0 && errno == EINTR)
			continue;
		if (status < 0)
			ERRS("read header from R");
		if (0 == status)
			break;
		if (TEMP_FAILURE_RETRY(read(R, buf, c)) < c)
			ERRS("read data from R");
		buf[(int)c] = 0;
		printf("\n%s\n", buf);
	}
}

void create_children_and_pipes(int n, int *fds, int R)
{
	int tmpfd[2];
	int max = n;
	while (n) {
		if (pipe(tmpfd))
			ERRS("pipe");
		switch (fork()) {
		case 0:
			while (n < max)
				if (fds[n] && TEMP_FAILURE_RETRY(close(fds[n++])))
					ERRS("close");
			free(fds);
			if (TEMP_FAILURE_RETRY(close(tmpfd[1])))
				ERRS("close");
			child_work(tmpfd[0], R);
			if (TEMP_FAILURE_RETRY(close(tmpfd[0])))
				ERRS("close");
			if (TEMP_FAILURE_RETRY(close(R)))
				ERRS("close");
			exit(EXIT_SUCCESS);

		case -1:
			ERRS("Fork:");
		}
		if (TEMP_FAILURE_RETRY(close(tmpfd[0])))
			ERRS("close");
		fds[--n] = tmpfd[1];
	}
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s n\n", name);
	fprintf(stderr, "0<n<=10 - number of children\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int n, *fds, R[2];
	if (2 != argc)
		usage(argv[0]);
	n = atoi(argv[1]);
	if (n <= 0 || n > 10)
		usage(argv[0]);
	if (sethandler(SIG_IGN, SIGINT))
		ERRS("Setting SIGINT handler");
	if (sethandler(SIG_IGN, SIGPIPE))
		ERRS("Setting SIGINT handler");
	if (sethandler(sigchld_handler, SIGCHLD))
		ERRS("Setting parent SIGCHLD:");
	if (pipe(R))
		ERRS("pipe");
	if (NULL == (fds = (int *)malloc(sizeof(int) * n)))
		ERRS("malloc");
	create_children_and_pipes(n, fds, R[1]);
	if (TEMP_FAILURE_RETRY(close(R[1])))
		ERRS("close");
	parent_work(n, fds, R[0]);
	while (n--)
		if (fds[n] && TEMP_FAILURE_RETRY(close(fds[n])))
			ERRS("close");
	if (TEMP_FAILURE_RETRY(close(R[0])))
		ERRS("close");
	free(fds);
	return EXIT_SUCCESS;
}