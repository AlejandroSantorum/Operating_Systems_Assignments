

#include "semaforos.h"

#define ERROR -1
#define OK 0

int Inicializar_Semaforo(int semid, unsigned short *array){
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  }arg;
  arg.array = array;

  if(semctl(semid, sizeof(array), SETALL, arg)==-1){
    printf("Error inicializando el semaforo %d",semid);
    return ERROR;
  }

  return OK;
}


int Borrar_Semaforo(int semid){
  if(semctl(semid, 0, IPC_RMID)==-1){
    return ERROR;
  }
  return OK;
}


int Crear_Semaforo(key_t key, int size, int *semid){
  *semid = semget(key, size, IPC_CREAT|IPC_EXCL);
  if(*semid == -1 && errno == EEXIST){
    return 1;
  }
  if(*semid == -1){
    return ERROR;
  }
  return 0;
}

int Down_Semaforo(int id, int num_sem, int undo){
  struct sembuf sops;
  
  sops.sem_num = num_sem;
  sops.sem_op = -1;
  sops.sem_flg = undo;
  
  if(semop(id, &sops, 1)==-1){
    printf("Error bajando el semaforo de id %d", id);
    return ERROR;
  }
  
  return OK;
}

int DownMultiple_Semaforo(int id, int size, int undo, int *active){
  struct sembuf sops[size];
  
  
  
  for(int i=0; i < size; i++){
    sops[i].sem_num = active[i];
    sops[i].sem_op = -1;
    sops[i].sem_flg = undo;
  }
  
  if(semop(id, sops, size)==-1){
    printf("Error bajando el semaforo de id %d", id);
    return ERROR;
  }
  
  return OK;
}

int Up_Semaforo(int id, int num_sem, int undo){
  struct sembuf sops;
  
  sops.sem_num = num_sem;
  sops.sem_op = 1;
  sops.sem_flg = undo;
  
  if(semop(id, &sops, 1)==-1){
    printf("Error subiendo el semaforo de id %d", id);
    return ERROR;
  }
  
  return OK;
}

int UpMultiple_Semaforo(int id, int size, int undo, int *active){
  struct sembuf sops[size];
  
  for(int i=0; i < size; i++){
    sops[i].sem_num = active[i];
    sops[i].sem_op = 1;
    sops[i].sem_flg = undo;
  }
  
  if(semop(id, sops, size)==-1){
    printf("Error subiendo el semaforo de id %d", id);
    return ERROR;
  }
  
  return OK;
}