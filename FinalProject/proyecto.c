#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <syslog.h>
#include "semaforos.h"


#define N_ARGS 6
#define MIN_PROC 3

#define MAX_HORSES 10
#define MIN_HORSES 2
#define MAX_RACE 200
#define MIN_RACE 20
#define MAX_BETTOR 100
#define MIN_BETTOR 1
#define MAX_WINDOWS 30
#define MIN_WINDOWS 1
#define MAX_MONEY 1000000
#define MIN_MONEY 100

#define PROC_DISPLAYER 0
#define PROC_MANAGER 1
#define PROC_BETTOR 2

#define SEM_SIZE 3
#define MARKET_SEM 0
#define PROFIT_SEM 1
#define FILE_SEM 2

#define PATH "/bin/bash"
#define ERROR -1
#define SECONDS 30
#define NAME_SIZE 20

struct msgbuf{
    long mtype; /* type of message */
    struct{
        char name[NAME_SIZE];
        int bettor_id;
        int horse_id;
        float bet;
    }info; /* information of message */
};

typedef struct{
    float horse_bet[MAX_HORSES];
    float horse_rate[MAX_HORSES];
    float total;
}market_rates_struct;


void init_syslog(){
    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("Proyecto_final_SOPER_logger", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}
int is_valid_integer(char *input);
int aleat_num(int inf, int sup);
float float_rand( float min, float max );
void* window_function(void* arg);
void handler_SIGUSR1_bettor(int sig);
void displayer_process(int bettor_process_pid, int betting_manager_process_pid);
void betting_manager_process(int n_horses, int n_bettor, int n_windows);
void bettor_process(int n_horses, int n_bettor, int money);
void caballo();


int semid, msgid, memid_market, memid_profit;
int n_horses;
int flag_bettor=0;
market_rates_struct *market_rates=NULL; /* Cotizaciones de los caballos */
float *profit=NULL; /* Posibles beneficios de cada apostador */

int main(int argc, char **argv){
    int len_race, n_bettor, n_windows, money, n_proc;
    int i, j;
    int semkey, msgkey, memkey_market, memkey_profit;
    int *pids=NULL;
    unsigned short ini_sem[SEM_SIZE];
    
    /* Comprobacion inicial de errores */
    if(argc != N_ARGS){
        printf("Argumentos que se deben especificar:\n");
        printf("-Numero de caballos de la carrera(max 10)\n");
        printf("-Longitud de la carrera\n");
        printf("-Numero de apostadores(max 100)\n");
        printf("-Numero de ventanillas\n");
        printf("-Dinero de cada apostante\n");
        exit(EXIT_FAILURE);
    }
    
    for(i = 1; i < N_ARGS; i++){
        if(!is_valid_integer(argv[i])){
            printf("Error en el parametro %d: entero no valido o 0\n",i);
            exit(EXIT_FAILURE);
        }
    }
    
    n_horses = atoi(argv[1]);
    len_race = atoi(argv[2]);
    n_bettor = atoi(argv[3]);
    n_windows = atoi(argv[4]);
    money = atoi(argv[5]);
    
    if(n_horses > MAX_HORSES || n_horses < MIN_HORSES){
        printf("Error. El numero de caballos debe estar entre %d y %d\n", MIN_HORSES, MAX_HORSES);
        exit(EXIT_FAILURE);
    }
    
    if(len_race < MIN_RACE || len_race > MAX_RACE){
        printf("Error. La longitud de la carrera debe ser como minimo %d, y como maximo %d\n", MIN_RACE, MAX_RACE);
        exit(EXIT_FAILURE);
    }
    
    if(n_bettor < MIN_BETTOR || n_bettor > MAX_BETTOR){
        printf("Error. El numero de apostadores debe ser como minimo %d, y como maximo %d\n", MIN_BETTOR, MAX_BETTOR);
        exit(EXIT_FAILURE);
    }
    
    if(n_windows < MIN_WINDOWS || n_windows > MAX_WINDOWS){
        printf("Error. El numero de ventanillas debe ser como minimo %d, y como maximo %d\n", MIN_WINDOWS, MAX_WINDOWS);
        exit(EXIT_FAILURE);
    }
    
    if(money < MIN_MONEY || money > MAX_MONEY){
        printf("Error. La cantidad de dinero de un apostador debe ser como minimo %d, y como maximo %d\n", MIN_MONEY, MAX_MONEY);
        exit(EXIT_FAILURE);
    }
    /* Fin de la comprobacion inicial de errores */
    
    n_proc = n_horses+MIN_PROC;
    srand(time(NULL));
    
    /* Array de pids de todos los procesos a crear */
    pids = (int*) malloc(n_proc * sizeof(int));
    if(!pids){
        perror("Error al reservar memoria para el array de pids");
        exit(EXIT_FAILURE);
    }
    
    /* CREACION COLA DE MENSAJES */
    msgkey = ftok(PATH, aleat_num(1000, 3000));
    if(msgkey == ERROR){
        free(pids);
        perror("Error obteniendo la key para la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    msgid = msgget(msgkey, IPC_CREAT|IPC_EXCL|0660);
    if(msgid==ERROR && errno==EEXIST){
        msgid = msgget(msgkey, IPC_CREAT|0660);
    }else if(msgid == ERROR){
        free(pids);
        perror("Error creando la cola de mensajes");
        exit(EXIT_FAILURE);
    }
    /* FIN CREACION COLA MENSAJES */
    
    
    /* CREACION DE SEMAFOROS */
    semkey = ftok(PATH, aleat_num(3001, 6000));
    if(semkey == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        free(pids);
        perror("Error obteniendo la key para los semaforos");
        exit(EXIT_FAILURE);
    }
    if(Crear_Semaforo(semkey, SEM_SIZE, &semid) == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        free(pids);
        perror("Error creando el conjunto de semaforos");
        exit(EXIT_FAILURE);
    }
    for(j=0; j<SEM_SIZE; j++){
        ini_sem[j] = 1;
    }
    if(Inicializar_Semaforo(semid, ini_sem) == ERROR){/*Ponemos los semáforos a 1*/
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        free(pids);
        Borrar_Semaforo(semid);
        perror("Error inicializando a 1 el conjunto de semaforos");
        exit(EXIT_FAILURE);
    }
    /* FIN CREACION DE SEMAFOROS */ 
    
    
    /* CREACION MEMORIA COMPARTIDA */
    memkey_market = ftok(PATH, aleat_num(6001,9000));
    if(memkey_market == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        Borrar_Semaforo(semid);
        free(pids);
        perror("Error obteniendo la key para la zona compartida de las cotizaciones");
        exit(EXIT_FAILURE);
    }
    memid_market = shmget(memkey_market, sizeof(market_rates_struct), IPC_CREAT|0660);
    if(memid_market == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        Borrar_Semaforo(semid);
        free(pids);
        perror("Error al crear la memoria compartida para las cotizaciones.");
        exit(EXIT_FAILURE);
    }
    market_rates = (market_rates_struct *) shmat(memid_market, (char *)0, 0);
    if(market_rates == ((void*)ERROR)){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        shmctl(memid_market, IPC_RMID, NULL);
        Borrar_Semaforo(semid);
        free(pids);
        perror("Error adjuntando la zona de memoria para las cotizaciones");
        exit(EXIT_FAILURE);
    }
    market_rates->total = 0.0;
    
    memkey_profit = ftok(PATH, aleat_num(9001, 12000));
    if(memkey_profit == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        shmdt(market_rates);
        shmctl(memid_market, IPC_RMID, NULL);
        Borrar_Semaforo(semid);
        free(pids);
        perror("Error obteniendo la key para la zona compartida de los beneficios de los apostadores");
        exit(EXIT_FAILURE);
    }
    memid_profit = shmget(memkey_profit, sizeof(float)*n_bettor, IPC_CREAT|0660);
    if(memid_profit == ERROR){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        shmdt(market_rates);
        shmctl(memid_market, IPC_RMID, NULL);
        Borrar_Semaforo(semid);
        free(pids);
        perror("Error creando la memoria compartida para los beneficios de los apostadores");
        exit(EXIT_FAILURE);
    }
    profit = (float *) shmat(memid_profit, (char*)0, 0);
    if(profit == ((void*)ERROR)){
        msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
        shmdt(market_rates);
        shmctl(memid_market, IPC_RMID, NULL);
        shmctl(memid_profit, IPC_RMID, NULL);
        Borrar_Semaforo(semid);
        free(pids);
        perror("Error creando la memoria compartida para los beneficios de los apostadores");
        exit(EXIT_FAILURE);
    }
    /* FIN CREACION MEMORIA COMPARTIDA */
    
    
    
    
    
    for(i=0; i<n_proc; i++){
        if((pids[i] = fork())==ERROR){
            msgctl(msgid, IPC_RMID, NULL); /* Sin comprobacion de error porque precisamente estamos terminando el programa */
            shmdt(market_rates);
            shmctl(memid_market, IPC_RMID, NULL);
            shmdt(profit);
            shmctl(memid_profit, IPC_RMID, NULL);
            Borrar_Semaforo(semid);
            free(pids);
            perror("Error al realizar fork");
            exit(EXIT_FAILURE);
        }
        if(!pids[i]){
            break;
        }
    }
    
    if(i==PROC_DISPLAYER){
        displayer_process(pids[PROC_BETTOR], pids[PROC_MANAGER]);
    }else if(i==PROC_MANAGER){
        betting_manager_process(n_horses, n_bettor, n_windows);
    }else if(i==PROC_BETTOR){
        bettor_process(n_horses, n_bettor, money);
    }else if(i>PROC_BETTOR && i<n_proc){
        caballo();
    }else{/* Proceso padre*/
        pause();
    }
    
    msgctl(msgid, IPC_RMID, NULL);
    shmdt(market_rates);
    shmctl(memid_market, IPC_RMID, NULL);
    shmdt(profit);
    shmctl(memid_profit, IPC_RMID, NULL);
    Borrar_Semaforo(semid);
    free(pids);
    exit(EXIT_SUCCESS);
}


void displayer_process(int bettor_process_pid, int betting_manager_process_pid){
    int j, k;
    
    for(j=SECONDS; j>0; j--){
        sleep(1);
        if(Down_Semaforo(semid, MARKET_SEM, 0) == ERROR){
            perror("Error bajando el semaforo de las cotizaciones en el monitor");
            Borrar_Semaforo(semid);
            exit(EXIT_FAILURE);
        }
        printf("\t===> SEGUNDOS RESTANTES PARA LA CARRERA = %d <===\n", j);
        printf("COTIZACIONES ACTUALES DE LOS CABALLOS:\n");
        for(k=0; k<n_horses; k++){
            printf("Cotizacion caballo ID %d = %f\n", k+1, market_rates->horse_rate[k]);
        }
        if(Up_Semaforo(semid, MARKET_SEM, 0) == ERROR){
            perror("Error levantando el semaforo de las cotizaciones en el monitor");
            Borrar_Semaforo(semid);
            exit(EXIT_FAILURE);
        }
    }
    
    kill(bettor_process_pid, SIGUSR1);
    kill(betting_manager_process_pid, SIGUSR1);
    sleep(1); /* Ayuda a la sincronizacion */
    printf("\n\t ======================================\n");
    printf("APUESTAS FINALIZADAS. La carrera comenzara en breves instantes");
    printf("\n\t ======================================\n");
    
    kill(getppid(), SIGUSR1);
    exit(EXIT_SUCCESS);
}


void betting_manager_process(int n_horses, int n_bettor, int n_windows){
    pthread_t *thr=NULL;
    int j;
    
    /* Inicializamos las apuestas */
    for(j=0; j<n_horses; j++){
        market_rates->horse_bet[j] = 1.0;
        market_rates->total += 1.0;
        market_rates->horse_rate[j] = market_rates->total/market_rates->horse_bet[j];
    }
    for(j=0; j<n_bettor; j++){
        profit[j] = 0.0;
    }
    /* --------------------- */
    
    thr = (pthread_t*) malloc(n_windows * sizeof(pthread_t));
    if(!thr){
        printf("Error reservando los hilos en el gestor de apuestas");
        exit(EXIT_FAILURE);
    }
    for(j=0; j<n_windows; j++){
        if(pthread_create(&thr[j], NULL, window_function, (void *) &j)!= 0){
            free(thr);
            perror("Error en la creación de los hilos ventanilla");
            exit(EXIT_FAILURE);
        }
    }
    
    pause(); /* Esperando a que comience la carrera para terminar con los apuestas */
    
    for(j=0; j<n_windows; j++){ /* Las ventanillas no recogen mas apuestas */
        pthread_kill(thr[j], SIGINT); /* Finalizamos la ejecucion de los hilos */
    }
    
    exit(EXIT_SUCCESS);
}


void bettor_process(int n_horses, int n_bettor, int money){
    struct msgbuf msg_arg={0};
    void handler_SIGUSR1_bettor();
    int rand_horse;
    float rand_bet;
    int j;
    char aux[NAME_SIZE];
    
    if(signal(SIGUSR1, handler_SIGUSR1_bettor)==SIG_ERR){
        perror("Error estableciendo el nuevo manejador de SIGUSR1 en el proceso apostador");
        exit(EXIT_FAILURE);
    }
    
    msg_arg.mtype = 1;
    while(!flag_bettor){
        for(j=0; j<n_bettor; j++){
            rand_horse = aleat_num(1, n_horses);
            do{
                rand_bet = float_rand(0, money);
            }while(rand_bet == 0.0);
            strcpy(msg_arg.info.name, "Apostador-");
            sprintf(aux, "%d", j+1);
            strcpy(msg_arg.info.name, aux);
            msg_arg.info.bettor_id = j+1;
            msg_arg.info.horse_id = rand_horse;
            msg_arg.info.bet = rand_bet;
            if(msgsnd(msgid, (struct msgbuf *)&msg_arg, sizeof(msg_arg.info), 0) == ERROR){
                if(errno!=EIDRM && errno!=EINTR){
                    /* msgsnd puede devolver error si el proceso recibe una señal
                     * del sistema o si la cola de mensajes es eliminada, lo cual
                     * no es un error que deba ser reportado y subsanado */
                    perror("Error enviando mensaje desde el proceso apostador\n");
                    exit(EXIT_FAILURE);
                }
                
            }
            memset(msg_arg.info.name, 0, sizeof(msg_arg.info.name));
        }
    }
    exit(EXIT_SUCCESS);
}

void caballo(){
    /* ..... */
    exit(EXIT_SUCCESS);
}

void handler_SIGUSR1_bettor(int sig){
    flag_bettor=1; /* Proceso acabado */
    return;
}


void* window_function(void* arg){
    int window_id = *((int*)arg);
    struct msgbuf rcv={0};
    int j;
    float auxiliar_rate;
    FILE *f=NULL;
    
    while(1){
        if(msgrcv(msgid, (struct msgbuf *)&rcv, sizeof(rcv.info), 1, 0) == ERROR){
            if(errno == EIDRM || errno == EINTR){ /* Las apuestas han acabado */
                exit(EXIT_SUCCESS);
            }else{ /* Error */
                perror("Error recibiendo un mensaje en una ventanilla de apuestas");
                exit(EXIT_FAILURE);
            }
        }
        
        /* Comprobamos el caballo y hacemos las actualizaciones necesarias */
        if(rcv.info.horse_id > 0 && rcv.info.horse_id <= n_horses){
            /* Bajando semaforo de proteccion de memoria compartida
             *  para las posibles ganancias de los apostadores */
            if(Down_Semaforo(semid, PROFIT_SEM, 0) == ERROR){ 
                printf("Error bajando el semaforo de los apostadores en una ventanilla.\n");
                Borrar_Semaforo(semid);
                exit(EXIT_FAILURE);
            }
            /* Bajando semaforo de proteccion de memoria compartida
             *  para las cotizaciones de los caballos */
            if(Down_Semaforo(semid, MARKET_SEM, 0) == ERROR){ 
                printf("Error bajando el semaforo de las cotizaciones en una ventanilla.\n");
                Borrar_Semaforo(semid);
                exit(EXIT_FAILURE);
            }
            
            /* Actualizamos beneficios del apostador */
            /* beneficio = apuesta*cotizacion_caballo */
            auxiliar_rate = market_rates->horse_rate[rcv.info.horse_id-1];
            profit[rcv.info.bettor_id-1] = rcv.info.bet * auxiliar_rate;
            /* Actualizamos cotizaciones de cada caballo */
            market_rates->horse_bet[rcv.info.horse_id-1] += rcv.info.bet;
            market_rates->total += rcv.info.bet;
            for(j=0; j<n_horses; j++){
                market_rates->horse_rate[j] = market_rates->total / market_rates->horse_bet[j];
            }
            
            if(Up_Semaforo(semid, MARKET_SEM, 0) == ERROR){ 
                perror("Error subiendo el semaforo de las cotizaciones en una ventanilla.");
                Borrar_Semaforo(semid);
                exit(EXIT_FAILURE);
            }
            if(Up_Semaforo(semid, PROFIT_SEM, 0) == ERROR){ 
                perror("Error subiendo el semaforo de los apostadores en una ventanilla.");
                Borrar_Semaforo(semid);
                exit(EXIT_FAILURE);
            }
            
            
            /* Bajando semaforo de proteccion de memoria compartida
             *  para las cotizaciones de los caballos */
            if(Down_Semaforo(semid, FILE_SEM, 0) == ERROR){ 
                perror("Error bajando el semaforo del fichero de registro de apuestas.");
                Borrar_Semaforo(semid);
                exit(EXIT_FAILURE);
            }
            
            f = (FILE*) fopen("apuestas.txt", "a");
            if(!f){
                perror("Error abriendo el fichero de registro de apuestas en una ventanilla");
                Up_Semaforo(semid, FILE_SEM, 0);
                exit(EXIT_FAILURE);
            }
            fprintf(f, "%d %d %d %f %f\n", rcv.info.bettor_id, window_id, rcv.info.horse_id, auxiliar_rate, rcv.info.bet);
            fclose(f);
            
            if(Up_Semaforo(semid, FILE_SEM, 0) == ERROR){ 
                perror("Error subiendo el semaforo de las cotizaciones en una ventanilla.");
                Borrar_Semaforo(semid);
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
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
         return 0;
      }
    }
    if(!atoi(input)){
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

float float_rand( float min, float max ){
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}