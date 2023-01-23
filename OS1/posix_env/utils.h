#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void get_string(char* str, int size, FILE* stream);
void split_string(char* str, char* delim, char** splited_arr, int* size);




#endif