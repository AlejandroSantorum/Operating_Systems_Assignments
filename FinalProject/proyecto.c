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
#include <syslog.h>
#include "semaforos.h"


#define N_ARGS 6
#define MIN_PROC 3

#define MAX_HORSES 10
#define MIN_HORSES 2
#define MAX_RACE 200
#define MIN_RACE 20
#define MAX_BETTOR 100
#define MIN_BETTOR 1
#define MAX_WINDOWS 30
#define MIN_WINDOWS 1
#define MAX_MONEY 10000
#define MIN_MONEY 50

#define PROC_MONITOR 0
#define PROC_GESTOR 1
#define PROC_APOSTADOR 2

#define PATH "\bin\bash"
#define ERROR -1

struct msgbuf{
    long mtype; /* type of message */
    struct info{
        char name[20];
        int id;
        float bet;
    }; /* information of message */
};


void init_syslog(){
    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("Proyecto_final_SOPER_logger", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}


int is_valid_integer(char *input);
int aleat_num(int inf, int sup);


int main(int argc, char **argv){
    int n_horses, len_race, n_bettor, n_windows, money, n_proc;
    int i;
    int memkey, memid;
    int msgkey, msgid;
    int *pids=NULL;
    float *cotizaciones=NULL;
    
    /* Comprobacion inicial de errores */
    if(argc != N_ARGS){
        printf("Argumentos que se deben especificar:\n");
        printf("-Numero de caballos de la carrera(max 10)\n");
        printf("-Longitud de la carrera\n");
        printf("-Numero de apostadores(max 100)\n");
        printf("-Numero de ventanillas\n");
        printf("-Dinero de cada apostante\n");
        exit(EXIT_FAILURE);
    }
    
    for(i = 0; i < N_ARGS; i++){
        if(!is_valid_integer(argv[i])){
            printf("Error en el parametro %d: entero no valido o 0\n",i);
            exit(EXIT_FAILURE);
        }
    }
    
    n_horses = atoi(argv[1]);
    len_race = atoi(argv[2]);
    n_bettor = atoi(argv[3]);
    n_window = atoi(argv[4]);
    money = atoi(argv[5]);
    
    if(n_horses > MAX_HORSES || n_horses < MIN_HORSES){
        printf("Error. El numero de caballos debe estar entre %d y %d\n", MIN_HORSES, MAX_HORSES);
        exit(EXIT_FAILURE);
    }
    
    if(len_race < MIN_RACE || len_race > MAX_RACE){
        printf("Error. La longitud de la carrera debe ser como minimo %d, y como maximo %d\n", MIN_RACE, MAX_RACE);
        exit(EXIT_FAILURE);
    }
    
    if(n_bettor < MIN_BETTOR || n_bettor > MAX_BETTOR){
        printf("Error. El numero de apostadores debe ser como minimo %d, y como maximo %d\n", MIN_BETTOR, MAX_BETTOR);
        exit(EXIT_FAILURE);
    }
    
    if(n_window < MIN_WINDOWS || n_window > MAX_WINDOWS){
        printf("Error. El numero de ventanillas debe ser como minimo %d, y como maximo %d\n", MIN_WINDOWS, MAX_WINDOWS);
        exit(EXIT_FAILURE);
    }
    
    if(money < MIN_MONEY || money > MAX_MONEY){
        printf("Error. La cantidad de dinero de un apostador debe ser como minimo %d, y como maximo %d\n", MIN_MONEY, MAX_MONEY);
        exit(EXIT_FAILURE);
    }
    /* Fin de la comprobacion inicial de errores */
    
    n_proc = n_horses+MIN_PROC;
    
    /* Array de pids de todos los procesos a crear */
    pids = (int*) malloc(n_proc * sizeof(int));
    if(!pids){
        perror("Error al reservar memoria para el array de pids");
        exit(EXIT_FAILURE);
    }
    
    /* Creacion de la cola de mensajes */
    msgkey = ftok(PATH, aleat_num(3001, 5000));
    if(msgkey == ERROR){
        free(pids);
        perror("Error obteniendo la key para la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    msgid = msgget(msgkey, IPC_CREAT|IPC_EXCL|0660);
    if(msgid==ERROR && errno==EEXIST){
        msgid = msgget(msgkey, IPC_CREAT|0660);
    }else if(msgid == ERROR){
        free(pids);
        perror("Error creando la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    /*Fin de la creacion de la cola de mensajes*/
    
    /* Creacion de la memoria compartida */
    memkey = ftok(PATH, aleat_num(1000,3000));
    if(memkey == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        free(pids);
        perror("Error obteniendo la key para la zona compartida de las cotizaciones");
        exit(EXIT_FAILURE);
    }
    memid = shmget(memkey, sizeof(float)*n_horses, IPC_CREAT);
    if(memid == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        free(pids);
        perror("Error al crear la memoria compartida para las cotizaciones.");
        exit(EXIT_FAILURE);
    }
    cotizaciones = (float *) shmat(memid, (char *)0, 0);
    if(cotizaciones == ((void*)ERROR)){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        shmctl(memid, IPC_RMID, NULL);
        free(pids);
        perror("Error adjuntando la zona de memoria para las cotizaciones");
        exit(EXIT_FAILURE);
    }
    /* Fin de la creacion de la memoria compartida */
    
    
    for(i=0; i<n_proc; i++){
        if((pids[i] = fork())==ERROR){
            msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
            shmdt(cotizaciones);
            shmctl(memid, IPC_RMID, NULL);
            free(pids);
            perror("Error al realizar el %d-esimo proceso",i);
            exit(EXIT_FAILURE);
        }
        if(!pids[i]){
            break;
        }
    }
    
    if(i==PROC_MONITOR){
        proceso_monitor();
    }else if(i==PROC_GESTOR){
        proceso_gestor_apuestas();
    }else if(i==PROC_APOSTADOR){
        proceso_apostador();
    }else if(i>PROC_APOSTADOR && i<n_proc){
        caballo();
    }else{/* Proceso padre*/
        
    }
}



int is_valid_integer(char *input){
    int len, i;

    if(!input){
        printf("Error. Input igual a NULL\n");
        exit(EXIT_FAILURE);
    }

    len = strlen(input);

    for(i=0; i<len; i++){
        if(!isdigit(input[i])){
         return 0;
      }
    }
    if(!atoi(input)){
      return 0;
   }
   return 1;
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