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
#include <string.h>
#include <unistd.h>

#include <mpi.h>

int main(int argc, char **argv)
{
  char nnf_storage_path[PATH_MAX];
  char nnf_storage_path2[PATH_MAX];

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

  if (argc < 3)
  {
    printf("Storage parameters not supplied\n");
    return -1;
  }
  strncpy(nnf_storage_path, argv[1], PATH_MAX);
  strncpy(nnf_storage_path2, argv[2], PATH_MAX);

  // Print off a hello world message
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  printf("Hello world from processor %s, rank %d out of %d processors. NNF Storage path: %s, hostname: %s\n",
         processor_name, world_rank, world_size, nnf_storage_path, hostname);

  // We're using a GFS2 filesystem, which has index mounts for every compute node
  // e.g. /mnt/nnf/5d335081-cd0f-4b8a-a1f4-94860a8ae702-1/0/
  if (sprintf(nnf_storage_path, "%s/0/testfile", nnf_storage_path) == -1)
  {
    fprintf(stderr, "rank %d: %s\n", world_rank, strerror(errno));
    return errno;
  }
  printf("rank %d: test file: %s\n", world_rank, nnf_storage_path);

  // Lustre does not use index mounts
  // e.g. /mnt/nnf/5d335081-cd0f-4b8a-a1f4-94860a8ae702-0/
  if (sprintf(nnf_storage_path2, "%s/testfile", nnf_storage_path2) == -1)
  {
    fprintf(stderr, "rank %d: %s\n", world_rank, strerror(errno));
    return errno;
  }
  printf("rank %d: test file2: %s\n", world_rank, nnf_storage_path2);

  // Open a testfile on each filesystem
  int fd = open(nnf_storage_path, O_WRONLY | O_CREAT);
  if (fd == -1)
  {
    fprintf(stderr, "rank %d: error opening file: %s\n", world_rank, strerror(errno));
    return errno;
  }
  int fd2 = open(nnf_storage_path2, O_WRONLY | O_CREAT);
  if (fd2 == -1)
  {
    fprintf(stderr, "rank %d: error opening file2: %s\n", world_rank, strerror(errno));
    return errno;
  }

  // Allocate some storage for each test file
  int res = posix_fallocate(fd, 0, 100);
  if (res == -1)
  {
    fprintf(stderr, "rank %d: error allocating file: %s\n", world_rank, strerror(errno));
    return errno;
  }
  int res2 = posix_fallocate(fd2, 0, 100);
  if (res2 == -1)
  {
    fprintf(stderr, "rank %d: error allocating file2: %s\n", world_rank, strerror(errno));
    return errno;
  }

  printf("rank %d: wrote file to '%s'\n", world_rank, nnf_storage_path);
  printf("rank %d: wrote file2 to '%s'\n", world_rank, nnf_storage_path2);

  // Finalize the MPI environment. No more MPI calls can be made after this
  MPI_Finalize();
}
