# TODO: Set the UID and GID To match the user who is going to use this container. Due to security, the UID and GID
# *must* be baked in to the image in order for sshd to obtain the correct permissions to enable ssh security.
UID ?= 1060
GID ?= 100

# NOTE: SSH_PORT is only for local testing purposes. In Rabbit User Containers, the port is assigned dynamically via the
# NNF_CONTAINER_PORTS environment variable.
SSH_PORT ?= 5123

# SSH key files - these are generated locally and used in the container. The mpiuser_key is used for ssh authentication
# to connect to the container. Do not lose the client keys.
SSH_HOST_KEY_DIR = ./ssh_host_keys
SSH_HOST_RSA_KEY = $(SSH_HOST_KEY_DIR)/ssh_host_rsa_key
SSH_CLIENT_KEY_DIR = ./ssh_client_keys
SSH_CLIENT_KEY = $(SSH_CLIENT_KEY_DIR)/mpiuser_key

# Image name and tag
IMAGE_NAME ?= ssh-container
TAG ?= latest

# CONTAINER_TOOL defines the container tool to be used for building images.
# Be aware that the target commands are only tested with Docker which is
# scaffolded by default. However, you might want to replace it to use other
# tools. (i.e. podman)
CONTAINER_TOOL ?= docker

# Container name for easier management
CONTAINER_NAME = ssh-container-$(UID)-$(GID)

.PHONY: all docker-build clean ssh-keys client-keys keys run stop connect status help

all: docker-build

help: ## Show this help message
	@echo "Available targets:"
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  %-15s %s\n", $$1, $$2}' $(MAKEFILE_LIST)

# Generate SSH host keys
ssh-keys: ## Generate SSH host keys for the container
	@echo "Generating SSH host keys..."
	@mkdir -p $(SSH_HOST_KEY_DIR)
	@if [ ! -f $(SSH_HOST_RSA_KEY) ]; then \
		ssh-keygen -t rsa -b 4096 -f $(SSH_HOST_RSA_KEY) -N '' -C "SSH host RSA key"; \
	fi
	@echo "SSH host keys generated successfully"

# Generate SSH client keys for authentication
client-keys: ## Generate SSH client keys for authentication
	@echo "Generating SSH client keys..."
	@mkdir -p $(SSH_CLIENT_KEY_DIR)
	@if [ ! -f $(SSH_CLIENT_KEY) ]; then \
		ssh-keygen -t rsa -b 4096 -f $(SSH_CLIENT_KEY) -N '' -C "mpiuser@ssh-container"; \
		echo ""; \
		echo "Client key generated: $(SSH_CLIENT_KEY)"; \
		echo "Public key: $(SSH_CLIENT_KEY).pub"; \
		echo ""; \
		echo "To connect without password:"; \
		echo "  ssh -i $(SSH_CLIENT_KEY) -p $(SSH_PORT) mpiuser@localhost"; \
		echo ""; \
	fi

# Generate both SSH host and client keys
keys: ssh-keys client-keys ## Generate both SSH host and client keys
	@echo "SSH keys generated successfully"
	@echo "Host keys in: $(SSH_HOST_KEY_DIR)"
	@echo "Client keys in: $(SSH_CLIENT_KEY_DIR)"

# Build the container image
docker-build:
	@echo "Building $(CONTAINER_TOOL) image with UID=$(UID), GID=$(GID)..."
	$(CONTAINER_TOOL) build \
		--platform linux/amd64 \
		--build-arg USER_UID=$(UID) \
		--build-arg USER_GID=$(GID) \
		-t $(IMAGE_NAME):$(TAG) \
		-t $(IMAGE_NAME):uid$(UID)-gid$(GID) \
		.
	@echo "$(CONTAINER_TOOL) image built successfully"
	@echo "Image tags:"
	@echo "  - $(IMAGE_NAME):$(TAG)"
	@echo "  - $(IMAGE_NAME):uid$(UID)-gid$(GID)"
	@echo ""
	@echo "To inspect UID/GID/PORT labels:"
	@echo "  $(CONTAINER_TOOL) inspect $(IMAGE_NAME):$(TAG) | grep -A 10 Labels"

# Run the container
run: docker-build ## Run the container with SSH service (as UID 1060 and GID 100)
	$(CONTAINER_TOOL) run -d \
		--platform linux/amd64 \
		--name $(CONTAINER_NAME) \
		--user $(UID):$(GID) \
		-p $(SSH_PORT):$(SSH_PORT) \
		-e NNF_CONTAINER_PORTS=$(SSH_PORT) \
		$(IMAGE_NAME):$(TAG)
	@echo "Container started with SSH on port $(SSH_PORT) (accessible via host port $(SSH_PORT))"
	@echo "Connect with: make connect"

# Stop and remove container
stop: ## Stop and remove the container
	-$(CONTAINER_TOOL) stop $(CONTAINER_NAME)
	-$(CONTAINER_TOOL) rm $(CONTAINER_NAME)

# Check container status
status: ## Check if the container is running
	@if $(CONTAINER_TOOL) ps | grep -q $(CONTAINER_NAME); then \
		echo "Container $(CONTAINER_NAME) is running"; \
		$(CONTAINER_TOOL) ps | grep $(CONTAINER_NAME); \
	else \
		echo "Container $(CONTAINER_NAME) is not running"; \
	fi

# Connect to the running container using SSH keys
connect: ## Connect to the container via SSH
	@if [ ! -f $(SSH_CLIENT_KEY) ]; then \
		echo "Client key not found. Run 'make client-keys' first."; \
		exit 1; \
	fi
	@if ! $(CONTAINER_TOOL) ps | grep -q $(CONTAINER_NAME); then \
		echo "Container $(CONTAINER_NAME) is not running."; \
		echo "Run 'make run' to start the container first."; \
		echo "Or run 'make status' to check container status."; \
		exit 1; \
	fi
	@echo "Connecting to SSH container..."
	ssh -i $(SSH_CLIENT_KEY) -p $(SSH_PORT) -o StrictHostKeyChecking=no mpiuser@localhost

# Clean up generated files and containers
clean: stop ## Clean up generated files and containers
	rm -rf $(SSH_HOST_KEY_DIR)
	rm -rf $(SSH_CLIENT_KEY_DIR)
	-$(CONTAINER_TOOL) rmi $(IMAGE_NAME):$(TAG)
	-$(CONTAINER_TOOL) rmi $(IMAGE_NAME):uid$(UID)-gid$(GID)