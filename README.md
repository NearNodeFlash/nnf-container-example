# NNF Container Workflow Tutorial

## Overview

This repo contains an example MPI application that is built into a container that can be used for an
NNF Container Workflow. The application uses the dynamic storage created by the workflow. The
storage path is passed into the hello world application.

The overall process for creating a container workflow is as follows:

* A container image is created from this repo
* An NNF Container Profile is created on the NNF Kubernetes cluster:
  * The container image is specified in the profile along with the command to run
  * A required GFS2 storage is defined in the profile
  * The command also includes an environment
* A Workflow is created and contains two directives:
  * Directive for the GFS2 filesystem (that matches the GFS2 storage name in the profile)
  * Directive for the container (that specifies the profile)
* Workflow is progressed to the PreRun stage, where the container(s) starts

## Making a Container Image

To start, you must have a working container image that includes your application. This image and
your application are used in the NNF Container Profile to instruct the container workflow to run
your application.

The `Dockerfile` in this repository creates an example image that can be used to drive container
workflows.

When building your own image, ensure it meets the following requirements coupled with your user
application.

### Requirements

Any container image that is built must be available in an image registry that is available on your
cluster. See your cluster administrator for more details.

In this example, we're using the GitHut Container Registry (ghcr.io), so your cluster must have internet access to retrieve the image.

#### MPI Applications

For MPI applications, the container must include the following:

* open-mpi
* MPI File Utils
* ssh server

The easiest way to do this is to use the NNF MFU (MPI File Utils) image in your Dockerfile:

```dockerfile
FROM ghcr.io/nearnodeflash/nnf-mfu:latest
```

Using this image ensures that your image contains the necessary software to run MPI applications
across Kubernetes pods that are running on NNF nodes.

Your container image is used as the image for both MPI launchers and workers. `mpirun` uses ssh to
communicate with worker nodes, so an ssh server must be enabled:

```dockerfile
RUN service ssh start
```

#### Non-MPI Applications

There are no requirements for non-MPI applications. There is no launcher/worker model, so each
container is executed with the same command and without `mpirun` or any ssh communication.

## Writing an NNF Container Profile

Once you have a working container image, it's time to create an NNF Container Profile. The profile
is used to define the storages that you expect to use with your application. It also defines how you
run your application.

### Storages

In this example, we are expecting to have 1 non-optional storage called `DW_JOB_my_storage`. If the
storage is persistent storage, then it must start with `DW_PERSISTENT` rather than `DW_JOB`. Filesystem types are not defined here, but later in the DW directive.

```yaml
---
apiVersion: nnf.cray.hpe.com/v1alpha1
kind: NnfContainerProfile
metadata:
  name: nnf-container-example-profile
  namespace: nnf-system
data:
  storages:
    - name: DW_JOB_my_storage
      optional: false
```

The name of this storage **must** be present in the container directive - more on that later.

### Container Specs

Next, we define the container specification. For MPI applications, this is done using `mpiSpec`. The
`mpiSpec` allows us to define the Launcher and Worker containers.

```yaml
apiVersion: nnf.cray.hpe.com/v1alpha1
kind: NnfContainerProfile
metadata:
  name: nnf-container-example-profile
  namespace: nnf-system
data:
  storages:
    - name: DW_JOB_my_storage
      optional: false
  mpiSpec:
    mpiReplicaSpecs:
      Launcher:
        template:
          spec:
            containers:
              - name: nnf-container-example
                image: nnf-container-example:0.0.1
                command:
                  - mpirun
                  - mpi_hello_world
                  - "$(DW_JOB_my_storage)"
      Worker:
        template:
          spec:
            containers:
              - name: nnf-container-example
                image: nnf-container-example:0.0.1
```

Both the `Launcher` and `Worker` must be defined. The main pieces here are to set the images for both
and the command for the `Launcher`. Boiled down, these are just Kubernetes [PodTemplateSpecs](https://pkg.go.dev/k8s.io/api/core/v1#PodTemplateSpec).

Our `mpi_hello_world` application takes in a command line argument for the storage file path. We are
using the name of the storage we defined above in the `storages` object to pass into our command:

```yaml
               command:
                  - mpirun
                  - mpi_hello_world
                  - "$(DW_JOB_my_storage)"
```

For the full definition of the `MPIJobSpec` provided by `mpi-operator`, see the definition
[here](https://pkg.go.dev/github.com/lukwil/mpi-operator/pkg/apis/kubeflow/v1#MPIJobSpec).  However,
some of these values are overridden by NNF software and not all configurable options have been
tested.

For a full understanding of the other optinos in an NNF Container Profile, see the nnf-sos
[samples](https://github.com/NearNodeFlash/nnf-sos/blob/master/config/samples/nnf_v1alpha1_nnfcontainerprofile.yaml)
and
[examples](https://github.com/NearNodeFlash/nnf-sos/blob/master/config/examples/nnf_v1alpha1_nnfcontainerprofiles.yaml).

## Creating a DW Container Workflow

With a container image and an NNF Container Profile, we are now ready to create a Workflow.

The workflow definition will include two DW Directives:

1. `#DW jobdw` to create the storage defined in the profile
2. `#DW container` to create the containers and map the storage/profile.

First, we'll create the storage. We will be using GFS2 for the filesystem:

```none
#DW jobdw name=nnf-container-example-gfs2 type=gfs2 capacity=50GB
```

Then, define the container. Note the `DW_JOB_my_storage=nnf-container-example-gfs2` argument matches what is in the NNF Container Profile and maps it to the name of the GFS2 filesystem created in the DW Directive above.

```none
#DW container name=nnf-container-example profile=nnf-container-example-profile DW_JOB_my_storage=nnf-container-example-gfs2
```

## Deploy Example to Cluster

**Note**: The Flux workload manager may take care of all or most of the steps in this section. This is
the manual way of doing things. You may want to consult a Flux expert on how to drive a container
workflow using Flux.

With a working Kubernetes cluster, the previous examples can be put together and deployed on the system. The files in this repository have done that.

### Create the Profile and Workflow

Deploy the profile and create the workflow:

```shell
kubectl apply -f nnf-container-example.yaml
```

### Assign Nodes to the Workflow

For container directives, compute nodes must be assigned to the workflow. The NNF software will trace
the computes node back to their local NNF nodes and the container will be executed on those NNF
nodes.

For the `jobdw` directive that is included, we must define the servers (i.e. NNF nodes) and the computes.

Update the `servers` and `computes` resources assigned to the workflow. 

**Note**: you must change
the node names in the `allocation-*.yaml` files to match your system. `allocationCount` must match
the number of compute nodes being targeted for that particular NNF node. In the example,
`rabbit-node-1` is attached to `compute-node-1` and `compute-node-2`, etc.

```shell
kubectl patch --type merge --patch-file=allocation-servers.yaml servers nnf-container-example-0
kubectl patch --type merge --patch-file=allocation-computes.yaml computes nnf-container-example
```

### Progress Workflow

At this point, the workflow should be in `Proposal` state and ready:

```shell
$ kubectl get workflows
NAME                    STATE      READY   STATUS      AGE
nnf-container-example   Proposal   true    Completed   12m
```

Progress the workflow to the `Setup` state and wait for ready:

```shell
kubectl patch --type merge workflow nnf-container-example --patch '{"spec": {"desiredState": "Setup"}}'
```

```shell
kubectl get workflows
NAME                    STATE   READY   STATUS      AGE
nnf-container-example   Setup   true    Completed   12m
```

Progress to `DataIn`:

```shell
kubectl patch --type merge workflow nnf-container-example --patch '{"spec": {"desiredState": "DataIn"}}'
```

Then `PreRun`:

```shell
kubectl patch --type merge workflow nnf-container-example --patch '{"spec": {"desiredState": "PreRun"}}'
```

The `PreRun` state will start the containers. Once the containers have started
successfully, the state will be `Ready`. Your application is now running.

```shell
$ kubectl get pods
NAME                                   READY   STATUS    RESTARTS   AGE
nnf-container-example-launcher-wcvcs   1/1     Running   0          5s
nnf-container-example-worker-0         1/1     Running   0          5s
nnf-container-example-worker-1         1/1     Running   0          5s
```

You can use kubectl to inspect the log to get your application's output:

```shell
$ kubectl logs nnf-container-example-launcher-wcvcs
Defaulted container "nnf-container-example" out of: nnf-container-example, mpi-init-passwd (init)
Warning: Permanently added 'nnf-container-example-worker-1.nnf-container-example-worker.default.svc,10.244.1.6' (ECDSA) to the list of known hosts.
Warning: Permanently added 'nnf-container-example-worker-0.nnf-container-example-worker.default.svc,10.244.3.5' (ECDSA) to the list of known hosts.
Hello world from processor nnf-container-example-worker-1, rank 1 out of 2 processors. Storage path: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0
Hello world from processor nnf-container-example-worker-0, rank 0 out of 2 processors. Storage path: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0
```

You can see that the Storage path is passed into the container application and printed to the log:

```none
Hello world from processor nnf-container-example-worker-1, rank 1 out of 2 processors. Storage path: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0
```

The next state is `PostRun`. When containers have exited cleanly, the state will become `Ready`.
Otherwise, it will remain `Ready:false`.

```shell
kubectl patch --type merge workflow nnf-container-example --patch '{"spec": {"desiredState": "PostRun"}}'
```

```shell
$ kubectl get workflows
NAME                    STATE     READY   STATUS      AGE
nnf-container-example   PostRun   true    Completed   13m
```

```shell
kubectl get pods
NAME                                   READY   STATUS      RESTARTS   AGE
nnf-container-example-launcher-wcvcs   0/1     Completed   0          73s
nnf-container-example-worker-0         1/1     Running     0          73s
nnf-container-example-worker-1         1/1     Running     0          73s
```
