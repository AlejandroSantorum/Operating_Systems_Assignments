#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

char *nombre_fichero(int num);

int main(){
  int n_fich, suma=0;
  char buff[128], str[128];
  FILE *in=NULL;
  
  in = (FILE *) fopen("fichero_prueba.txt", "w");
  if(!in){
      printf("Error con el puto fichero\n");
      exit(-1);
  }
  
  fprintf(in, "\n%d %s", 1, "lleno");
  fprintf(in, "\n%d %s", 2, "lleno");
  fprintf(in, "\n%d %s", 3, "lleno");
  fprintf(in, "\n%d %s", 4, "terminado");
  fprintf(in, "\n%d %s", 5, "terminado");
  
  fclose(in);
  
  in = (FILE *) fopen("fichero_prueba.txt", "r+");
  if(!in){
      printf("Error con el puto fichero\n");
      exit(-1);
  }
  
  fgets(buff, 128, in);
  sscanf(buff, "%d", &n_fich);
  
  while(!feof(in)){
      fgets(buff, 128, in);
      sscanf(buff, "%d %s", &n_fich, str);
      suma += n_fich;
      if(!strcmp(str, "lleno")){
          printf("%d esta lleno\n", n_fich);
          //fprintf(in, "%s\n", "leido-lleno");
      }
      else if(!strcmp(str, "terminado")){
          printf("%d esta terminado\n", n_fich);
          //fprintf(in, "%s\n", "leido-ter");
      }
  }
  fclose(in);
  
  printf("Suma total = %d", suma);

  exit(EXIT_SUCCESS);
}
