# Start from the NearNodeFlash MPI File Utils Image that is used for Data Movement. This is an easy
# way to get MPI File Utils/mpi-operator as a base.
FROM ghcr.io/nearnodeflash/nnf-mfu:latest

# MPI Containers communicate via ssh, so we need to ensure that sshd is running on each container.
RUN service ssh start

CMD ["mpirun", "-host", "localhost", "hostname"]