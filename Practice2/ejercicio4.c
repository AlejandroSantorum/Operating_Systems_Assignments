#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>

#define N_PRINT 10

int is_valid_integer(char *input);

void handler_SIGUSR1(int sig);


int main(int argc, char *argv[]){
    int pid=0, last_pid = -1, i, j, childs;
    int pfather;
    void handler_SIGUSR1();
    
    if(argc != 2){
        printf("ERROR. Parámetros de entrada incorrectos.\n");
        printf("Por favor, introduzca el nombre del programa seguido\n");
        printf("del numero de procesos hijos que se quiere generar.\n");
        exit(EXIT_FAILURE);
    }
    
    if(!is_valid_integer(argv[1])) exit(EXIT_FAILURE);
    
    childs = atoi(argv[1]);
    
    pfather = getpid();
    
    if(signal(SIGUSR1, handler_SIGUSR1)==SIG_ERR){
        printf("Error estableciendo el nuevo manejador de SIGUSR1.\n");
        exit(EXIT_FAILURE);
    }
    
    for(i=0; i<childs; i++){
        if((pid = fork())<0){
            printf("%s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        if(!pid){
            if(last_pid != -1){
                kill(last_pid, SIGTERM);
            }
            
            for(j = 0; j < N_PRINT; j++){
                printf("Soy %d y estoy trabajando\n", getpid());
                fflush(stdout);
                sleep(1);
            }
            sleep(5);
            kill(pfather, SIGUSR1);
            while(1){
                /* Se sigue imprimiento mientras el proceso padre
                 * no recoja la señal mandada y responda a ella.*/
                printf("Soy %d y estoy trabajando despues de enviar la señal\n", getpid());
                fflush(stdout);
                sleep(1);
            }
        }
        else{
            pause();
            last_pid = pid;
        }
    }
    
    kill(last_pid,SIGTERM);
    
    while(wait(NULL)<0);
    
    return(EXIT_SUCCESS);
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
   return -1;
}


void handler_SIGUSR1(int sig){
    return;
}