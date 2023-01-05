#include "logging.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>

#define PATHNAME ".pipe"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "error\n");
    }
    char * register_pipename ;
    char * pipename;
    char * box_name;
    register_pipename = malloc(sizeof(char)*strlen(argv[1]));
    strcpy(register_pipename,argv[1]);
    strcat(register_pipename, PATHNAME);
    pipename = malloc(sizeof(char)*strlen(argv[2]));
    strcpy(pipename,argv[2]);
    strcat(pipename, PATHNAME);
    box_name = malloc(sizeof(char)*strlen(argv[3]));
    strcpy(box_name,argv[3]);
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
    return -1;
}
