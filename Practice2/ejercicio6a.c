#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>

#define NUM 5

int main(int argc, char *argv[]){
    int pid, counter;
    sigset_t set, oset;
    
    pid = fork();
    if (pid == 0){
        alarm(40);
        
        while(1){
            if(sigemptyset(&set)){
                printf("Error en sigemptyset(&set)");
                exit(EXIT_FAILURE);
            }
            if(sigemptyset(&oset)){
                printf("Error en sigemptyset(&oset)");
                exit(EXIT_FAILURE);
            }
            
            if(sigaddset(&set, SIGUSR1)){
                printf("Error incluyendo la señal SIGUSR1\n");
                exit(EXIT_FAILURE);
            }
            if(sigaddset(&set, SIGUSR2)){
                printf("Error incluyendo la señal SIGUSR2\n");
                exit(EXIT_FAILURE);
            }
            if(sigaddset(&set, SIGALRM)){
                printf("Error incluyendo la señal SIGALRM\n");
                exit(EXIT_FAILURE);
            }
            
            if(sigprocmask(SIG_BLOCK, &set, &oset)){
                printf("Error bloqueando el set de señales\n");
                exit(EXIT_FAILURE);
            }
            
            
            for (counter = 0; counter < NUM; counter++){
                printf("%d\n", counter);
                sleep(1);
            }
            
            
            if(sigdelset(&set, SIGUSR2)){
                printf("Error al desbloquear la señal SIGUSR1\n");
                exit(EXIT_FAILURE);
            }
            
            if(sigprocmask(SIG_UNBLOCK, &set, &oset)){
                printf("Error bloqueando el set de señales\n");
                exit(EXIT_FAILURE);
            }
            
            sleep(3);
        }
    }
    while(wait(NULL)>0);
    return EXIT_SUCCESS;
}

