#include semaforos.h

#define NUM_CAJEROS 5
#define NUM_OPERACIONES 50
#define MAX_PRICE 300
#define MAX_CAJA 1000

int main(){
    char *text, *cajero;
    FILE *in;
    int pid, pos, price, amount;
    
    /**Creacion de los ficheros con las 50 transacciones que van a hacer*/
    for(int i=1; i<=NUM_CAJEROS; i++){
        strcpy(text, nombre_fichero(i))
        in = fopen(text,"w");
        if(!in){
            printf("Fallo en la creación de ficheros.\n");
            exit(EXIT_FAILURE);
        }
        for(int j=0; j<NUM_OPERACIONES; j++){
            fprintf(in, "%d\n", aleat_num(0,MAX_PRICE));
        }
        fclose(in);
    }
    
    /**Creacion de los cajeros como procesos*/
    for(i=1;i<=NUM_CAJEROS;i++){
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
        pos = 0;
        strcpy(text, nombre_fichero(i));
        strcpy(cajero, nombre_cajero(i));
        /**Inicializamos nuestra caja a 0*/
        in = fopen(cajero, "w");
        if(!in){
            printf("Error intentando abrir un archivo de texto.\n");
            exit(EXIT_FAILURE);
        }
        fprintf(in,"0\n");
        fclose(in);
            
        for(int j=0; j<NUM_OPERACIONES; j++){
            /**Leemos la operacion que queremos hacer*/
            in = fopen(text, "r");
            if(!in){
                printf("Error intentando abrir un archivo de texto.\n");
                exit(EXIT_FAILURE);
            }
            if(fseek(in,pos,SEEK_SET)){
                printf("Error con fseek\n");
            }
            fscanf(in, "%d\n", &price);
            pos = ftell(in);
            fclose(in);
            
            /**Leemos el dinero que tenemos en el cajero*/
            in = fopen(cajero, "r");
            if(!in){
                printf("Error intentando abrir un archivo de texto.\n");
                exit(EXIT_FAILURE);
            }
            fscanf(in, "%d\n", &amount);
            fclose(in);
            /**Guardamos la nueva cantidad de dinero*/
            amount += price;
            in = fopen(cajero, "w");
            if(!in){
                printf("Error intentando abrir un archivo de texto.\n");
                exit(EXIT_FAILURE);
            }
            fprintf(in,"%d\n",amount);
            fclose(in);
            /**Si el cajero tiene mas dinero del permitido, avisamos al padre*/
            if(amount > MAX_CAJA){
                if(kill(getppid(),SIGUSR1)){ /* Si kill != 0 entonces ERROR */
                    printf("%s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            wait(aleat_num(1,5));
        }
    }
}


char *nombre_fichero(int num){
    char text[20], text2[10];
    strcpy(text, "clientesCaja");
    sprintf(text2,"%d",num);
    strcat(text2,".txt");
    strcat(text,text2);
    return text;
}

char *nombre_cajero(int num){
    char text[10], text2[10];
    strcpy(text, "cajero");
    sprintf(text2,"%d",num);
    strcat(text2,".txt");
    strcat(text,text2);
    return text;
}

int aleat_num(int inf, int sup){
    int result = 0;
    
    if(inf == sup){
        return sup;
    }
    
    else if (inf > sup){
        printf("ERROR: Límite inferior mayor que el límite superior.\n");
        exit(-1);
    }

    result = (inf + ((int) ((((double)(sup-inf+1)) * rand())/(RAND_MAX + 1.0))));
    
    return result;
}