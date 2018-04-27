#include "queue_int.h"

struct _Queue {
    int *item;
    int head;
    int end;
    int max_size;
};


Queue* queue_create(int max_size){
    Queue *q=NULL;
    
    if(max_size <= 0){
        printf("ERROR: Queue size is equal or less than zero\n");
        return NULL;
    }
    
    q = (Queue *) malloc(sizeof(Queue));
    if(!q){
        printf("ERROR: Allocating memory for new queue\n");
        return NULL;
    }
    
    q->item = (int *) malloc(max_size * sizeof(int));
    if(!q->item){
        printf("ERROR: Allocating memory for new queue's int array\n");
        return NULL;
    }
    
    q->head = -1;
    q->end = 0;
    q->max_size = max_size;
    
    return q;
}


void queue_destroy(Queue *q){
    if(q!=NULL){
        if(q->item!=NULL) free(q->item);
        free(q);
        return;
    }
    return;
}


int queue_isFull(Queue *q){
    
    if(q==NULL){
        printf("ERROR: Queue is null\n");
        return 0;
    }
    
    if(q->head == q->end){
        return 1;
    }
    
    return 0;
}


int queue_isEmpty(Queue *q){
    if(!q){
        printf("ERROR: Queue is null\n");
        return 0;
    }
    
    if(q->head == -1){
        return 1;
    }
    
    return 0;
}


Queue* queue_push(Queue *q, int n){
    if(q==NULL){
        printf("ERROR: Queue is null\n");
        return NULL;
    }
    if(queue_isFull(q)){
        printf("ERROR: Queue is full\n");
        return NULL;
    }
    
    q->item[q->end] = n;
    
    if(q->head == -1){
        q->head = q->end;
    }
    
    if(q->end == q->max_size-1){
        q->end = 0;
    }
    else{
        q->end++;
    }
    
    return q;
}


int queue_pop(Queue *q){
    int aux;
    int check;
    
    if(q==NULL){
        printf("ERROR: Queue is null\n");
        return -1;
    }
    if(queue_isEmpty(q)){
        printf("ERROR: Queue is empty\n");
        return -1;
    }
    
    aux = q->item[q->head];
    
    if(q->end == 0){
        check = q->max_size-1;
    }else{
        check=q->end-1;
    }
    
    if(q->head == check){
        q->head = -1;
    }else{
        if(q->head == q->max_size-1){
            q->head = 0;
        }else{
            q->head++;
        }
    }
    
    return aux;
}

