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
#include "semaforos.h"

#define BUFFER_SIZE 80
#define KEY 1998
#define SEM_SIZE 2

typedef struct{
    char nombre[BUFFER_SIZE];
    int id;
}info;

int is_valid_integer(char *input);
int aleat_num(int inf, int sup);
void handler_SIGUSR1(int sig);


int main(int argc, char *argv[]){
    int childs, pid, key, id_zone, i, keysem, check_sem, semid;
    unsigned short *sem_array;
    void handler_SIGUSR1();
    sigset_t set;
    char buff[BUFFER_SIZE];
    info *information;

    if(argc != 2){
        printf("Error al pasar los argumentos. Introduce el numero de procesos hijo.\n");
        exit(EXIT_FAILURE);
    }

    if(!is_valid_integer(argv[1])) {
        printf("Error, el numero de procesos hijo debe ser un entero.\n");
        exit(EXIT_FAILURE);
    }

    childs = atoi(argv[1]);

    if(childs < 0){
        printf("Error, el numero de procesos hijo debe ser un entero positivo.\n");
        exit(EXIT_FAILURE);
    }

    /*Hacemos que la estuctura esté en memoria compartida*/
    key = ftok("/bin/bash", KEY);
    id_zone = shmget(key, sizeof(info), IPC_CREAT);
    if(id_zone == -1){
        printf("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    information = (info *)shmat(id_zone, (char *)0, 0);

    /**Creamos dos semáforos: el primero para controlar que solo pueda haber
    un proceso solicitando datos al output y el segundo para la memoria compartida*/
    keysem = ftok("/bin/bash", aleat_num(1000,5000));

    check_sem = Crear_Semaforo(keysem, SEM_SIZE, &semid);
    if(check_sem == ERROR){
        printf("Error en la creacion del conjunto de semaforos\n");
        exit(EXIT_FAILURE);
    }

    sem_array = (unsigned short *) malloc((SEM_SIZE)*sizeof(unsigned short));
    if(!sem_array){
        printf("Error reservando memoria para el array de ids de semaforos involucrados\n");
        exit(EXIT_FAILURE);
    }
    for(i=0; i<=SEM_SIZE; i++){
        sem_array[i] = 1;
    }

    if(Inicializar_Semaforo(semid, sem_array) == ERROR){/*Ponemos los semáforos a 1*/
        printf("Error inicializando a 1 todos los semaforos al principio\n");
        free(sem_array);
        exit(EXIT_FAILURE);
    }
    free(sem_array);


    /**Creamos los procesos hijo*/
    for(i=0; i<childs; i++){
        if((pid = fork())<0){
            printf("Error al crear los procesos hijo.\n");
            exit(EXIT_FAILURE);
        }
        if(!pid){
            break;
        }
    }

    /*Código del proceso hijo*/
    if(!pid){
        sleep(aleat_num(1,5));

        if(Down_Semaforo(semid, 0, SEM_UNDO) == ERROR){/*Bajando el semaforo del output*/
            printf("Error bajando el semaforo del output.\n");
            exit(EXIT_FAILURE);
        }
        printf("Alta del cliente del proceso %d: ", i);
        scanf("%s",buff);

        if(Up_Semaforo(semid, 0, SEM_UNDO) == ERROR){/*Subimos el semaforo del output*/
            printf("Error subiendo el semaforo del output.\n");
            exit(EXIT_FAILURE);
        }

        if(Down_Semaforo(semid, 1, SEM_UNDO) == ERROR){/*Bajamos el semaforo de la memoria compartida*/
            printf("Error bajando el semaforo de la memoria compartida.\n");
            exit(EXIT_FAILURE);
        }
        printf("Hemos entrado en memoria compratida\n");
        information->id = information->id + 1;
        printf("Se ha modificado la memoria compartida\n");
        fflush(NULL);
        kill(getppid(), SIGUSR1);

        exit(EXIT_SUCCESS);
    }

    /*Código del proceso padre*/
    if(signal(SIGUSR1, handler_SIGUSR1)==SIG_ERR){ /* Armamos nuevo manejador de la señal SIGUSR1 */
        printf("Error estableciendo el nuevo manejador de SIGTERM.\n"); /* Error */
        exit(EXIT_FAILURE);
    }

    if(sigfillset(&set)){  /* Creamos un conjunto de señales vacio */
            printf("Error en sigfillset(&set)"); /* Error */
            exit(EXIT_FAILURE);
    }

    if(sigaddset(&set, SIGUSR1)){ /*Añadimos nuestra señal únicamente al padre*/
        printf("Error en sigaddset(&set, SIGUSR1)"); /* Error */
        exit(EXIT_FAILURE);
    }

    for(i=0; i<childs; i++){/**Bucle en el que el padre esperará a sus hijos*/
        sigsuspend(&set);
        if(Down_Semaforo(semid, 0, SEM_UNDO) == ERROR){/*Bajando el semaforo del output*/
            printf("Error bajando el semaforo del output.\n");
            exit(EXIT_FAILURE);
        }
        printf("Nombre: %s\n", information->nombre);
        printf("Identificador: %d\n", information->id);
        if(Up_Semaforo(semid, 0, SEM_UNDO) == ERROR){/*Subimos el semaforo del output*/
            printf("Error subiendo el semaforo del output.\n");
            exit(EXIT_FAILURE);
        }

        if(Up_Semaforo(semid, 1, SEM_UNDO) == ERROR){/*Subimos el semaforo de la memoria compartida*/
            printf("Error subiendo el semaforo de la memoria compartida.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    Borrar_Semaforo(semid);

    exit(EXIT_SUCCESS);
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
         printf("ERROR. El parámetro introducido no es un entero positivo.\n");
         return 0;
      }
    }
    if(!atoi(input)){
      printf("ERROR. No se admite cero como entero de entrada.\n");
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

void handler_SIGUSR1(int sig){
    return;
}
