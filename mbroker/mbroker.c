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
#include "producer-consumer.c"
#include "producer-consumer.h"


#define PATHNAME ".pipe"
#define BUFFER_SIZE (289)
#define BOXCREATERESPONSE 1029
#define PIPENAME_SIZE 256
#define BOXNAME_SIZE 32
#define ERROR_MESSAGE_SIZE 1024

typedef struct
{
    char boxname[BOXNAME_SIZE];
    char pipename[PIPENAME_SIZE];
    int i; //0 se publisher 1 se subscriber
}box;

/* pthread_cond_t cond;
*/
int sessions = 0;
pthread_t *tid;
box *userarray;
pthread_mutex_t userarraylock = PTHREAD_MUTEX_INITIALIZER;


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
    memcpy(result, str + start, end - start);
}

void *threadfunction(){
    int b = 0;
    for (int a = 0; a < MAX_DIR_ENTRIES ; a++) {
        if (userarray[a].i != -1){
            b = a;
            break;
        }
    }
    if (userarray[b].i == 0){
        int rx = open(userarray[b].pipename, O_RDONLY);
        while(true){
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
        }
    } else {
        int rx = open(userarray[b].pipename, O_WRONLY);

    }
    return;
}

void register_publisher(char * pipename, char * boxname){
    pthread_mutex_lock(&userarraylock);
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        fprintf(stderr, "Erro\n");
    } else{
        for (int a = 0; a < MAX_DIR_ENTRIES ; a++) {
            if (userarray[a].i == -1){
                userarray[a].i = 0;
                memcpy(userarray[a].boxname, boxname, BOXNAME_SIZE);
                memcpy(userarray[a].pipename, pipename, PIPENAME_SIZE);
                sessions++;
                break;
            } else if(userarray[a].boxname == boxname && userarray[a].i == 0){
                fprintf(stderr,"Erro, Box já está associada com um Publisher");
            }
        }
    }
    pthread_mutex_unlock(&userarraylock);

}

void register_subscriber(char * pipename, char * boxname){
    pthread_mutex_lock(&userarraylock);
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        fprintf(stderr, "Erro\n");
    } else{
        for (int a = 0; a < MAX_DIR_ENTRIES ; a++) {
            if (userarray[a].i == -1){
                userarray[a].i = 1;
                memcpy(userarray[a].boxname, boxname, BOXNAME_SIZE);
                memcpy(userarray[a].pipename, pipename, PIPENAME_SIZE);
                sessions++;
                break;
            }
        }
    }
    pthread_mutex_unlock(&userarraylock);
}

void create_box(char * pipename, char * boxname) {
    int32_t return_code;
    char error_message[ERROR_MESSAGE_SIZE];
    if(tfs_lookup(boxname, ROOT_DIR_INUM) != -1){
        return_code = -1;
        memcpy(error_message, "Caixa existe", ERROR_MESSAGE_SIZE);
    } else{
        return_code = 0;
        error_message[0] = '\0';
        int i = tfs_open(boxname, TFS_O_CREAT);
        if(tfs_close(i) == -1) {
            memcpy(error_message, "ERRO", 5);
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
    memcpy(message, error_message, ERROR_MESSAGE_SIZE);
    send_msg(rx, message);
    close(rx);
}

void remove_box(char * pipename, char * boxname) {
    int32_t return_code;
    char error_message[ERROR_MESSAGE_SIZE];
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        return_code = -1;
        memcpy(error_message, "Caixa nao existe", 17);
    } else{
        return_code = 0;
        error_message[0] = '\0';
        if (tfs_unlink(boxname) == -1) {
            memcpy(error_message, "ERRO", ERROR_MESSAGE_SIZE);
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
    memcpy(message, error_message, ERROR_MESSAGE_SIZE);
    send_msg(rx, message);
    close(rx);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "error\n");
    }
    char * register_pipename;        //register_pipename, pipe que recebe os pedidos de registo dos clientes 
    register_pipename = malloc(sizeof(char)*strlen(argv[1]));
    strcpy(register_pipename,argv[1]);
    strcat(register_pipename, PATHNAME);
    long unsigned int max_sessions = (long unsigned int)atoi(argv[2]);
    tid = malloc(max_sessions * sizeof(pthread_t));
    userarray = malloc(INODE_TABLE_SIZE * sizeof(box));
    pc_queue_t queue;
    for(int i = 0; i < max_sessions; i++) {
        pthread_create(&tid[i], NULL, threadfunction, NULL);
    }
    for (int g = 0; g <  MAX_DIR_ENTRIES; g++) {
        userarray[g].i = -1;
        userarray[g].boxname[0] = '\0';
        userarray[g].pipename[0] = '\0';
    }
    if (unlink(register_pipename) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", register_pipename,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(register_pipename, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    int rx = open(register_pipename, O_RDONLY);
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
        char client_pipename[PIPENAME_SIZE];
        char box_name[BOXNAME_SIZE];
        slice(buffer, ccode, 0, 1);
        uint8_t code = (uint8_t) atoi(ccode);
        slice(buffer, client_pipename, 1, 257);
        slice(buffer, box_name, 257, 289);
        switch (code){
        case(1):{
            register_publisher(client_pipename,box_name);
            break;
        };
        case(2):{
            register_subscriber(client_pipename,box_name);
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