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
#include "operations.h"
#include "operations.c"
#include "state.c"


#define PATHNAME ".pipe"
#define BUFFER_SIZE (128)
#define BOXCREATERESPONSE 1029

struct box
{
    char boxname[32];
    char pipename[256];
    int i; //0 se publisher 1 se subscriber
};

/* pthread_cond_t cond;
*/

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
/*
void *thr_func(void *ptr,pthread_mutex_t lock) {
    // first step: wait until the value is positive
    if (pthread_mutex_lock(&lock) != 0)
        exit(EXIT_FAILURE);

    // TO DO: wait g_value==0
    while(g_value == 0){
        pthread_cond_wait(&cond, &lock);
    }
    // second step: change the value
    fprintf(stdout, "[thread #%ld] read value=%d, will increment\n", pthread_self(),
           g_value);
    g_value++;

    if (pthread_mutex_unlock(&lock) != 0) {
        exit(EXIT_FAILURE);
    }

    return NULL;
} */

char *convert(uint8_t *a)
{
  char* buffer2;
  int i;

  buffer2 = malloc(9);
  if (!buffer2)
    return NULL;

  buffer2[8] = 0;
  for (i = 0; i <= 7; i++)
    buffer2[7 - i] = (((*a) >> i) & (0x01)) + '0';

  puts(buffer2);

  return buffer2;
}

void slice(const char *str, char *result, size_t start, size_t end)
{
    strncpy(result, str + start, end - start);
}

void register_publisher(char * pipename, char * boxname, struct box *userarray, int maxsessions){
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        fprintf(stderr, "Erro\n");
    } else{
        for (int a = 0; a < MAX_DIR_ENTRIES * maxsessions; a++) {
            if (userarray[a].i == -1){
                userarray[a].i = 0;
                memcpy(userarray[a].boxname, boxname, 32);
                memcpy(userarray[a].pipename, pipename, 256);
            }
        }
    }
}

void register_subscriber(char * pipename, char * boxname, struct box *userarray, int maxsessions){
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        fprintf(stderr, "Erro\n");
    } else{
        for (int a = 0; a < MAX_DIR_ENTRIES * maxsessions; a++) {
            if (userarray[a].i == -1){
                userarray[a].i = 1;
                memcpy(userarray[a].boxname, boxname, 32);
                memcpy(userarray[a].pipename, pipename, 256);
            }
        }
    }
}

void create_box(char * pipename, char * boxname) {
    int32_t return_code;
    char error_message[1024];
    if(tfs_lookup(boxname, ROOT_DIR_INUM) != -1){
        return_code = -1;
        memcpy(error_message, "Caixa existe", 1024);
    } else{
        return_code = 0;
        error_message[0] = '\0';
        int i = tfs_open(boxname, TFS_O_CREAT);
        if(tfs_close(i) == -1) {
            memcpy(error_message, "ERRO", 1024);
        }
    }
    int rx = open(pipename, O_WRONLY);
    char creturn_code[4];
    sprintf( creturn_code, "%I64u", return_code);
    uint8_t code = 4;
    char *ccode = convert(&code);
    char message[BOXCREATERESPONSE];
    memcpy(message,ccode, 1);
    memcpy(message, creturn_code, 4);
    memcpy(message, error_message, 1024);
    send_msg(rx, message);
    close(rx);
}

void remove_box(char * pipename, char * boxname) {
    int32_t return_code;
    char error_message[1024];
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        return_code = -1;
        memcpy(error_message, "Caixa nao existe", 1024);
    } else{
        return_code = 0;
        error_message[0] = '\0';
        if (tfs_unlink(boxname) == -1) {
            memcpy(error_message, "ERRO", 1024);
        }
    }
    int rx = open(pipename, O_WRONLY);
    char creturn_code[4];
    sprintf( creturn_code, "%I64u", return_code);
    uint8_t code = 6;
    char *ccode = convert(&code);
    char message[BOXCREATERESPONSE];
    memcpy(message,ccode, 1);
    memcpy(message, creturn_code, 4);
    memcpy(message, error_message, 1024);
    send_msg(rx, message);
    close(rx);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "error\n");
    }
    char * pipename ;
    pipename = malloc(sizeof(char)*strlen(argv[1]));
    strcpy(pipename,argv[1]);
    strcat(pipename, PATHNAME);
    int max_sessions = atoi(argv[2]);
    struct box userarray[max_sessions * MAX_DIR_ENTRIES];
    for (int g = 0; g < max_sessions * MAX_DIR_ENTRIES; g++) {
        userarray[g].i = -1;
        userarray[g].boxname[0] = '\0';
        userarray[g].pipename[0] = '\0';
    }
    if (unlink(pipename) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipename,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(pipename, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    /* pthread_mutex_t trinco[max_sessions];
    pthread_cond_init(&cond, NULL);
    pthread_t tid[max_sessions];
    for (int i = 0; i < max_sessions; i++) {
        int error_num = pthread_create(&tid[i], NULL, NULL, NULL);
        //pthread_mutex_init(&trinco[i], NULL);
        if (error_num != 0) {
            fprintf(stderr, "error creating thread: strerror(%s)\n", strerror(error_num));
            return -1;
        }
    } */
    int rx = open(pipename, O_RDONLY);
    if (rx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    while (true) {
        char buffer[BUFFER_SIZE];
        ssize_t ret = read(rx, buffer, BUFFER_SIZE - 1);
        if (ret == 0) {
            // ret == 0 indicates EOF
            fprintf(stderr, "[INFO]: pipe closed\n");
            return 0;
        } else if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        buffer[ret] = 0;
        char *ccode;
        char client_pipename[256];
        char box_name[32];
        slice(buffer, ccode, 0, 1);
        uint8_t code = (uint8_t) atoi(ccode);
        slice(buffer, pipename, 1, 257);
        slice(buffer, box_name, 257, 289);
        switch (code){
        case(1):{
            register_publisher(client_pipename,box_name, userarray, max_sessions);
            break;
        };
        case(2):{
            register_subscriber(client_pipename,box_name, userarray, max_sessions);
            break;
        }
        case(3):{
            create_box(client_pipename, box_name);
            break;
        }
        case(5):{
            remove_box(client_pipename, box_name);
            break;
        }
        case(7):{
            break;
        }
        default:
            fprintf(stderr, "Erro\n");
        }
    }
    close(rx);
    fprintf(stderr, "usage: mbroker <pipename>\n");
    return -1;
}
