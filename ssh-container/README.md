# SSH Container for Rabbit User Container Workflows

This container provides SSH access to launcher containers running on Rabbit nodes.

## Overview

The SSH container enables remote access to Rabbit User Container workflows. It requires predetermined UID/GID and
generated SSH keys baked into the image for secure authentication. This means users will need to build their own image
and publish it somewhere. 

⚠️ **Security Note**: Images are user-specific and the generated keys must be kept secure - loss of keys means loss of
access to the container.

## Building and Publishing Your Image

In this example, I use ghcr.io to publish the image publicly. Change this to an internal registry. This is using docker, but podman can be used.

```bash
# 1. Generate SSH keys
make keys

# 2. Build with your UID/GID  
make build UID=1060 GID=100 # you can also change this in the Makefile

# 3. Test locally
make run
make connect

# 4. Publish to your registry
docker tag ssh-container:latest ghcr.io/$USERNAME/ssh-container:latest
docker push ghcr.io/$USERNAME/ssh-container:latest
```

The keys will be created and kept in `ssh_client_keys/`. The private key `mpiuser_key` is required to SSH into the
container — **keep this file safe and secure; if you lose it, you will lose access to the container.**

Only the private key (`mpiuser_key`) is needed on the compute node where you SSH from. The corresponding public key is
already included in the container image.

## NNF Container Profile

Edit the `NnfContainerProfile` to point to your image. An example is provided in this repo in `ssh-container.yaml`. Have
an admin create a profile referencing your published image:

```shell
kubectl apply -f ssh-container.yaml
```
## Run 

Once you have an image published and available to you via an `NnfContainerProfile`, you can use a flux allocation to
start the containers and then jump to a compute node. As noted previously, you will need your private ssh key to get in.

Start the allocation and use a `container` directive:

```shell
flux alloc -N4 --setattr=dw="#DW container name=ssh-container profile=ssh-container"
```

Then, ssh into the container using the private key:

```shell
[blake@rabbit-compute-2 ~]$ ssh -i ./mpiuser_key -p $NNF_CONTAINER_PORTS mpiuser@$NNF_CONTAINER_LAUNCHER

The programs included with the Debian GNU/Linux system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Debian GNU/Linux comes with ABSOLUTELY NO WARRANTY, to the extent
permitted by applicable law.

$ hostname
fluxjob-321103304048246784-launcher
$ whoami
mpiuser$
```

Once inside the container, you can then run mpirun commands:

```
$ mpirun hostname
fluxjob-321103304048246784-worker-0
fluxjob-321103304048246784-worker-1
```

In this case, we have 2 workers on 2 rabbit nodes.

## Customization

Add tools by modifying the Dockerfile:

```dockerfile
# Add your tools after the base setup
RUN apt-get update && apt-get install -y \
    your-tool \
    && rm -rf /var/lib/apt/lists/*
```