#define _XOPEN_SOURCE 500

#define COMMAND_SIZE 8
#define BUFF_SIZE 128

int verb;

#include"utils.h"

#include<stdio.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<dirent.h>
#include<stdlib.h>
#include<string.h>
#include<ftw.h>
#include<errno.h>
#include<unistd.h>
#include<time.h>
#include<string.h>



void parse_commands();
void print_dir();
void remove_file(char *string);
void copy();
void copy_file(char *source, char *destination);
void copy_directory(char *source, char *destination);
void make_file(FILE* file, char *name, ssize_t size, mode_t perms);



int main(int argc, char** argv) {
    // Input check
    char dirname[BUFF_SIZE];
    char c;
    while (-1 != (c = getopt(argc, argv, "p:v"))) {
        switch (c) {
            case 'p':   // -p
                strcpy(dirname, optarg);

                break;
            case 'v':
                verb = 1;

                break;
            default:
                break;
        }
    }

    // switch directories
    if(0 != chdir(dirname)) ERR("Can't change folder");

    parse_commands();

    return 0;
}

void parse_commands() {
    char command[COMMAND_SIZE];
    char buffer[BUFF_SIZE];
    do {
        scanf("%s", command);

        if(strcmp(command, "ls") == 0) {
            print_dir();
        }
        else if (strcmp(command, "rm") == 0) {
            // scanf("%s", buffer);
            get_string(buffer, BUFF_SIZE, stdin);

            remove_file(buffer);
        }
        else if(strcmp(command, "cp") == 0) {
            //get_string(buffer, BUFF_SIZE, stdin);

            copy();
        }
        else if(strcmp(command, "exit") == 0) {
            if(verb) printf("\tExited after %s operation\n", command);

            break;
        }
        else if(strcmp(command, "cd") == 0) {
            scanf("%s", buffer);

            if(chdir(buffer)) {
                ERR("chdir");
            }
        }
    }
    while(/*strcmp(command, "exit") != 0*/ 1);
}

void copy_file(char *source, char *destination)
{
    FILE *fsource, *fdestination;
    struct stat fdest_stat;

    if(NULL == (fsource = fopen(source, "r"))) ERR("Can't open a source file");
        
    if(lstat(source, &fdest_stat)) ERR("Can't retrieve info about source file");

    umask(~fdest_stat.st_mode&0777);
    if(NULL == (fdestination = fopen(destination, "w+"))) ERR("Can't create/open a destination file");

    char c;
    while(EOF != (c = fgetc(fsource)))
    {
        fputc(c, fdestination);
    }

    if(fclose(fsource) || fclose(fdestination)) ERR("Can't close files");

    if(verb) printf("\tSuccessfully copied: %s to %s after cp %s %s operation\n", source, destination, source, destination);
}

void copy_directory(char *dir_path, char *target_path) 
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    char home_path[BUFF_SIZE];

    if (NULL == getcwd(home_path, BUFF_SIZE)) {
        ERR("getcwd");
    }
    if (chdir(dir_path)) {
        ERR("chdir");
    }

    if ((dirp = opendir(dir_path)) == NULL) {
        ERR("opendir");
    }

    mkdir(target_path, 0777);

    do {
        if (NULL != (dp = readdir(dirp))) {
            if (lstat(dp->d_name, &filestat)) {
                printf("lstat: %s\n", dp->d_name);
                ERR("lstat");
            }
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }  

            char to[BUFF_SIZE];
            strcpy(to, target_path);
            strcat(to, "/");
            strcat(to, dp->d_name);

            char from[BUFF_SIZE];
            strcpy(from, dir_path);
            strcat(from, "/");
            strcat(from, dp->d_name);
            if (S_ISDIR(filestat.st_mode)) {
                printf("directory\n");
                copy_directory(from, to);
            } else {
                printf("copying file\n");
                copy_file(from, to);
            }     
        }
    } while (NULL != dp);

    if (closedir(dirp)) {
        ERR("closedir");
    }

    if (chdir(home_path)) {
        ERR("chdir");
    }
}

void copy()
{
    char option[BUFF_SIZE];
    char source[BUFF_SIZE];
    char destination[BUFF_SIZE];

    //get_string(option, BUFF_SIZE, stdin);
    scanf("%s", option);
    if(strcmp(option, "\0") == 0) ERR("remove_file error");

    if(strcmp(option, "-R") != 0)
    {
        scanf("%s", destination);

        copy_file(option, destination);
    }
    else
    {
        scanf("%s", source);
        scanf("%s", destination);

        copy_directory(source, destination);
    }
}

void print_dir()
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;

    if(NULL == (dirp = opendir("."))) ERR("Opendir");
    
    do {
        errno = 0;
        if(NULL != (dp = readdir(dirp))) {
            if(lstat(dp->d_name, &filestat)) ERR("Lstat");

            if((S_ISREG(filestat.st_mode) || S_ISDIR(filestat.st_mode)) && strcmp(dp->d_name ,".") != 0 && strcmp(dp->d_name ,"..") != 0) {
                printf("\t%s\t%s", dp->d_name, ctime(&filestat.st_mtime));
            }
        }
    } 
    while(dp != NULL);

    if(errno != 0) ERR("Error encountered in readdir");
    if(closedir(dirp)) ERR("Closedir");
}

void remove_file(char *string)
{
    if(strcmp(string, "\0") == 0) ERR("remove_file error");

    char* token = strtok(string, " ");
    char rmres;
    while(NULL != token) {
        //printf("%s\n", token);

        if(0 != (rmres = unlink(token))) {
            printf("\tNo such file: [%s] can be removed\n", token);
        }

        if(verb && rmres == 0) {
            printf("\tSuccessfully removed: %s after rm %s operation\n", token, token);
        }

        token = strtok(NULL, " ");
    }
}
