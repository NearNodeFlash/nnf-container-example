// Author: Wes Kendall
// Copyright 2011 www.mpitutorial.com
// This code is provided freely with the tutorials on mpitutorial.com. Feel
// free to modify it for your own use. Any distribution of the code must
// either provide a link to www.mpitutorial.com or keep this header intact.
//
// An intro MPI hello world program that uses MPI_Init, MPI_Comm_size,
// MPI_Comm_rank, MPI_Finalize, and MPI_Get_processor_name.
//
#include <mpi.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

int main(int argc, char** argv) {
  char nnf_storage_path[PATH_MAX];

  // Initialize the MPI environment. The two arguments to MPI Init are not
  // currently used by MPI implementations, but are there in case future
  // implementations might need the arguments.
  MPI_Init(NULL, NULL);

  // Get the number of processes
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Get the rank of the process
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  // Get the name of the processor
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);

  if (argc < 2) {
    printf("Storage parameter not supplied\n");
    return -1;
  }
  strncpy(nnf_storage_path, argv[1], PATH_MAX); 

  // Print off a hello world message
  printf("Hello world from processor %s, rank %d out of %d processors. Storage path: %s\n",
         processor_name, world_rank, world_size, nnf_storage_path);
  // TODO pull in the storage env variables

  // Finalize the MPI environment. No more MPI calls can be made after this
  MPI_Finalize();
}