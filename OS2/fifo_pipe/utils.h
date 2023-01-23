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
#include <stdbool.h>



//#define _UALL // for tests

#ifdef _UALL

#define _UERRORS
#define _URND
#define _STRSTACK
#define _USTR
#define _UFILES

#endif

#if defined(_USTR) && !defined(_USTRSTACK)
#define _USTRSTACK
#endif



#ifdef _UERRORS 

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

#endif



#ifdef _URND

#define RND(min, max) (min + rand() % (max - min + 1))
#define RND_T(seedptr, min, max) (min + rand_r(seedptr) % (max - min + 1))

#define NEXT_DOUBLE() ((double)rand() / (double)RAND_MAX)
#define NEXT_DOUBLE_T(seedptr) ((double)rand_r(seedptr) / (double)RAND_MAX)

#endif



#ifdef _USTRSTACK

// ----------------------------------------------------------------------------------------
// string stack decl

typedef struct _string_node {
    char* str;
    int size;
    struct _string_node* next;

} strnode_t;

strnode_t* create_strnode(char* value, int size);
void dfstrnode(strnode_t* node);

typedef struct _string_stack {
    int size;
    struct _string_node* head;

} strstack_t;

// creates an empty string stack.
strstack_t create_strstack();
// destroys all nodes and clears the string stack.
void clear_strstack(strstack_t* stack);

bool is_strstack_empty(strstack_t* stack);
// adds the *value* to the stack, if the *size* parameter
// is <= 0, the imput string is copied fully, othervise
// only *size* bytes will be copied
void strstack_push(strstack_t* stack, char* value, int size);
// returns the pointer to a string taken off the stack,
// the string *size* can be retrieved from the second parameter,
// if passed as NULL, nothing happen
char* strstack_pop(strstack_t* stack, int* out_size);
void for_stack(strstack_t* stack, void (*fun)(char*, int));

// end decl
// ----------------------------------------------------------------------------------------

#endif



#ifdef _USTR

void get_string(char* str, int size, FILE* stream);
void split_string(char* str, char* delim, strstack_t* out_strings);

#endif



#ifdef _UFILES

// ERROR: if(bulk_close(par)) != 0
int bulk_close(int fd);

// ERROR: if((read = bulk_read(...)) < 0)
ssize_t bulk_read(int fd, char* buf, size_t count);

// ERROR: if((read = bulk_write(...)) < 0)
ssize_t bulk_write(int fd, char* buf, size_t count);

#endif



#endif