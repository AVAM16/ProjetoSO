#include "logging.h"
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

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
    if (argc == 4) {
        char * register_pipename ;
        char * pipename;
        register_pipename = malloc(sizeof(char)*strlen(argv[1]));
        strcpy(register_pipename,argv[1]);
        strcat(register_pipename, PATHNAME);
        pipename = malloc(sizeof(char)*strlen(argv[2]));
        strcpy(pipename,argv[2]);
        strcat(pipename, PATHNAME);
    } else if(argc == 5){
        char * namefour;
        namefour = malloc(sizeof(char)*strlen(argv[4]));
        char * register_pipename ;
        char * pipename;
        register_pipename = malloc(sizeof(char)*strlen(argv[1]));
        strcpy(register_pipename,argv[1]);
        strcat(register_pipename, PATHNAME);
        pipename = malloc(sizeof(char)*strlen(argv[2]));
        strcpy(pipename,argv[2]);
        strcat(pipename, PATHNAME);
        if(strcmp(namefour, "create") == 0) {
            
        }else{

        }
    } else{
        print_usage();
    }
    WARN("unimplemented"); // TODO: implement
    return -1;
}
