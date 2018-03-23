#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define N_CHILDS 4

int main(){
    int pid, i = 0;
    
    for(i = 0; i<N_CHILDS; i++){
        if((pid = fork())<0){
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        /* Procesos hijos */
        if(!pid){
            printf("Soy el proceso hijo %d\n", getpid());
            fflush(stdout);
            sleep(8);
            printf("Soy el proceso hijo %d y ya me toca terminar\n",getpid());
            break;
            /* Gracias a este break, los procesos hijos nunca ejecutarán el código de abajo */
            /* De todas formas, este programa no deja ni que llegue al segundo printf
            *  debido a la señal de terminacion controlada kill(pid, SIGTERM) */
        }
        
        /* Proceso padre */
        sleep(3);
        if(kill(pid,SIGTERM)){ /* Si kill != 0 entonces ERROR */
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    
    exit(EXIT_SUCCESS);
}