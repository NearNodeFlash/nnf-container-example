FROM ghcr.io/nearnodeflash/nnf-dm-copy-offload:0.1.19

# Build arguments for user UID/GID
ARG USER_UID=1234
ARG USER_GID=1234

# Set environment variables
ENV USER_UID=${USER_UID}
ENV USER_GID=${USER_GID}

ENV SESSION_DIR="/home/mpiuser/ssh-session"
ENV KEYS_DIR="/home/mpiuser/ssh-session/keys"

# Modify existing mpiuser to use specified UID/GID
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN TARGET_GROUP=$(getent group ${USER_GID} | cut -d: -f1) && \
    if [ -z "$TARGET_GROUP" ]; then \
        groupmod -g ${USER_GID} mpiuser; \
        TARGET_GROUP="mpiuser"; \
    fi && \
    usermod -u ${USER_UID} -g ${USER_GID} mpiuser
SHELL ["/bin/sh", "-c"]

# Set up SSH directory for mpiuser
RUN mkdir -p /home/mpiuser/.ssh && \
    mkdir -p ${SESSION_DIR} && \
    mkdir -p ${KEYS_DIR}

# Copy SSH host keys
COPY ssh_host_keys/ssh_host_rsa_key ${KEYS_DIR}/ssh_host_rsa_key
COPY ssh_host_keys/ssh_host_rsa_key.pub ${KEYS_DIR}/ssh_host_rsa_key.pub

# Copy client public key for authorized access
COPY ssh_client_keys/mpiuser_key.pub ${KEYS_DIR}mpiuser_key.pub
RUN cat ${KEYS_DIR}mpiuser_key.pub > ${KEYS_DIR}/authorized_keys

# Override any entrypoint from base image and set our own
COPY entrypoint.sh /entrypoint.sh
RUN chmod a+x /entrypoint.sh

WORKDIR /home/mpiuser

# Configure SSH - port is set at runtime so we don't hardcode it here
RUN echo "HostKey $KEYS_DIR/ssh_host_rsa_key" > "$SESSION_DIR/sshd_config" && \
    echo "AuthorizedKeysFile $KEYS_DIR/authorized_keys" >> "$SESSION_DIR/sshd_config" && \
    echo "PermitRootLogin no" >> "$SESSION_DIR/sshd_config" && \
    echo "PasswordAuthentication no" >> "$SESSION_DIR/sshd_config"

# Set proper permissions on keys
RUN chmod 600 ${KEYS_DIR}/ssh_host_rsa_key && \
    chmod 644 ${KEYS_DIR}/ssh_host_rsa_key.pub && \
    chmod 600 ${KEYS_DIR}/authorized_keys && \
    chmod 700 /home/mpiuser/.ssh && \
    chown -R ${USER_UID}:${USER_GID} /home/mpiuser

# Labels for identifying the UID/GID
LABEL uid="${USER_UID}"
LABEL gid="${USER_GID}"
LABEL description="SSH container with mpiuser (UID:${USER_UID}, GID:${USER_GID})"

ENTRYPOINT ["/entrypoint.sh"]
