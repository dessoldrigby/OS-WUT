#include "utils.h"

void get_string(char* str, int size, FILE* stream)
{
    // fgets writes the \n symbol after the finishing of an input operation which leads
    // to a problem with recognizing correctly a file name
    fgets(str, size, stream);

    str[strlen(str) - 1] = '\0';
}

ssize_t bulk_read(int fd, char* buf, size_t count) {
    ssize_t c, len = 0;

    do {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0) return c;
        if(0 == c) return len; // \EOF

        buf += c;
        len += c;
        count -= c;
    } while (count > 0);

    return len;
}
ssize_t bulk_write(int fd, char* buf, size_t count) {
    ssize_t c, len = 0;

    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0) return c;

        buf += c;
        len += c;
        count -= c;
    } while (count > 0);

    return len;
}