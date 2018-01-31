#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char *argv[]){
  int size, rank , i;
  int start, end;
  int length;
  MPI_Status    status;
  int filesize = 100000;
  /* Initialize MPI */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if(size < 2){
    printf("Add another process pls .\n");
    exit(1);
  }   
  if(rank == 0) {
    FILE* f;  
    char fileName[100];
    printf("Enter file name you want to send :\n");
    scanf("%s", fileName);
    f = fopen(fileName,"r");
    double *data = (double*) malloc (filesize/size);
    fseek(f, filesize/size*rank, SEEK_SET);
    MPI_Send(data, 100, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
    printf("sent\n");
    /* close the file */
    fclose(f);
  } else { 
    FILE* fpa; 
    char fileName[100];
    double *data = (double*) malloc (filesize/size);
    MPI_Recv(data, 100, MPI_BYTE , 0, 0, MPI_COMM_WORLD, &status);
    char* output = "output.txt";
    for(i=0; i<size; i++) {
        if(rank == i) {
            fpa = fopen ( output, "w");
            fseek(fpa, filesize/size*rank, SEEK_SET);
            fwrite ( &data[0], sizeof(double), filesize/(size*sizeof(double)), fpa);
            fclose (fpa);
        } 
    printf("received\n");
    }  
  }

  MPI_Finalize();
  return 0;
}