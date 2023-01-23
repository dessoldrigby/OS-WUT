#define _GNU_SOURCE
#define _UERRORS
#define _URND

#define BUFF_SIZE 1024


#include "utils.h"

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

typedef unsigned int uint;
typedef struct timespec timespec_t;

#define _TEST_

#ifdef _TEST4_

#define DEFAULT_S 10

#define ELAPSED(stard, end) \
    ((end).tv_sec - (start).tv_sec + ((end).tv_nsec - (start).tv_nsec) * 1.0e-9)

typedef struct _student_list {
    bool* removed;
    pthread_t *th_students;
    int count;
    int present;

} student_list_t;

typedef struct _year_counters {
    int years[4];
    pthread_mutex_t years_mutexes[4];

} year_counters_t;

typedef struct _args_modify {
    year_counters_t* year_counters;
    int year;

} args_modify_t;

void read_arguments(int argc, char** argv, int* student_out)
{
    *student_out = 10;
    if(argc >= 2) {
        *student_out = atoi(argv[1]);
        if(*student_out <= 0 || *student_out >= 100)
            ERR("0 <= S <= 100");
    }
}

void increment_counter(args_modify_t* args)
{
    pthread_mutex_lock(&(args->year_counters->years_mutexes[args->year]));
    {
       args->year_counters->years[args->year] += 1;
    }
    pthread_mutex_unlock(&(args->year_counters->years_mutexes[args->year]));
}

void decrement_counter(void *void_args)
{
    args_modify_t *args = (args_modify_t*)void_args;

    pthread_mutex_lock(&(args->year_counters->years_mutexes[args->year]));
    {
       args->year_counters->years[args->year] -= 1;
    }
    pthread_mutex_unlock(&(args->year_counters->years_mutexes[args->year]));
}

void msleep(uint miliseconds)
{
    time_t sec = (int) (miliseconds / 1000);
    miliseconds = miliseconds - (sec * 1000);

    timespec_t req = { 0 };
    req.tv_nsec = sec;
    req.tv_nsec = miliseconds * 1000000L;
    if(nanosleep(&req, &req))
        ERR("nanosleep");
}

void* student_life_routine(void *void_arg) 
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    args_modify_t arg;
    arg.year_counters = void_arg;

    for(arg.year = 0; arg.year < 3; arg.year++) {
        increment_counter(&arg);
        pthread_cleanup_push(decrement_counter, &arg);
        msleep(1000);
        pthread_cleanup_pop(1);
    }
    increment_counter(&arg);

    return NULL;
}

void kick_student(student_list_t* list)
{
    int idx;
    if(0 == list->present)
        return;

    do {
        idx = rand() % list->count;
    } while (list->removed[idx] == true);

    // printf("kiking st num: %d, stud tid: %d\n", idx, (int)list->th_students[idx]);
    if(pthread_cancel(list->th_students[idx]))
        ERR("CANCEL");
    list->removed[idx] = true;
    list->present -= 1;
}

int main(int argc, char* argv[])
{
    int student_count;
    read_arguments(argc, argv, &student_count);
    
    year_counters_t counters = {
        .years = { 0 },
        .years_mutexes = {
            PTHREAD_MUTEX_INITIALIZER, 
            PTHREAD_MUTEX_INITIALIZER,
            PTHREAD_MUTEX_INITIALIZER,
            PTHREAD_MUTEX_INITIALIZER 
        }
    };

    student_list_t student_list = {
        .count = student_count,
        .present = student_count,
        .th_students = (pthread_t*)malloc(sizeof(pthread_t) * student_count),
        .removed = (bool*)malloc(sizeof(bool) * student_count)
    };
    if(NULL == student_list.th_students || NULL == student_list.removed)
        ERR("MALLOC");

    for(int i = 0; i < student_count; i++)
        student_list.removed[i] = false;

    for(int i = 0; i < student_count; i++)
        if(pthread_create(&student_list.th_students[i], NULL, student_life_routine, &counters))
            ERR("CREATE");

    srand(time(NULL));
    timespec_t start, current;

    if(clock_gettime(CLOCK_REALTIME, &start))
        ERR("CLOCK");

    do {
        msleep(rand() % 201 + 100);
        if(clock_gettime(CLOCK_REALTIME, &current))
            ERR("CLOCK");

        kick_student(&student_list);
    } while (ELAPSED(start, current) < 4.0);

    for(int i = 0; i < student_count; i++)
        if(pthread_join(student_list.th_students[i], NULL))
            ERR("JOIN");

    printf("First year:\t %d\n", counters.years[0]);
    printf("Second year:\t %d\n", counters.years[1]);
    printf("Third year:\t %d\n", counters.years[2]);
    printf("Engineers year:\t %d\n", counters.years[3]);

    free(student_list.th_students);
    free(student_list.removed);
    return EXIT_SUCCESS;   
}

#endif

#ifdef _TEST3_

#define MAX_LINE 1024
#define DEFAULT_K 10
#define DELETED_ITEM -1

typedef struct _arg_sig_hanlder {
    pthread_t tid;
    int *array_count;
    int *array;
    pthread_mutex_t *array_mutex;

    sigset_t *mask;
    
    bool *quit_flag;
    pthread_mutex_t *quit_flag_mutex;
} arg_sig_hanlder_t;

void read_arguments(int argc, char* argv[], int *array_size)
{
    *array_size = DEFAULT_K;
    if(argc >= 2) {
        *array_size = atoi(argv[1]);
        if(*array_size <= 0)
            ERR("K");
    }
}

void remove_item(int* arr, int* size, int idx)
{
    int curr_idx = -1;
    int i = -1;
    while (curr_idx != idx) {
        i++;
        if(arr[i] != DELETED_ITEM) {
            curr_idx++;
        }
    }

    arr[i] = DELETED_ITEM;
    *size -= 1;
}

void* sig_handling_routine(void* void_arg) 
{
    arg_sig_hanlder_t *arg = (arg_sig_hanlder_t*)void_arg;
    int sig_no;
    srand(time(NULL));

    while(true) {
        if(sigwait(arg->mask, &sig_no))
            ERR("SIGWAIT");
        
        switch (sig_no) {
            case SIGINT:
                pthread_mutex_lock(arg->array_mutex);
                {
                    if(arg->array_count > 0)
                        remove_item(arg->array, arg->array_count, rand() % (*arg->array_count));
                }
                pthread_mutex_unlock(arg->array_mutex);
                break;
            case SIGQUIT:
                pthread_mutex_lock(arg->quit_flag_mutex);
                {
                    *arg->quit_flag = true;
                }
                pthread_mutex_unlock(arg->quit_flag_mutex);
                return NULL;
            default:
                printf("unexpected signal %d\n", sig_no);
                exit(EXIT_FAILURE);
        }
    }
}

void print_array(int* arr, int size)
{
    printf("[");
    for(int i = 0; i < size; i++)
       if(arr[i] != DELETED_ITEM)
            printf(" %d,", arr[i]);

    printf(" ]\n");
}

int main(int argc, char* argv[])
{
    int array_size, *array;
    bool quit_flag = false;
    pthread_mutex_t array_mutex = PTHREAD_MUTEX_INITIALIZER, quit_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

    read_arguments(argc, argv, &array_size);

    int array_count = array_size;
    if(NULL == (array = (int*)malloc(sizeof(int) * array_size)))
        ERR("MALLOC");

    for(int i = 0; i < array_size; i++)
       array[i] = i + 1;

    sigset_t oldmask, newmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    sigaddset(&newmask, SIGQUIT);
    if(pthread_sigmask(SIG_BLOCK, &newmask, &oldmask))
        ERR("SIGBLOCK");

    arg_sig_hanlder_t arg = {
        .array = array,
        .array_mutex = &array_mutex,
        .quit_flag = &quit_flag,
        .quit_flag_mutex = &quit_flag_mutex,
        .array_count = &array_count,
        .mask = &newmask
    };

    if(pthread_create(&arg.tid, NULL, sig_handling_routine, &arg))
        ERR("CREATE");

    while(true) {
        pthread_mutex_lock(&quit_flag_mutex);
        if(quit_flag) {
            pthread_mutex_unlock(&quit_flag_mutex);
            break;
        }

        pthread_mutex_unlock(&quit_flag_mutex);
        pthread_mutex_lock(&array_mutex);
        {
            print_array(array, array_size);
        }
        pthread_mutex_unlock(&array_mutex);
        sleep(1);
    }

    if(pthread_join(arg.tid, NULL))
        ERR("JOIN");

    free(array);

    if(pthread_sigmask(SIG_UNBLOCK, &newmask, &oldmask))
        ERR("SIGMASK");

    return EXIT_SUCCESS;
}

#endif

#ifdef _TEST2_

#define MAX_LINE 4096
#define DEFAULT_N 1000
#define DEFAULT_K 10
#define BIN_COUNT 11

typedef struct _args_thrower {
    pthread_t tid;
    uint seed;
    int *balls_thrown;
    int *balls_waiting;
    int *bins;
    pthread_mutex_t *balls_waiting_mutex;
    pthread_mutex_t *balls_thrown_mutex;
    pthread_mutex_t *bins_mutex;

    int num;
    bool *finished_flags;

} args_thrower_t;



void read_arguments(int argc, char *argv[], int *balls_count, int *throwers_count)
{
    *balls_count = DEFAULT_N;
    *throwers_count = DEFAULT_K;

    if(argc >= 2) {
        *balls_count = atoi(argv[1]);
        if(*balls_count <= 0)
            ERR("INVALID BALLS COUNT PARAMETER");
    }

    if(argc >= 3) {
        *throwers_count = atoi(argv[2]);
        if(*throwers_count <= 0)
            ERR("INVALID THROWERS COUNT PARAMETER");
    }
}

int throw_ball(uint *seedptr)
{
    int res = 0;
    
    for (int i = 0; i < BIN_COUNT - 1; i++) {
        if(NEXT_DOUBLE_T(seedptr) > 0.5)
            res++;
    }

    return res;
}

void* throwing_routine(void *args)
{
    args_thrower_t *arg = args;

    while(1) {
        pthread_mutex_lock(arg->balls_waiting_mutex);
        {
            if(*arg->balls_waiting > 0) {
                *arg->balls_waiting -= 1;
                pthread_mutex_unlock(arg->balls_waiting_mutex);
            } 
            else {
                pthread_mutex_unlock(arg->balls_waiting_mutex);
                break;
            }
        }

        int bin_no = throw_ball(&arg->seed);

        pthread_mutex_lock(&arg->bins_mutex[bin_no]);
        {
            arg->bins[bin_no] += 1;
        }
        pthread_mutex_unlock(&arg->bins_mutex[bin_no]);

        pthread_mutex_lock(arg->balls_thrown_mutex);
        {
            *arg->balls_thrown += 1;
        }
        pthread_mutex_unlock(arg->balls_thrown_mutex);
    }

    struct timespec t = { RND_T(&arg->seed, 3, 10), 0 };
    nanosleep(&t, NULL);

    arg->finished_flags[arg->num] = true;

    return NULL;
}

void make_throwers(args_thrower_t *args_array, int throwers_count)
{
    pthread_attr_t thread_attr;

    if(pthread_attr_init(&thread_attr))
        ERR("pthread_attr_init");

    if(pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED))
        ERR("pthread_attr_setdetachstate");

    for (int i = 0; i < throwers_count; i++) {
        if(pthread_create(&(args_array[i].tid), &thread_attr, throwing_routine, &args_array[i]))
            ERR("pthread_create");
    }
}




int main(int argc, char* argv[])
{
    int balls_count, throwers_count;
    read_arguments(argc, argv, &balls_count, &throwers_count);

    int balls_thrown = 0, bt = 0;
    pthread_mutex_t balls_thrown_mutex = PTHREAD_MUTEX_INITIALIZER;

    int balls_waiting = balls_count;
    pthread_mutex_t balls_waiting_mutex = PTHREAD_MUTEX_INITIALIZER;

    int bins[BIN_COUNT];
    pthread_mutex_t bins_mutex[BIN_COUNT];
    for(int i = 0; i < BIN_COUNT; i++) {
        bins[i] = 0;
        if(pthread_mutex_init(&bins_mutex[i], NULL))
            ERR("pthread_mutex_init");
    }

    args_thrower_t *args = (args_thrower_t*)malloc(sizeof(args_thrower_t) * throwers_count);
    if(NULL == args)
        ERR("MALLOC");

    srand(time(NULL));

    bool *finished_flags = (bool*)malloc(sizeof(bool) * throwers_count); 
    if(NULL == finished_flags)
        ERR("MALLOC");

    memset(finished_flags, false, sizeof(bool) * throwers_count);

    for(int i = 0; i < throwers_count; i++) {
        args[i].seed = (uint)rand();
        args[i].balls_thrown = &balls_thrown;
        args[i].balls_waiting = &balls_waiting;
        args[i].bins = bins;
        args[i].balls_thrown_mutex = &balls_thrown_mutex;
        args[i].balls_waiting_mutex = &balls_waiting_mutex;
        args[i].bins_mutex = bins_mutex;
        args[i].finished_flags = finished_flags;
        args[i].num = i;
    }

    make_throwers(args, throwers_count);

    while(bt < balls_count) {
        sleep(1);

        pthread_mutex_lock(&balls_thrown_mutex);
        {
            bt = balls_thrown;
        }
        pthread_mutex_unlock(&balls_thrown_mutex);
    }

    int real_balls_count = 0;
    double mean_value = 0.0;

    for(int i = 0; i < BIN_COUNT; i++) {
        real_balls_count += bins[i];
        mean_value += bins[i] * i;
    }

    int fin = 0;
    struct timespec t = { 0, 1000000 };
    while(fin < throwers_count) {
        printf("Main Thread awaiting {%d} subthreads\n", throwers_count - fin);
    
        nanosleep(&t, NULL);

        for(int i = 0; i < throwers_count; i++)
            fin += (int)finished_flags[i];
    }

    mean_value = mean_value / real_balls_count;
    printf("Bins count:\n");

    for (int i = 0; i < BIN_COUNT; i++) {
        printf("%d\t", bins[i]);
    }  

    printf("Total ball count:\t%d\nMean value:\t%f\n", real_balls_count, mean_value);

    free(finished_flags);
    free(args);

    return EXIT_SUCCESS;
}

#endif

#ifdef _TEST1_

#define DEFAULT_THREADCOUNT 10
#define DEFAULT_SAMPLESIZE 100

typedef struct _args_estimation {
    pthread_t tid;
    uint seed;
    int samples_count;

} args_estimation_t;



void read_arguments(int argc, char *argv[], int *thread_count, int *samples_count)
{
    *thread_count = DEFAULT_THREADCOUNT;
    *samples_count = DEFAULT_SAMPLESIZE;

    if(argc >= 2) {
        *thread_count = atoi(argv[1]);
        if(*thread_count <= 0)
            ERR("WRONG THREAD COUNT PARAMETER");
    }
    if(argc >= 3) {
        *samples_count = atoi(argv[2]);
        if(*samples_count <= 0 || *samples_count >= 200)
            ERR("WRONG SAMPES COUNT PARAMETER");
    }
}

void* pi_estimation(void *void_args)
{
    args_estimation_t *args = void_args;
    double *res;

    if(NULL == (res = (double*)malloc(sizeof(double))))
        ERR("MALLOC");

    int inside_count = 0;

    for (int i = 0; i < args->samples_count; i++) {
        double x = ((double)rand_r(&args->seed) / (double)RAND_MAX);
        double y = ((double)rand_r(&args->seed) / (double)RAND_MAX);

        if(sqrt(x * x + y * y) <= 1.0)
            inside_count++;
    }

    *res = 4.0 * (double)inside_count / (double)args->samples_count;

    return res;
}



int main(int argc, char *argv[])
{
    int thread_count, samples_count;
    double *sub_res;

    read_arguments(argc, argv, &thread_count, &samples_count);

    args_estimation_t *estimations = 
        (args_estimation_t*)malloc(sizeof(args_estimation_t) * thread_count);
    if(NULL == estimations)
        ERR("MALLOC");

    srand(time(NULL));

    for (int i = 0; i < thread_count; i++) {
        estimations[i].seed = rand();
        estimations[i].samples_count = samples_count;
    }

    for (int i = 0; i < thread_count; i++) {
        if(0 != pthread_create(&(estimations[i].tid), NULL, pi_estimation, &estimations[i]))
            ERR("THREAD CREATION");
    }

    double cumulative_res = 0.0;

    for (int i = 0; i < thread_count; i++) {
        if(0 != pthread_join(estimations[i].tid, (void*)&sub_res))
            ERR("THREAD JOIN");

        if(NULL != sub_res) {
            cumulative_res += *sub_res;
            free(sub_res);
        }
    }

    double res = cumulative_res / (double)thread_count;

    printf("PI ~= %f\n", res);

    free(estimations);
	return EXIT_SUCCESS;
}

#endif