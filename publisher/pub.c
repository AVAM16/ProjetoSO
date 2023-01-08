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

static void EOF_handler() {
    fprintf(stderr, "Caught EOF - that's all folks!\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (signal(EOF, EOF_handler) == SIG_ERR) {
        exit(EXIT_FAILURE);
    }
    if (argc != 4) {
        fprintf(stderr, "error\n");
    }
    char * register_pipename ;
    char pipename[256];
    char box_name[32];
    register_pipename = malloc(sizeof(char)*strlen(argv[1]));
    strcpy(register_pipename,argv[1]);
    strcat(register_pipename, PATHNAME);
    strcpy(pipename,argv[2]);
    int pipenamelength = (int) strlen(argv[2]);
    pipename[pipenamelength + 1] = '\0';
    strcpy(box_name,argv[3]);
    int boxnamelength = (int) strlen(argv[3]);
    box_name[boxnamelength] = '\0';
    int rx = open(register_pipename, O_WRONLY);
    // [ code = 1 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
    uint8_t code = 1;
    char message[MESSAGELENGTH];
    sprintf(message, "%x | %s | %s", code, pipename, box_name);
    send_msg(rx, message);
    close(rx);
    for (;;) { // Loop forever, waiting for signals
        pause(); // Block until a signal is caught
    }
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
    return -1;
}
