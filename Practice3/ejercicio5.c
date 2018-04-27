#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "semaforos.h"


#define NUM_PROC 3
#define FILE_N_BYTES 8192
#define SIZE 2048
#define SEM_SIZE 1

struct msgbuf{
    long mtype; /* type of message */
    char mtext[SIZE]; /* message text */
}

int aleat_num(int inf, int sup);
void process_a(int msgid, int semid, char *filename);
void process_b(int msgid, int semid);


int flag = 0;

int main(int argc, char *argv[]){
    int key_msg1, key_msg2, key_sem, msgid2, msgid1, i, pid[NUM_PROC], semid, check_sem;
    int ini_array[SEM_SIZE] = {1};
    
    if(argc != 3){
        printf("Error de entrada: parametros incorrectos.\n");
        printf("Por favor. Introduzca el nombre del programa, el nombre del fichero 1 y el nombre del fichero 2\n");
        exit(EXIT_FAILURE);
    }
    
    /*Creamos el archivo inicial de origen*/
    prepare_file(argv[1], FILE_N_BYTES);
    
    /*Creación de la cola de mensajes*/
    if((key_msg1 = ftok("/bin/bash",aleat_num(1000, 3000)) == -1){
        perror("Error al hacer ftok para la creacion de cola de mensajes (1)");
        exit(EXIT_FAILURE);
    }
    
    if((msgid1 = msgget(key_msg1, IPC_CREAT|0660)) == -1){
        perror("Error al crear la cola de mensajes (1)");
        exit(EXIT_FAILURE);
    }
    
    if((key_msg2 = ftok("/bin/bash",aleat_num(5000, 7000)) == -1){
        perror("Error al hacer ftok para la creacion de cola de mensajes (2)");
        exit(EXIT_FAILURE);
    }
    
    if((msgid2 = msgget(key_msg2, IPC_CREAT|0660)) == -1){
        perror("Error al crear la cola de mensajes (2)");
        exit(EXIT_FAILURE);
    }
    /*Fin de la creacion de cola de mensajes*/
    
    /*Creación de hijos*/
    for(i=0; i<NUM_PROC; i++){
        if((pid[i] = fork()) < 0){
            printf("Error al crear los hijos.\n");
            exit(EXIT_FAILURE);
        }
        
        if(!pid[i]){
            break;
        }
    }
    /*Fin de la creacion de hijos*/
    
    /*Proceso C*/
    if(i == 0){
        process_c(msgid2, argv[2]);
        exit(EXIT_SUCCESS);
    }
    
    /*Proceso B*/
    else if(i == 1){
        process_b(msgid1, msgid2, pid[0]);
        exit(EXIT_SUCCESS);
    }
    
    /*Proceso A*/
    else if(i == 2){
        process_a(msgid1, argv[1], pid[1]);
        exit(EXIT_SUCCESS);
    }
    
    while(wait(NULL)<0);
    exit(EXIT_SUCCESS);
}


void prepare_file(char *filename, int n_bytes){
    FILE *f=NULL;
    
    if(filename==NULL){
        perror("Error. el nombre del fichero en prepare_file function es NULL\n");
        exit(EXIT_FAILURE);
    }
    
    f = (FILE*) fopen(filename, "w");
    if(f==NULL){
        perror("Error abriendo el fichero origen para su preparacion\n");
        exit(EXIT_FAILURE);
    }
    
    for(i=0; i<n_bytes; i++){
        fprintf(f, "%c", aleat_num('a', 'z'));
    }
    
    fclose(f);
    return;
}


void process_a(int msgid, char *filename, int pid_b){
    FILE *f=NULL;
    size_t size=0;
    struct msgbuf msg_arg;
    
    if(msgid<0 || filename==NULL ||pid_b<0){
        perror("Error en los argumentos de entrada en process_a function\n");
        exit(EXIT_FAILURE);
    }
    
    f = fopen(filename, "r");
    if(f==NULL){
        perror("Error abriendo el fichero en process_a function");
        exit(EXIT_FAILURE);
    }
    
    while(size >= SIZE){
        size = fread(msg_arg.mtext, 1, SIZE, f);
        if(size==0) break; /* El tamaño del fichero era potencia de dos */
        
        msg_arg.mtype = 1;
        if(msgsnd(msgid, (struct msgbuf *)&msg_arg, sizeof(struct msgbuf) - sizeof(long), IPC_WAIT) == -1){
            perror("Error enviando mensaje desde A\n");
            exit(EXIT_FAILURE);
        }
    }
    fclose(f);
    if(kill(pid_b, SIGUSR1)){ /* Si kill != 0 entonces ERROR */
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return;
}


void process_b(int msgid1, int msgid2, int pid_c){
    struct msqid_ds buf;
    struct msgbuf msg_rcv, msg_snd;
    int num_msg, i;
    void handler_SIGUSR1;
    
    if(msgid1<0 || msgid2<0 || filename==NULL || pid_c<0){
        perror("Error en los argumentos de entrada en process_b function");
        exit(EXIT_FAILURE);
    }
    
    if(signal(SIGUSR1, handler_SIGUSR1)==SIG_ERR){ 
        perror("Error estableciendo el nuevo manejador de SIGUSR1 en B"); /* Error */
        exit(EXIT_FAILURE);
    }
    
    while(1){
        if(flag == 1){
            if(msgctl(msgid1, IPC_STAT, &buf)==-1){
                perror("Error obteniendo la informacion de la cola en process_b function");
                exit(EXIT_FAILURE);
            }
            num_msg = buf.msg_qnum;
            if(num_msg==0){
                break;
            }
        }
        
        if(msgrcv(msgid1, (struct msgbuf *)&msg_rcv, sizeof(struct msgbuf) - sizeof(long), 1, 0) == -1){
            perror("Error recibiendo mensaje en B\n");
            exit(EXIT_FAILURE);
        }
        
        for(i=0; i<SIZE; i++){
            if(msg_rcv.mtext[i] == 'z') msg_rcv.mtext[i] = 'a';
            else if(msg_rcv.mtext[i] < 'z' && msg_rcv.mtext[i] >= 'a') msg_rcv.mtext[i] = msg_rcv.mtext[i]+1;
            /* Si es una letra mayuscula u otro caracter, no sera variado */
        }
        
        strcpy(msg_snd.mtext, msg_rcv.mtext);
        msg_snd.mtype = 2;
        if(msgsnd(msgid2, (struct msgbuf *)&msg_snd, sizeof(struct msgbuf) - sizeof(long), IPC_WAIT) == -1){
            perror("Error enviando mensaje desde B\n");
            exit(EXIT_FAILURE);
        }
        
    }
    if(kill(pid_c, SIGUSR1)){ /* Si kill != 0 entonces ERROR */
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return;
}



void process_c(int msgid, char *filename){
    struct msqid_ds buf;
    struct msgbuf msg_rcv;
    int num_msg;
    FILE *f=NULL;
    void handler_SIGUSR1;
    
    if(msgid<0 || filename==NULL){
        perror("Error en los argumentos de entrada en process_c function\n");
        exit(EXIT_FAILURE);
    }
    
    if(signal(SIGUSR1, handler_SIGUSR1)==SIG_ERR){ 
        perror("Error estableciendo el nuevo manejador de SIGUSR1 en C"); /* Error */
        exit(EXIT_FAILURE);
    }
    
    f = (FILE *) fopen(filename, "w");
    if(f==NULL){
        perror("Error abriendo el fichero de escritura en el proceso C");
    }
    
    while(1){
        if(flag == 1){
            if(msgctl(msgid, IPC_STAT, &buf)==-1){
                perror("Error obteniendo la informacion de la cola en process_c function");
                exit(EXIT_FAILURE);
            }
            num_msg = buf.msg_qnum;
            if(num_msg==0){
                break;
            }
        }
    
        if(msgrcv(msgid, (struct msgbuf *)&msg_rcv, sizeof(struct msgbuf) - sizeof(long), 2, 0) == -1){
            perror("Error recibiendo mensaje en C\n");
            exit(EXIT_FAILURE);
        }
        
        fwrite(msg_rcv.mtext, 1, sizeof(msg_rcv.mtext), f);
    }
    fclose(f);
    return;
}




int aleat_num(int inf, int sup){
    int result = 0;

    if(inf == sup){
        return sup;
    }

    else if (inf > sup){
        printf("ERROR: Límite inferior mayor que el límite superior.\n");
        exit(EXIT_FAILURE);
    }

    result = (inf + ((int) ((((double)(sup-inf+1)) * rand())/(RAND_MAX + 1.0))));

    return result;
}


void handler_SIGUSR1(int sig){
    flag=1;
    return;
}

char next_char(char old){
    char new;
    
    if(old< 'z' && old>='a'){
        new = old + 1
    }
    else if(old == 'z'){
        new = 'a';
    }
    else{
        printf("Error en next_char.\n");
        exit(EXIT_FAILURE);
    }
    
    return new;
}