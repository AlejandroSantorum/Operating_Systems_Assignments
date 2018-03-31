#ifndef SEMAFOROS_H
#define SEMAFOROS_H

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

#ifndef ERR
    #define ERROR -1
    #define OK 0
#endif

int Inicializar_Semaforo(int semid, unsigned short *array);

int Borrar_Semaforo(int semid);

int Crear_Semaforo(key_t key, int size, int *semid);

int Down_Semaforo(int id, int num_sem, int undo);

int DownMultiple_Semaforo(int id, int size, int undo, int *active);

int Up_Semaforo(int id, int num_sem, int undo);

int UpMultiple_Semaforo(int id, int size, int undo, int *active);

#endif /** SEMAFOROS_H */