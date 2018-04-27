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
#include "semaforos.h"


#define KEY 1300
#define SIZE 20
#define NUM_OPERATIONS 50
#define MAX_PRICE 300
#define MAX_AMOUNT 1000
#define REMOVE 900
#define BUFFER_SIZE 128

char *nombre_transaccion(int num);

char *nombre_cajero(int num);

int aleat_num(int inf, int sup);

int main(int argc, char **argv){
    int num_cash, key, check_sem1, semid;
    int *sem_array;
    char buff[BUFFER_SIZE], status[BUFFER_SIZE];
    char *name, *fcash;
    FILE *in, *faux;
    int i, j, k, pid, price, amount, id, total, n_fin = 0;
    void handler_SIGUSR1();
    
    if(argc != 2){
        printf("ERROR Parametros de Entrada:\n");
        printf("Por favor, introduzca después del nombre el numero de cajeros\n");
        exit(EXIT_FAILURE);
    }
    
    num_cash = atoi(argv[1]); /* Numero de cajeros */
    
    key = ftok("/workspace/", KEY);
    /*Inicializacion de semaforos*/
    check_sem1 = Crear_Semaforo(key, num_cash+1, &semid);
    if(check_sem1 == 1){
        printf("Error. El semaforo que se intenta crear ya existe\n");
        exit(EXIT_FAILURE);
    }else if(check_sem1 == ERROR){
        printf("Error en la creacion del conjunto de semaforos\n");
        exit(EXIT_FAILURE);
    }
    
    sem_array = (int *) malloc((num_cash+1)*sizeof(int));
    if(!sem_array){
        printf("Error reservando memoria para el array de ids de semaforos involucrados\n");
        exit(EXIT_FAILURE);
    }
    for(i=0; i<=num_cash; i++){
        sem_array[i] = i;
    }
    
    if(UpMultiple_Semaforo(semid, num_cash+1, SEM_UNDO, sem_array)==ERROR){
        printf("Error UpMultiple_Semaforo levantando todos los semaforos al principio\n");
        free(sem_array);
        exit(EXIT_FAILURE);
    }
    free(sem_array);
    
    /**Creacion de los ficheros con las 50 transacciones que van a hacer*/
    for(i=1; i<=num_cash; i++){
        name = nombre_transaccion(i);
        in = (FILE *) fopen(name, "w");
        if(!in){
            printf("Fallo creando el fichero clientes %d.\n", i);
            exit(EXIT_FAILURE);
        }
        fprintf(in, "%d", aleat_num(0,MAX_PRICE)); /* Una iteracion fuera del bucle porque no nos interesa el \n */
        for(j=1; j<NUM_OPERATIONS; j++){
            fprintf(in, "\n%d", aleat_num(0,MAX_PRICE));
        }
        free(name);
        fclose(in);
    }
    
    /**Creacion del fichero en el que guardaremos la cola de procesos que pide que se le retiren 900 euros*/
    in = (FILE *) fopen("cola.txt", "w");
    if(!in){
        printf("Fallo en la creación del fichero 'cola.txt'.\n");
        exit(EXIT_FAILURE);
    }
    fclose(in);
    
    /**Creacion del fichero donde guardaremos toda la recaudacion*/
    in = (FILE *) fopen("total.txt", "w");
    if(!in){
        printf("Fallo en la creación del fichero 'total.txt'.\n");
        exit(EXIT_FAILURE);
    }
    fclose(in);
    
    /* Asignamos un nuevo manejador a la señal SIGUSR1 */
    if(signal(SIGUSR1, handler_SIGUSR1)==SIG_ERR){ 
        printf("Error estableciendo el nuevo manejador de SIGUSR1.\n"); /* Error */
        exit(EXIT_FAILURE);
    }
    
    /**Creacion de los cajeros como procesos*/
    for(i=1; i<=num_cash; i++){
        if((pid = fork())<0){
            printf("Error al crear los cajeros (fork).\n");
            exit(EXIT_FAILURE);
        }
        if(pid == 0){
            break;
        }
    }
    
    /**Cajeros*/
    if(pid == 0){
        name = nombre_transaccion(i);
        fcash = nombre_cajero(i);
        /**Inicializamos nuestra caja a 0*/
        in = (FILE *) fopen(fcash, "w");
        if(!in){
            printf("Error intentando abrir un archivo de texto (1).\n");
            exit(EXIT_FAILURE);
        }
        fprintf(in,"%d\n", 0);
        fclose(in);
        free(fcash);
            
        for(j=0; j<NUM_OPERATIONS; j++){
            /**Leemos la operacion que queremos hacer*/
            in = (FILE *) fopen(name, "r");
            if(!in){
                printf("Error intentando abrir un archivo de texto (2).\n");
                exit(EXIT_FAILURE);
            }
            for(k=0; k<=j; k++){
                fscanf(in, "%d", &price);
            }
            fclose(in);
            
            if(Down_Semaforo(semid, i, SEM_UNDO) == ERROR){
                    printf("Error bajando el semaforo de la caja %d\n", id);
                    free(name);
                    exit(EXIT_FAILURE);
            }
            
            /**Leemos el dinero que tenemos en el cajero*/
            in = (FILE *) fopen(fcash, "r");
            if(!in){
                printf("Error intentando abrir un archivo de texto (3).\n");
                exit(EXIT_FAILURE);
            }
            fscanf(in, "%d\n", &amount);
            fclose(in);
            /**Guardamos la nueva cantidad de dinero*/
            amount += price;
            in = fopen(fcash, "w");
            if(!in){
                printf("Error intentando abrir un archivo de texto (4).\n");
                exit(EXIT_FAILURE);
            }
            fprintf(in,"%d",amount);
            fclose(in);
            
            if(Up_Semaforo(semid, i, SEM_UNDO) == ERROR){
                printf("Error levantando el semaforo de cola.txt en la caja %d\n", i);
                exit(EXIT_FAILURE);
            }
            
            /**Si el cajero tiene mas dinero del permitido, avisamos al padre*/
            
            if(amount > MAX_AMOUNT){
                if(Down_Semaforo(semid, 0, SEM_UNDO) == ERROR){
                    printf("Error bajando el semaforo de cola.txt en la caja %d\n", i);
                    exit(EXIT_FAILURE);
                }
                in = (FILE *) fopen("cola.txt","a");
                if(!in){
                    printf("Error intentando abrir un archivo de texto (5).\n");
                    exit(EXIT_FAILURE);
                }
                fprintf(in,"\n%d %s" ,i,"filled");
                fclose(in);
                
                
                if(kill(getppid(),SIGUSR1)){ /* Si kill != 0 entonces ERROR */
                    printf("%s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                if(Up_Semaforo(semid, 0, SEM_UNDO) == ERROR){
                printf("Error levantando el semaforo de cola.txt en la caja %d\n", i);
                exit(EXIT_FAILURE);
                }
            }
            
            sleep(aleat_num(1,5));
        }
        if(Down_Semaforo(semid, 0, SEM_UNDO) == ERROR){
            printf("Error bajando el semaforo de cola.txt en la caja %d\n", i);
            exit(EXIT_FAILURE);
        }
        /*Indicamos que el cajero ha finalizado sus transacciones*/
        in = (FILE *) fopen("cola.txt","a");
        if(!in){
            printf("Error intentando abrir un archivo de texto (5).\n");
            exit(EXIT_FAILURE);
        }
        fprintf(in,"\n%d %s" ,i,"fin");
        fclose(in);
        if(kill(getppid(),SIGUSR1)){ /* Si kill != 0 entonces ERROR */
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        if(Up_Semaforo(semid, 0, SEM_UNDO) == ERROR){
                printf("Error levantando el semaforo de cola.txt en la caja %d\n", i);
                exit(EXIT_FAILURE);
            }
    }
    
    /**Proceso padre*/
    if(pid != 0){
        while(n_fin < num_cash){
            
            pause();
            
            if(Down_Semaforo(semid, 0, SEM_UNDO) == ERROR){
                printf("Error bajando el semaforo de cola.txt en el proceso padre\n");
                exit(EXIT_FAILURE);
            }
            
            in = (FILE *)fopen("cola.txt","r");
            if(!in){
                printf("Error abriendo el fichero 'cola.txt'");
                exit(EXIT_FAILURE);
            }
            /**Leemos la cola de cajeros que han solicitado que se retire su dinero*/
            fgets(buff, BUFFER_SIZE, in);
            sscanf(buff, "%d", &id);
            while(!feof(in)){
                fgets(buff, BUFFER_SIZE, in);
                sscanf(buff, "%d %s", &id, status);
                name = nombre_cajero(id);
                
                if(Down_Semaforo(semid, id, SEM_UNDO) == ERROR){
                    printf("Error bajando el semaforo de la caja %d en el proceso padre\n", id);
                    fclose(in);
                    free(name);
                    exit(EXIT_FAILURE);
                }
                
                faux = (FILE *) fopen(name,"r");
                if(!faux){
                    printf("Error abriendo para leer el fichero de caja en el proceso padre\n");
                    fclose(in);
                    free(name);
                    exit(EXIT_FAILURE);
                }
                
                fscanf(faux, "%d", &price);
                fclose(faux);
                
                faux = (FILE *) fopen(name, "w");
                if(!faux){
                    printf("Error abriendo para escribir el fichero de caja en el proceso padre\n");
                    fclose(in);
                    free(name);
                    exit(EXIT_FAILURE);
                }
                
                if(!strcmp(status, "filled") && price >= MAX_AMOUNT){
                    amount = price - REMOVE;
                    fprintf(faux, "%d", amount);
                    
                    total += REMOVE;
                }else if(!strcmp(status, "fin")){
                    total += price;
                    fprintf(faux, "%d", 0);
                    n_fin ++;
                }
                free(name);
                fclose(faux);
                
                if(Up_Semaforo(semid, id, SEM_UNDO) == ERROR){
                    printf("Error levantando el semaforo de la caja %d en el proceso padre\n", id);
                    fclose(in);
                    exit(EXIT_FAILURE);
                }
            }
            fclose(in);
            if(Up_Semaforo(semid, 0, SEM_UNDO) == ERROR){
                printf("Error levantando el semaforo de cola.txt en el proceso padre\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    if(Borrar_Semaforo(semid)==ERROR){
        printf("Error eliminando conjunto de semaforos\n");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}



char *nombre_transaccion(int num){
    char *text=NULL, *text2=NULL;
    
    text = (char *) malloc(SIZE * sizeof(char));
    if(!text){
        printf("Error reservando memoria para text en la funcion 'nombre_transaccion'\n");
        return NULL;
    }
    
    text2 = (char *) malloc(SIZE * sizeof(char));
    if(!text2){
        printf("Error reservando memoria para text2 en la funcion 'nombre_transaccion'\n");
        return NULL;
    }
    
    strcpy(text, "clientesCaja");
    sprintf(text2,"%d",num);
    strcat(text2,".txt");
    strcat(text,text2);
    free(text2);
    return text;
}


char *nombre_cajero(int num){
    char *text=NULL, *text2=NULL;
    
    text = (char *) malloc(SIZE * sizeof(char));
    if(!text){
        printf("Error reservando memoria para text en la funcion 'nombre_cajero'\n");
        return NULL;
    }
    
    text2 = (char *) malloc(SIZE * sizeof(char));
    if(!text2){
        printf("Error reservando memoria para text2 en la funcion 'nombre_cajero'\n");
        return NULL;
    }
    
    strcpy(text, "cajero");
    sprintf(text2,"%d",num);
    strcat(text2,".txt");
    strcat(text,text2);
    free(text2);
    return text;
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