#define _GNU_SOURCE
#define BUFF_SIZE 1024

#define PARENT 'P'
#define CHILD 'C'

#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

volatile sig_atomic_t su1_signal;
volatile sig_atomic_t su2_signal;
volatile sig_atomic_t su1_count = 0;

void sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void su1_handler(int sig)
{
    su1_count++;
    su1_signal = sig; // BAD
}

void su2_handler(int sig)
{
    su2_signal = sig;
}

void sig_child_handler(int sig)
{
	pid_t pid;
	for (;;) {
		pid = waitpid(0, NULL, WNOHANG);
		if (pid == 0)
			return;
		if (pid <= 0) {
			if (errno == ECHILD)
				return;
			ERR("waitpid");
		}
	}
}


void child_work(int n, sigset_t oldmask)
{   
    int pid = getpid();
    srand(time(NULL) * pid);
    int s = RND(10, 100), out, timeout = n;
    ssize_t count = s * sizeof(char) + 1, written = 0, total = 0;
    n += '0';

    printf("[%d]:[%c] created with n:{%d} - s:{%d}\n", pid, CHILD, n, s);

    struct timespec t = { timeout, 0 };
    nanosleep(&t, NULL);

    char name[BUFF_SIZE];
    sprintf(name, "%d.txt", pid);

    char* buf = (char*)malloc(count);
    if(!buf) 
        ERR("MALLOC");

    memset(buf, n, count);
    buf[count - 1] = '\n';

    if ((out = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
		ERR("OPEN");

    while(1) {
        su1_signal = 0;
        su2_signal = 0;

        while(su1_signal != SIGUSR1 && su2_signal != SIGUSR2) {
            printf("[%d]:[%c] waits SU1 or SU2\n", pid, CHILD);
            sigsuspend(&oldmask);
        }
        // printf("[%d] Waits SU1 or SU2\n", pid);
        // sigsuspend(&oldmask);

        printf("[%d]:[%c] operating\n", pid, CHILD);

        if(su1_signal == SIGUSR1) {
            printf("[%d]:[%c] received SU1\n", pid, CHILD);
            if((written = bulk_write(out, buf, count)) < 0)
                ERR("WRITE");

            printf("[%d]:[%c] wrote {%ld} bytes of {%c}\n", pid, CHILD, written, n);

            total += written;
        }
        if(su2_signal == SIGUSR2) {
            printf("[%d]:[%c] received SU2, exiting\n", pid, CHILD);
            break;
        }
    }

    if(close(out)) 
        ERR("CLOSE");    
    free(buf);

    printf("[%d]:[%c] terminates, name: {%s}, wrote {%ld} bytes of {%c}\n", pid, CHILD, name, total, n);
}

void parent_work()
{
    struct timespec t = { 2, 0 };
    int iter = 4, p;

    for(int i = 0; i < iter; i++) {
        p = nanosleep(&t, NULL);
        // while((p = nanosleep(&dt, &dt)) < 0);

        printf("[%d]:[%c] sent SIGUSR1\n", getpid(), PARENT);

        if(kill(0, SIGUSR1))  
            ERR("KILL");
    }
    
    printf("[%d]:[%c] reports %d SIGUSR1's\n", getpid(), PARENT, su1_count);

    if(kill(0, SIGUSR2)) 
        ERR("KILL");

    printf("[%d]:[%c] waits for childer\n", getpid(), PARENT);
    while(wait(NULL) > 0) ;
}



int main(int argc, char *argv[])
{
    sethandler(sig_child_handler, SIGCHLD);
    sethandler(su1_handler, SIGUSR1);
    sethandler(su2_handler, SIGUSR2);

    sigset_t mask, oldmask;
    sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

    for(int i = 1; i < argc; i++) {
        switch(fork()) {
            case 0:
                sethandler(su1_handler, SIGUSR1);
                sethandler(su2_handler, SIGUSR2);
                child_work(atoi(argv[i]), oldmask);

                exit(EXIT_SUCCESS);
            case 1: 
                ERR("FORK");
                
                exit(EXIT_FAILURE);
        }
    }

    parent_work();

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
	return EXIT_SUCCESS;
}
