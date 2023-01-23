#include <asm-generic/errno-base.h>
#include <linux/limits.h>
#define BUFF_SIZE 1024

#define _UERRORS
#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


void read_from_fifo(int fifo)
{
    ssize_t count;
    char buffer[PIPE_BUF];
    do 
    {
        if((count = read(fifo, buffer, PIPE_BUF)) < 0)
            ERR("READ");

        if(count > 0) {
            printf("\nPID:%d---------------------\n", *((pid_t*)buffer));
            
            for(int i = sizeof(pid_t); i < PIPE_BUF; i++) {
                if(isalnum(buffer[i]))
                    printf("%c", buffer[i]);
            }
        }

    } while (count > 0);
}

int main(int argc, char* argv[]) 
{
    int fifo;
    if(argc != 2)
        ERR("USAGE");

    if(mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
        if(errno != EEXIST)
            ERR("FIFO");

    if((fifo = open(argv[1], O_RDONLY)) < 0)
        ERR("OPEN");

    read_from_fifo(fifo);

    if(close(fifo) < 0)
        ERR("CLOSE FIFO");

    if(unlink(argv[1]) < 0)
        ERR("REMOVE FIFO");

    return EXIT_SUCCESS;
}