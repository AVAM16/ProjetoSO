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

#define PATHNAME ".pipe"
#define CONCATNATOR " | "

void send_msg(int tx, char const *str) {
    size_t len = strlen(str);
    size_t written = 0;

    while (written < len) {
        ssize_t ret = write(tx, str + written, len - written);
        if (ret < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        written += ret;
    }
}

static void sigint_handler(int sig) {
    fprintf(stdout, "Caught SIGINT - that's all folks!\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
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
    int pipenamelength = strlen(argv[2]);
    for (int a = pipenamelength; a <= 256; a++) {
        pipename[a] = "\0";
    }
    strcpy(box_name,argv[3]);
    int boxnamelength = strlen(argv[3]);
    for (int b = boxnamelength; b <= 32; b++) {
        box_name[b] = "\0";
    }
    int rx = open(register_pipename, O_WRONLY);
    // [ code = 2 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
    uint8_t code = 2;
    char * message = malloc(3*sizeof(CONCATNATOR) + sizeof(uint8_t) + 256 +32);
    strcpy(message, code);
    strcpy(message, CONCATNATOR);
    strcpy(message, pipename);
    strcpy(message, CONCATNATOR);
    strcpy(message, box_name);
    send_msg(rx, message);
    close(rx);
    for (;;) { // Loop forever, waiting for signals
        pause(); // Block until a signal is caught
    }
    fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
    return -1;
}
