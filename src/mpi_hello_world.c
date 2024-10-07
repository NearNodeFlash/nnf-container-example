// Author: Wes Kendall
// Copyright 2011 www.mpitutorial.com
// This code is provided freely with the tutorials on mpitutorial.com. Feel
// free to modify it for your own use. Any distribution of the code must
// either provide a link to www.mpitutorial.com or keep this header intact.
//
// An intro MPI hello world program that uses MPI_Init, MPI_Comm_size,
// MPI_Comm_rank, MPI_Finalize, and MPI_Get_processor_name.
//
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mpi.h>

int main(int argc, char **argv)
{
  char nnf_storage_path[PATH_MAX];
  char *nnf_node_name = NULL;

  if (getenv("DEAN1") != NULL) {
    printf("HEY, found DEAN1 env var with (%s)!\n", getenv("DEAN1"));
  }

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

  if (argc < 2)
  {
    printf("Storage parameter not supplied\n");
    return -1;
  }
  strncpy(nnf_storage_path, argv[1], PATH_MAX);

  // Print off a hello world message
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  printf("Hello world from processor %s, rank %d out of %d processors. NNF Storage path: %s, hostname: %s\n",
         processor_name, world_rank, world_size, nnf_storage_path, hostname);

  // We're using a GFS2 filesystem, which has index mounts for every compute node
  // e.g. /mnt/nnf/5d335081-cd0f-4b8a-a1f4-94860a8ae702-0/0/
  nnf_node_name = getenv("NNF_NODE_NAME");
  int stop = 0;
  if (nnf_node_name == NULL) {
    printf("NNF_NODE_NAME env value not found\n");
    stop = 1;
  }
  if (argc < 3) {
    printf("Node name parameter not supplied\n");
    stop = 1;
  }
  if (stop > 0) {
    return -1;
  }
  if (sprintf(nnf_storage_path, "%s/%s-0/testfile", nnf_storage_path, nnf_node_name) == -1)
  {
    fprintf(stderr, "rank %d: %s\n", world_rank, strerror(errno));
    return errno;
  }

  printf("rank %d: test file: %s\n", world_rank, nnf_storage_path);

  int fd = open(nnf_storage_path, O_WRONLY | O_CREAT);
  if (fd == -1)
  {
    fprintf(stderr, "rank %d: error opening file: %s\n", world_rank, strerror(errno));
    return errno;
  }

  int res = posix_fallocate(fd, 0, 100);
  if (res == -1)
  {
    fprintf(stderr, "rank %d: error allocating file: %s\n", world_rank, strerror(errno));
    return errno;
  }

  printf("rank %d: wrote file to '%s'\n", world_rank, nnf_storage_path);

  // Finalize the MPI environment. No more MPI calls can be made after this
  MPI_Finalize();
}
