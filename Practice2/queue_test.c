#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue_int.h"

int main(){
    int i;
    Queue *q=NULL;
    
    q = queue_create(5);
    if(q==NULL){
        printf("ERROR queue_create\n");
        exit(EXIT_FAILURE);
    }
    
    printf("\nPushing 5 items in a queue of max_size=5: \n");
    for(i = 1; i<=5; i++){
        q = queue_push(q, i);
        if(q==NULL){
            printf("ERROR queue_push en %d", i);
        }
    }
    printf("\nTrying to push a 6th element: it should return an error:");
    queue_push(q, 6);
    
    printf("\n==================================\n\n");
    printf("Full? ==> %d\n", queue_isFull(q));
    
    printf("Empty? ==> %d\n", queue_isEmpty(q));
    
    printf("\n==================================\n\n");
    
    
    for(i=0; i<5; i++){
        printf("%d\n", queue_pop(q));
    }
    
    printf("\n==================================\n\n");
    
    printf("Full? ==> %d\n", queue_isFull(q));
    
    printf("Empty? ==> %d\n", queue_isEmpty(q));
    
    printf("\n==================================\n\n");
    
    printf("\nTrying to pop a 6th element: it should return an error:");
    printf("%d\n", queue_pop(q));
    
    queue_destroy(q);
    
    exit(EXIT_SUCCESS);
}