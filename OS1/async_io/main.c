#define _GNU_SOURCE
#define _UERRORS
#define _URND

#define BUFF_SIZE 1024

#include "utils.h"

#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <aio.h>

#define BLOCKS 3

#define SHIFT(counter, x) ((counter + x) % BLOCKS)


volatile sig_atomic_t work;

void sigint_handler(int sig_no)
{
    work = 0;
}

void sethandler(void (*f)(int), int sig)
{
	struct sigaction sa;
	memset(&sa, 0x00, sizeof(struct sigaction));
	sa.sa_handler = f;
	if (sigaction(sig, &sa, NULL) == -1)
		ERRS("Error setting signal handler");
}

off_t get_file_len(int fd)
{
    struct stat buf;
    if(-1 == fstat(fd, &buf))
        ERR("FSTAT");

    return buf.st_size;
}

void suspend(struct aiocb* aiocbs)
{
    struct aiocb *aiolist[1];
    aiolist[0] = aiocbs;
    if(!work)
        return;
    
    while(-1 == aio_suspend((const struct aiocb *const *)aiolist, 1, NULL))
    {
        if(!work)
            return;

        if(errno == EINTR)
            continue;

        ERR("SUSPEND");
    }

    if(aio_error(aiocbs) != 0)
        ERR("SUSPEND");

    if(aio_return(aiocbs))
        ERR("RETURN ERROR");
}

void fill_aio_structs(struct aiocb* aiocbs, char** buffer, int fd, int block_size)
{
    for(int i = 0; i < BLOCKS; i++) {
        memset(&aiocbs[i], 0, sizeof(struct aiocb));
        aiocbs[i].aio_fildes = fd;
        aiocbs[i].aio_offset = 0;
        aiocbs[i].aio_nbytes = block_size;
        aiocbs[i].aio_buf = (void*)buffer[i];
        aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    }
}

void read_data(struct aiocb* aiocbs, off_t offset)
{
    if(!work)
        return;
    
    aiocbs->aio_offset = offset;
    if(-1 == aio_read(aiocbs))
        ERR("READ");
}

void write_data(struct aiocb* aiocbs, off_t offset)
{
    if(!work)
        return;
    
    aiocbs->aio_offset = offset;
    if(-1 == aio_write(aiocbs))
        ERR("READ");
}

void sync_data(struct aiocb* aiocbs)
{
    if(!work)
        return;

    suspend(aiocbs);
    if(-1 == aio_fsync(O_SYNC, aiocbs))
        ERR("SYNC");
    
    suspend(aiocbs);
}

void get_indexes(int *indexes, int max)
{
    indexes[0] = RND(0, max);
    indexes[1] = RND(0, max);
    if(indexes[1] >= indexes[0])
        indexes[1]++;
}

void cleanup(char** buffers, int fd)
{
    if(!work) 
        if(-1 == aio_cancel(fd, NULL))
            ERR("CANCEL");

    for(int i = 0; i < BLOCKS; i++)
        free(buffers[i]);

    if(-1 == TEMP_FAILURE_RETRY(fsync(fd)))
        ERR("FSYNC");
}

void reverse_buffer(char* buffer, int block_size)
{
    char tmp;
    for(int i = 0; i < block_size && work; i++) {
        tmp = buffer[i];
        buffer[i] = buffer[block_size - i - 1];
        buffer[block_size - i - 1] = tmp;
    }
}

void process_blocks(
    struct aiocb* aiocbs, char** buffer, int bcount, int bsize, int iter
)
{
    int currpos, index[2];
    iter--;

    currpos = iter == 0 ? 1 : 0;

    read_data(&aiocbs[1], bsize * (rand() % bcount));
    suspend(&aiocbs[1]);

    for(int i = 0; i < iter && work; i++) {
        get_indexes(index, bcount);

        if(i > 0)
            write_data(&aiocbs[currpos], index[0] * bsize);

        if(i < iter - 1)
            read_data(&aiocbs[SHIFT(currpos, 2)], index[1] * bsize);
    
        reverse_buffer(buffer[SHIFT(currpos, 2)], bsize);

        if(i > 0)
            sync_data(&aiocbs[currpos]);

        if(i < iter - 1)
            suspend(&aiocbs[SHIFT(currpos, 2)]);

        currpos = SHIFT(currpos, 1);
    }

    if(0 == iter)
        reverse_buffer(buffer[currpos], bsize);
        
    write_data(&aiocbs[currpos], bsize * (rand() % bcount));
    suspend(&aiocbs[currpos]);
}

int main(int argc, char* argv[]) 
{
    char *fname, *buffer[BLOCKS];
    int fd, n, k, block_size;
    struct aiocb aiocbs[4];

    if(argc != 4)
        ERR("ARGS");

    fname = argv[1];
    n = atoi(argv[2]);
    k = atoi(argv[3]);

    if(n < 2 || k < 1)
        return EXIT_FAILURE;

    work = 1;

    sethandler(sigint_handler, SIGINT);
    if(-1 == (fd = TEMP_FAILURE_RETRY(open(fname, O_RDWR))))
        ERR("READ");

    block_size = (get_file_len(fd) - 1) / n;

    fprintf(stderr, "bsize: %d\n", block_size);

    if(block_size > 0) {
        for(int i = 0; i < BLOCKS; i++)
            if(NULL == (buffer[i] = (char*)calloc(block_size, sizeof(char))))

        fill_aio_structs(aiocbs, buffer, n, block_size);
        srand(time(NULL));
        process_blocks(aiocbs, buffer, n, block_size, k);
        cleanup(buffer, fd);
    }

    if(-1 == TEMP_FAILURE_RETRY(close(fd)))
        ERR("CLOSE");

    return EXIT_SUCCESS;
}