// Author: Wes Kendall
// Copyright 2011 www.mpitutorial.com
// This code is provided freely with the tutorials on mpitutorial.com. Feel
// free to modify it for your own use. Any distribution of the code must
// either provide a link to www.mpitutorial.com or keep this header intact.
//
// An intro MPI hello world program that uses MPI_Init, MPI_Comm_size,
// MPI_Comm_rank, MPI_Finalize, and MPI_Get_processor_name.
//
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mpi.h>

int main(int argc, char **argv, char **envp)
{
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

  // Print off a hello world message
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  printf("Hello world from processor %s, rank %d out of %d processors. NNF Storage path: %s, hostname: %s\n",
         processor_name, world_rank, world_size, nnf_storage_path, hostname);

  // We're using a GFS2 filesystem, which has index mounts for every compute node
  // e.g. /mnt/nnf/5d335081-cd0f-4b8a-a1f4-94860a8ae702-0/rabbit-node-1-0/

  DIR *dir;
  struct dirent *entry;
  char nnf_storage_path[PATH_MAX];
  char storage_dir[PATH_MAX];
  char indexed_dir[PATH_MAX];

  // Find each mounted filesystem. The directories in /mnt/nnf are the
  // storages created via the "#DW jobdw" directives in the workflow.
  // For this demo, we'll pick the first storage. A full-scale app would work
  // through each of the storages.
  dir = opendir("/mnt/nnf");
  if (dir == NULL)
  {
    perror("opendir /mnt/nnf");
    return -1;
  }
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    printf("Found mounted filesystem: %s\n", entry->d_name);
    // In production, we would verify that it actually fits.
    sprintf(storage_dir, "/mnt/nnf/%s", entry->d_name);
    break; // Just use the first one for this demo.
  }
  closedir(dir);

  // Pick one of the indexed directories in the chosen storage. The directories
  // within the chosen storage are indexed, with one for each compute that has
  // access to this storage.
  // For this demo, we'll pick the first indexed dir. A full-scale app would
  // work through each of the indexed dirs.
  dir = opendir(storage_dir);
  if (dir == NULL)
  {
    perror("opendir storage_dir");
    return -1;
  }
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    printf("Found indexed dir: %s\n", entry->d_name);
    // In production, we would verify that it actually fits.
    sprintf(indexed_dir, "%s/%s", storage_dir, entry->d_name);
    break; // Just use the first one for this demo.
  }
  closedir(dir);

  // Now write a file to the indexed dir.
  if (sprintf(nnf_storage_path, "%s/testfile", indexed_dir) == -1)
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
