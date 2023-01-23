#include "utils.h"

void get_string(char* str, int size, FILE* stream)
{
    // fgets writes the \n symbol after the finishing of an input operation which leads
    // to a problem with recognizing correctly a file name
    fgets(str, size, stream);

    str[strlen(str) - 1] = '\0';
}

void split_string(char* str, char* delim, char** splited_arr, int* size) {
    char* token = strtok(str, delim);
    int sz = 0;
    char** res = malloc(sizeof(char*) * (sz + 1));

    while(NULL != token) {
        res[sz] = strcpy(res[sz], token);

        sz++;
        token = strtok(NULL, delim);
    }

    splited_arr = res;
    *size = sz;
}