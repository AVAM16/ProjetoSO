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
#include "producer-consumer.h"


#define PATHNAME ".pipe"
#define BUFFER_SIZE (289)
#define MESSAGE_SIZE (1025)
#define BOXCREATERESPONSE 1029
#define PIPENAME_SIZE 256
#define BOXNAME_SIZE 32
#define ERROR_MESSAGE_SIZE 1024
#define LIST_MESSAGE_SIZE 58
#define BOX_SIZE_BITS 1024

typedef struct
{
    char boxname[BOXNAME_SIZE];
    char pipename[PIPENAME_SIZE];
    int i; //0 se publisher 1 se subscriber
}box;

/* pthread_cond_t cond;
*/
long unsigned int max_sessions = 0;
int sessions = 0;
pthread_t *tid;
box *userarray;
char **boxarray;
pthread_mutex_t userarraylock = PTHREAD_MUTEX_INITIALIZER;
pc_queue_t queue;


static void sigint_handler() {
    fprintf(stdout, "Caught SIGINT - that's all folks!\n");
    exit(EXIT_SUCCESS);
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

void add_box(char * boxname) {
    for(int i = 0; i < INODE_TABLE_SIZE; i++) {
        if(boxarray[i][0] == '\0') {
            memcpy(boxarray[i], boxname, BOXNAME_SIZE);
            break; //precisamos de parar o for senao todas as caixas ficam iguais
        }
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
    char message[BUFFER_SIZE];
    memcpy(message,pcq_dequeue(&queue), BUFFER_SIZE);
    char ccode;
    char client_pipename[PIPENAME_SIZE];
    char box_name[BOXNAME_SIZE];
    slice(message, &ccode, 0, 1);
    uint8_t code = (uint8_t) atoi(&ccode);
    slice(message, client_pipename, 1, 257);
    slice(message, box_name, 257, 289);
    if (code == 1){
        while(true){
            int rx = open(client_pipename, O_RDONLY);
            char buffer[MESSAGE_SIZE];
            ssize_t ret = read(rx, buffer, MESSAGE_SIZE - 1);
            buffer[ret] = 0;
            if (ret > 0){
                char ccode1;
                char message1[ERROR_MESSAGE_SIZE];
                slice(buffer, &ccode1, 0, 1);
                uint8_t code1 = (uint8_t) atoi(&ccode1);
                slice(buffer, message1, 1, 1025);
                if (code1 == 9) {
                    int i = tfs_open(box_name, TFS_O_APPEND);
                    tfs_write(i, message1, 1024);
                    tfs_close(i);
                }
            }
            close(rx);
        }
    } else{
        while(true){
            int rx = open(client_pipename, O_WRONLY);
            char buffer[ERROR_MESSAGE_SIZE];
            int a = tfs_open(box_name, TFS_O_APPEND);
            while(tfs_read(a, buffer, ERROR_MESSAGE_SIZE) > 0) {
                char message1[MESSAGE_SIZE];
                uint8_t code1 = 10;
                char ccode1 = (char) code1;
                memcpy(message1,&ccode1, 1);
                memcpy(message1, buffer, ERROR_MESSAGE_SIZE);
                send_msg(rx, message1);
            }
            close(rx);
        }

    }
    return NULL;
}

void register_publisher(char * pipename, char * boxname, char * buffer){
    pthread_mutex_lock(&userarraylock);
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        fprintf(stderr, "Erro\n");
        pthread_mutex_unlock(&userarraylock);
        return;
    } else{
        for (int a = 0; a < max_sessions ; a++) {
            if(memcmp(userarray[a].boxname, boxname, BOXNAME_SIZE) == 0 && userarray[a].i == 0){
                fprintf(stderr,"Erro, Box já está associada com um Publisher");
                pthread_mutex_unlock(&userarraylock);
                return;
            }
        }
        userarray[sessions].i = 0;
        memcpy(userarray[sessions].boxname, boxname, BOXNAME_SIZE);
        memcpy(userarray[sessions].pipename, pipename, PIPENAME_SIZE);
        pcq_enqueue(&queue, buffer);
        sessions++;
    }
    pthread_mutex_unlock(&userarraylock);

}

void register_subscriber(char * pipename, char * boxname, char * buffer){
    pthread_mutex_lock(&userarraylock);
    if(tfs_lookup(boxname, ROOT_DIR_INUM) == -1){
        fprintf(stderr, "Erro\n");
        pthread_mutex_unlock(&userarraylock);
        return;
    } else{
        for (int a = 0; a < max_sessions; a++) {

            pthread_mutex_unlock(&userarraylock);
            return;
        }
        userarray[sessions].i = 1;
        memcpy(userarray[sessions].boxname, boxname, BOXNAME_SIZE);
        memcpy(userarray[sessions].pipename, pipename, PIPENAME_SIZE);
        pcq_enqueue(&queue, buffer);
        sessions++;
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
        } else {
            add_box(boxname);
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
        }else{
            add_box(boxname);
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

int cmpfunc(const void * a, const void * b){
    return  *(char*)a - *(char*)b;
}

void list_boxes(char * pipename){
    uint8_t code = 8;
    char *ccode = convert(&code);
    uint8_t last = 0;
    char boxname[BOXNAME_SIZE];
    char namelist[INODE_TABLE_SIZE];
    int rx = open(pipename, O_WRONLY);
    if(boxarray[0][0] == '\0' ){
        char message[34];
        last = 1;
        char *llast = convert(&last);
        for(int i=0; i<BOXNAME_SIZE;i++){
            boxname[i]='\0';
        }
        memcpy(message,ccode,1);
        memcpy(message,llast,1);
        memcpy(message,boxname,BOXNAME_SIZE);
        send_msg(rx, message);
    }else{
        uint64_t sub = 0;
        uint64_t pub = 0;
        uint64_t size = BOX_SIZE_BITS;
        char message[LIST_MESSAGE_SIZE];
        for(int i=0; i<INODE_TABLE_SIZE;i++){
            namelist[i] = boxarray[i][0]; 
        }
        qsort(namelist,INODE_TABLE_SIZE,sizeof(char),cmpfunc);
        for(int j=0; j<INODE_TABLE_SIZE;j++){
            for(int x=0; x<max_sessions;x++){
                if(namelist[j] == userarray[x].boxname){
                    if(userarray[x].i == 0){
                        pub++;
                    } else{
                        sub++;
                    }
                }
            }
            if(j == INODE_TABLE_SIZE-1){
                last = 1;
            }
            char *llast = convert(&last);
            char *ssub = (char*)sub;
            char *ppub = (char*)pub;
            char *ssize = (char*)size;
            memcpy(message,ccode,1);
            memcpy(message,llast,1);
            memcpy(message,namelist[j],BOXNAME_SIZE);
            memcpy(message,ssize,3);
            memcpy(message,ppub,1);
            memcpy(message,ssub,1);
            send_msg(rx, message);
        }
    }
    close(rx);
}

int main(int argc, char **argv) {
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        exit(EXIT_FAILURE);
    }
    if(tfs_init(NULL) == -1){
        return -1;
    }
    if (argc != 3) {
        fprintf(stderr, "error\n");
    }
    char * register_pipename;        //register_pipename, pipe que recebe os pedidos de registo dos clientes 
    register_pipename = malloc(sizeof(char)*strlen(argv[1]));
    strcpy(register_pipename,argv[1]);
    strcat(register_pipename, PATHNAME);
    max_sessions = (long unsigned int)atoi(argv[2]);
    tid = malloc(max_sessions * sizeof(pthread_t));
    userarray = malloc(INODE_TABLE_SIZE * sizeof(box));
    boxarray = (char**)malloc(sizeof(char*)*INODE_TABLE_SIZE);
    pcq_create(&queue, max_sessions);
    for(int i=0; i<INODE_TABLE_SIZE; i++)
    {
       boxarray[i] = (char*)malloc(sizeof(char)*BOXNAME_SIZE);
    }
    for(int a=0; a<INODE_TABLE_SIZE; a++)
    {
       boxarray[a][0] = '\0';
    }
    for(int i = 0; i < max_sessions; i++) {
        pthread_create(&tid[i], NULL, threadfunction, NULL);
    }
    for (int g = 0; g <  max_sessions; g++) {
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
            register_publisher(client_pipename,box_name, buffer);
            break;
        };
        case(2):{
            register_subscriber(client_pipename,box_name, buffer);
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
            //list_boxes(client_pipename);
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