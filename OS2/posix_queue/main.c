#define _UERRORS
#define _URND
// #define _USIG
#include "utils.h"

#define _GNU_SOURCE
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
#include <mqueue.h>

#define LIFE_SPAN 10
#define NUM_MIN 0
#define NUM_MAX 9
#define B_IN "/bingo_in"
#define B_OUT "/bingo_out"

volatile sig_atomic_t children_left = 0;

void read_args(int argc, char* argv[], int* out_children_count)
{
    if(argc != 2)
        ERR("ENTRY ARGS");

    *out_children_count = atoi(argv[1]);
    if(*out_children_count <= 0 || *out_children_count >= 100)
        ERR("children count must be 0 < n < 100");
}

void mq_handler(int sig, siginfo_t* info, void* p)
{
    mqd_t* pin;
    u_int8_t ni;
    unsigned msg_prio;

    pin = (mqd_t*)info->si_value.sival_ptr;

    static struct sigevent not;
    not.sigev_notify = SIGEV_SIGNAL;
    not.sigev_signo = SIGRTMIN;
    not.sigev_value.sival_ptr = pin;
    if(mq_notify(*pin, &not) < 0)
        ERR("MQ_NOTIFY");

    while(true) {
        if(mq_receive(*pin, (char*)&ni, 1, &msg_prio) < 1) {
            if(errno == EAGAIN)
                break;
            else
                ERR("MQ_RECEIVE");
        }

        if(0 == msg_prio)
            printf("MQ: got timeout from %d.", ni);
        else
            printf("MQ: %d is a bingo number!", ni);
    }
}

void sc_handler(int sig, siginfo_t *s, void *p)
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
		children_left--;
	}
}

void child_work(int count, mqd_t pin, mqd_t pout)
{
    int life;
    u_int8_t ni, my_bingo;
    
    srand(getpid());

    life = rand() % LIFE_SPAN + 1;
    my_bingo = (u_int8_t)RND(NUM_MIN, NUM_MAX);

    while(life--) {
        if(1 > TEMP_FAILURE_RETRY(
                mq_receive(
                    pout, 
                    (char*)&ni, 
                    1, 
                    NULL
                )
            )
        )
            ERR("MQ_RECEIVE");

        printf("[%d] Received %d\n", getpid(), ni);

        if(my_bingo == ni) {
            if( TEMP_FAILURE_RETRY(
                    mq_send(
                        pin, 
                        (const char*)&my_bingo, 
                        1, 
                        1
                    )
                )
            )
                ERR("MQ_SEND");

            return;
        }
    }

    if( TEMP_FAILURE_RETRY(
            mq_send(
                pin, 
                (const char*)&count, 
                1, 
                0
            )
        )
    )
        ERR("MQ_SEND");
}

void parent_work(mqd_t pout)
{
    srand(getpid());
    u_int8_t ni;

    while(children_left) {
        ni = (u_int8_t)RND(NUM_MIN, NUM_MAX);

        if( TEMP_FAILURE_RETRY(
                mq_send(
                    pout, 
                    (const char*)&ni, 
                    1,
                    0
                )
            )
        )
            ERR("MQ_SEND");

        sleep(1);
    }

    printf("[PARENT] Terminates\n");
}

void create_children(int n, mqd_t pin, mqd_t pout)
{
    while(n-- > 0) {
        switch (fork()) {
            case 0:
                // children_left++;
                child_work(n, pin, pout);
                // children_left--;
                exit(EXIT_SUCCESS);
            case -1:
                perror("FORK");
                exit(EXIT_FAILURE);
        }

        children_left--;
    }
}

int set_sig_action(void (*handler)(int, siginfo_t*, void*), int sig_no)
{
    struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = handler;
	act.sa_flags = SA_SIGINFO;
	if (-1 == sigaction(sig_no, &act, NULL))
		return -1;

    return 0;
}

int main(int argc, char* argv[])
{
    int child_count;
    read_args(argc, argv, &child_count);

    mqd_t pin, pout;
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1;
    if((mqd_t) - 1 == (
        pin = 
            TEMP_FAILURE_RETRY(
                mq_open(
                    B_IN, 
                    O_RDWR | O_NONBLOCK | O_CREAT, 
                    0600, 
                    &attr
                )
            )
        )
    ) {
        ERR("MQ OPEN IN");
    }

    if ((mqd_t) - 1 == (
        pout = 
            TEMP_FAILURE_RETRY(
                mq_open(
                    B_OUT, 
                    O_RDWR | O_CREAT, 
                    0600, 
                    &attr
                )
            )
        )
    ) {
		ERR("mq open out");
    }

    if(set_sig_action(sc_handler, SIGCHLD))
        ERR("SET SIG ACTION");
    if(set_sig_action(mq_handler, SIGRTMIN))
        ERR("SET SIG ACTION");

    create_children(child_count, pin, pout);

    static struct sigevent noti;
    noti.sigev_notify = SIGEV_SIGNAL;
	noti.sigev_signo = SIGRTMIN;
	noti.sigev_value.sival_ptr = &pin;
	if (mq_notify(pin, &noti) < 0)
		ERR("mq_notify");

    parent_work(pout);

    mq_close(pin);
    mq_close(pout);

    if(mq_unlink(B_IN) || mq_unlink(B_OUT))
        ERR("MQ_UNLINK");

    return EXIT_SUCCESS;
}