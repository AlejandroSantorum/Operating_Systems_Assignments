/**
* @brief Utilidades de manejo de una cola de enteros
*
* Este modulo contiene los prototipos de las funciones de manejo del
* tipo abstracto de datos Cola de enteros.
* @file queue_int.h
* @author Alejandro Santorum & David Cabornero (G2202-Pareja7)
* @version 1.0
* @date 01-04-2018
*/

#ifndef QUEUE_INT_H
#define QUEUE_INT_H


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

typedef struct _Queue Queue;


Queue* queue_create(int max_size);

void queue_destroy(Queue *q);

int queue_isFull(Queue *q);

int queue_isEmpty(Queue *q);

Queue* queue_push(Queue *q, int n);

int queue_pop(Queue *q);


#endif /** QUEUE_INT_H */