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
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PATHNAME ".pipe"
#define MESSAGELENGTH 295
#define MESSAGELISTLENGTH 260
static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

void send_msg(int tx, char const *str) {
    size_t len = strlen(str);
    size_t written = 0;

    while (written < len) {
        ssize_t ret = write(tx, str + written, len - written);
        if (ret < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        written += (size_t) ret;
    }
}

int main(int argc, char **argv) {
    char * register_pipename ;
    char pipename[256];
    register_pipename = malloc(sizeof(char)*strlen(argv[1]));
    strcpy(register_pipename,argv[1]);
    strcat(register_pipename, PATHNAME);
    strcpy(pipename,argv[2]);
    int pipenamelength = (int) strlen(argv[2]);
    pipename[pipenamelength + 1] = '\0';
    if (argc == 4) {
        int rx = open(register_pipename, O_WRONLY);
        uint8_t code = 7;
        char message[MESSAGELISTLENGTH];
        sprintf(message, "%x | %s", code, pipename);
        send_msg(rx, message);
        close(rx);
    } else if(argc == 5){
        char * namefour;
        namefour = malloc(sizeof(char)*strlen(argv[4]));
        char box_name[32];
        strcpy(box_name,argv[4]);
        int boxnamelength = (int) strlen(argv[4]);
        box_name[boxnamelength + 1] = '\0';
        int rx = open(register_pipename, O_WRONLY);
        if(strcmp(namefour, "create") == 0) {
            uint8_t code = 3;
            char message[MESSAGELENGTH];
            sprintf(message, "%x | %s | %s", code, pipename, box_name);
            send_msg(rx, message);
        }else{
            uint8_t code = 5;
            char message[MESSAGELENGTH];
            sprintf(message, "%x | %s | %s", code, pipename, box_name);
            send_msg(rx, message);
        }
        close(rx);
    } else{
        print_usage();
    }
    for (;;) {
        sleep(1);
    }
    return -1;
}
