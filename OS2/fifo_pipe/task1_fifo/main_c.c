#include <linux/limits.h>
#define _UERRORS
#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))



void write_to_fifo(int fifo, int file)
{
    int64_t count;
    char buffer[PIPE_BUF];
    char *buf;
    
    *((pid_t*)buffer) = getpid();
    buf = buffer + sizeof(pid_t);

    do 
    {
        if((count = read(file, buf, MSG_SIZE)) < 0)
            ERR("READ");

        if(count < MSG_SIZE)
            memset(buf + count, 0, MSG_SIZE - count);

        if(count > 0)
            if(write(fifo, buffer, PIPE_BUF) < 0)
                ERR("WRITE");

    } while(count == MSG_SIZE);
}

int main(int argc, char* argv[])
{
    int fifo, file;
    if(argc != 3)
        ERR("USAGE");

    if(mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
        if(errno != EEXIST)
            ERR("CREATE FIFO");

    if((fifo = open(argv[1], O_WRONLY)) < 0)
        ERR("FIFO OPEN");

    if((file = open(argv[2], O_RDONLY)) < 0)
        ERR("FILE OPEN");

    write_to_fifo(fifo, file);

    if(close(file) < 0)
        ERR("CLOSE FILE");

    if(close(fifo) < 0)
        ERR("CLOSE FIFO");

    return EXIT_SUCCESS;
}