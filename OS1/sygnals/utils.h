#ifndef UTILS_H
#define UTILS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source),\
                     exit(EXIT_FAILURE), \
                     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))

#define ERRC(source, handle) (perror(source),\
                     !handle ? \
                     exit(EXIT_FAILURE), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__) : \
                     fprintf(stderr, "%s:%d", __FILE__, __LINE__))

#define ERRS(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
                      perror(source),\
                      kill(0, SIGKILL),\
                      exit(EXIT_FAILURE))

#define RND(min, max) (min + rand() % (max - min + 1))

void get_string(char* str, int size, FILE* stream);

ssize_t bulk_read(int fd, char* buf, size_t count);
ssize_t bulk_write(int fd, char* buf, size_t count);

#endif