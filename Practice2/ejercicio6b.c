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
    void handler_SIGTERM();
    
    pid = fork();
    if (pid == 0){
        if(signal(SIGTERM, handler_SIGTERM)==SIG_ERR){
            printf("Error estableciendo el nuevo manejador de SIGTERM.\n");
            exit(EXIT_FAILURE);
        }
        
        if(sigfillset(&set)){
                printf("Error en sigfillset(&set)");
                exit(EXIT_FAILURE);
        }
        if(sigdelset(&set, SIGTERM));
            
        if(sigprocmask(SIG_BLOCK, &set, &oset)){
                printf("Error bloqueando el set de señales\n");
                exit(EXIT_FAILURE);
        }
        while(1){
            
            for (counter = 0; counter < NUM; counter++){
                printf("%d\n", counter);
                sleep(1);
            }

            sleep(3);
        }
        
    }
    else{
        sleep(40);
        kill(pid, SIGTERM);
    }
    exit(EXIT_SUCCESS);
}

void handler_SIGTERM(int sig){
    printf("Soy %d y he recibido la señal SIGTERM\n", getpid());
    exit(EXIT_SUCCESS);
}