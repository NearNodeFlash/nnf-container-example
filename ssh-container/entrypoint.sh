#!/bin/sh 

echo "Starting SSH container entrypoint script..."

whoami
id

ls -la /home/mpiuser
ls -la /home/mpiuser/ssh-session

export -p > /home/mpiuser/ssh-session/env.sh
echo "[ -f /home/mpiuser/ssh-session/env.sh ] && . /home/mpiuser/ssh-session/env.sh" >> "$HOME"/.profile

echo "Port $NNF_CONTAINER_PORTS" >> /home/mpiuser/ssh-session/sshd_config && \

/usr/sbin/sshd -D -f "/home/mpiuser/ssh-session/sshd_config" -E "/home/mpiuser/ssh-session/sshd.log"