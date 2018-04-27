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

#define KEY 2900
#define SEM_SIZE 3
#define N_PROD 50

int is_valid_integer(char *input);
int aleat_num(int inf, int sup);
char next_char(char old);

int main(int argc, char *argv[]){
    int size_queue, queue_place, check_sem, semid, keysem, i, pid;
    unsigned short *sem_array;
    char *buff;
    char item;
    
    if(argc != 2){
        printf("Error al pasar los argumentos. Introduce la longitud maxima de la cola.\n");
        exit(EXIT_FAILURE);
    }

    if(!is_valid_integer(argv[1])) {
        printf("Error, la longitud maxima de la cola debe ser un entero.\n");
        exit(EXIT_FAILURE);
    }

    size_queue = atoi(argv[1]);

    if(size_queue < 0){
        printf("Error, la longitud maxima de la cola debe ser un entero positivo.\n");
        exit(EXIT_FAILURE);
    }
    
    /**Creando zona de memoria compartida (nuestro buffer)*/
    key = ftok("/bin/bash", KEY);
    id_zone = shmget(key, sizeof(char)*size_queue, IPC_CREAT);
    if(id_zone == -1){
        printf("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }
    
    buff = (char *)shmat(id_zone, (char *)0, 0);
    /**Memoria compartida creada*/
    
    /**Creamos e inicializamos los tres semáforos: 
     * 1. El número de entradas vacías (vacio)
     * 2. El número de entradas ocupadas (lleno)
     * 3. Mutex para la memoria compartida (mutex)*/
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
    sem_array[0] = size_queue;
    sem_array[1] = 0;
    sem_array[2] = 1;
    
    
    if(Inicializar_Semaforo(semid, sem_array) == ERROR){/*Ponemos los semáforos a 1*/
        printf("Error inicializando a 1 todos los semaforos al principio\n");
        free(sem_array);
        exit(EXIT_FAILURE);
    }
    free(sem_array);
    /**Fin de la creación e inicialización de los semáforos.*/
    
    /**El proceso padre será el productor, el proceso hijo el consumidor*/
    pid = fork();
    if(pid < 0){
        printf("Error al crear el proceso hijo.\n");
        exit(EXIT_FAILURE);
    }
    
    /**Consumidor*/
    if(pid == 0){
        for(i = 0; i<N_PROD; i++){
            sleep(aleat_num(1,5));
            
            if(Down_Semaforo(semid, 1, SEM_UNDO) == ERROR){/*Bajando el semaforo de lleno*/
                printf("Error bajando el semaforo de lleno.\n");
                exit(EXIT_FAILURE);
            }
            if(Down_Semaforo(semid, 2, SEM_UNDO) == ERROR){/*Bajando el semaforo de mutex*/
                printf("Error bajando el semaforo de mutex.\n");
                exit(EXIT_FAILURE);
            }
            /**Entramos en memoria compartida*/
            
            
            /**Salimos de memoria compartida*/
            
            if(Up_Semaforo(semid, 2, SEM_UNDO) == ERROR){/*Subiendo el semaforo de mutex*/
                printf("Error subiendo el semaforo de mutex.\n");
                exit(EXIT_FAILURE);
            }
            if(Up_Semaforo(semid, 0, SEM_UNDO) == ERROR){/*Subiendo el semaforo de vacio*/
                printf("Error subiendo el semaforo de vacio.\n");
                exit(EXIT_FAILURE);
            }
            /**Imprimimos el item recogido*/
        }
        
        exit(EXIT_SUCCESS);
    }
    
    /**Productor*/
    item = 'A';
    for(i=0; i<N_PROD; i++){
        sleep(aleat_num(1,5));
        
        if(Down_Semaforo(semid, 0, SEM_UNDO) == ERROR){/*Bajando el semaforo de vacio*/
            printf("Error bajando el semaforo de vacio.\n");
            exit(EXIT_FAILURE);
        }
        if(Down_Semaforo(semid, 2, SEM_UNDO) == ERROR){/*Bajando el semaforo de mutex*/
            printf("Error bajando el semaforo de mutex.\n");
            exit(EXIT_FAILURE);
        }
        
        /**Entramos en memoria compartida*/
        /**Salimos de memoria compartida*/
        
        if(Up_Semaforo(semid, 2, SEM_UNDO) == ERROR){/*Subiendo el semaforo de mutex*/
            printf("Error subiendo el semaforo de mutex.\n");
            exit(EXIT_FAILURE);
        }
        if(Up_Semaforo(semid, 1, SEM_UNDO) == ERROR){/*Subiendo el semaforo de lleno*/
            printf("Error subiendo el semaforo de lleno.\n");
            exit(EXIT_FAILURE);
        }
        
        item = next_char(item);
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

char next_char(char old){
    char new;
    
    if((old< 'Z' && old>='A')||old<'9' && old >='0'){
        new = old + 1
    }
    else if(old == 'Z'){
        new = old + 1;
    }
    else if(old == '9'){
        new = old + 1;
    }
    else{
        printf("Error en next_char.\n");
        exit(EXIT_FAILURE);
    }
    
    return new;
}