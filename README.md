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
* Workflow is progressed to the PreRun stage, where the container starts

## Making a Container Image

To start, you must have a working container image that includes your application. This image and
your application are used in the NNF Container Profile to instruct the container workflow to run
your application. Adding an NNF Container Profile and container image may require elevated
privileges. Please work with your system administrator to get these on your system.

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
* nslookup

The easiest way to do this is to use the NNF MFU (MPI File Utils) image in your Dockerfile:

```dockerfile
FROM ghcr.io/nearnodeflash/nnf-mfu:master
```

Using this image ensures that your image contains the necessary software to run MPI applications
across Kubernetes pods that are running on NNF nodes.

## Writing an NNF Container Profile

Once you have a working container image, it's time to create an NNF Container Profile. The profile
is used to define the storages that you expect to use with your application. It also defines how you
run your application.

### Storages

In this example, we are expecting to have 1 non-optional storage called `DW_JOB_my_storage`. If the
storage is persistent storage, then it must start with `DW_PERSISTENT` rather than `DW_JOB`. Filesystem types are not defined here, but later in the DW directive.

```yaml
---
apiVersion: nnf.cray.hpe.com/v1alpha2
kind: NnfContainerProfile
metadata:
  name: demo
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
apiVersion: nnf.cray.hpe.com/v1alpha2
kind: NnfContainerProfile
metadata:
  name: demo
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
                image: nnf-container-example:master
                command:
                  - mpirun
                  - mpi_hello_world
                  - "$(DW_JOB_my_storage)"
      Worker:
        template:
          spec:
            containers:
              - name: nnf-container-example
                image: nnf-container-example:master
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

For a full understanding of the other options in an NNF Container Profile, see the nnf-sos
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
#DW jobdw name=demo-gfs2 type=gfs2 capacity=50GB
```

Then, define the container. Note the `DW_JOB_my_storage=demo-gfs2` argument matches what is in the NNF Container Profile and maps it to the name of the GFS2 filesystem created in the DW Directive above.

```none
#DW container name=demo-container profile=demo DW_JOB_my_storage=demo-gfs2
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

For container directives, **compute nodes** must be assigned to the workflow. The NNF software will trace
the compute nodes back to their local NNF nodes and the container will be executed on those NNF
nodes. The act of assigning compute nodes to your container workflow instructs the NNF software to
select the NNF nodes that run the containers.

For the `jobdw` directive that is included, we must define the servers (i.e. NNF nodes) and the computes.

Update the `servers` and `computes` resources assigned to the workflow.

**Note**: you must change
the node names in the `allocation-*.yaml` files to match your system. `allocationCount` must match
the number of compute nodes being targeted for that particular NNF node. In the example,
`rabbit-node-1` is attached to `compute-node-1` and `compute-node-2`, etc.

```shell
kubectl patch --type merge --patch-file=allocation-servers.yaml servers demo-container-0
kubectl patch --type merge --patch-file=allocation-computes.yaml computes demo-container
```

### Progress Workflow

At this point, the workflow should be in `Proposal` state and `Completed` status:

```shell
$ kubectl get workflows
NAME                    STATE      READY   STATUS      AGE
demo-container   Proposal   true    Completed   12m
```

Progress the workflow to the `Setup` state and wait for `Completed`:

```shell
kubectl patch --type merge workflow demo-container --patch '{"spec": {"desiredState": "Setup"}}'
```

```shell
kubectl get workflows
NAME                    STATE   READY   STATUS      AGE
demo-container   Setup   true    Completed   12m
```

Progress to `DataIn`:

```shell
kubectl patch --type merge workflow demo-container --patch '{"spec": {"desiredState": "DataIn"}}'
```

Then `PreRun`:

```shell
kubectl patch --type merge workflow demo-container --patch '{"spec": {"desiredState": "PreRun"}}'
```

The `PreRun` state will start the containers. Once the containers have started successfully, the
status will become `Completed`. Your application is now running via the launcher pod, which is instructing
`mpirun` to run your application on the worker pods. When the compute nodes were assigned to the
workflow in the `Proposal` state (via the `computes` resource), the NNF software traced the compute
nodes to their local NNF nodes. In this case, it means two NNF nodes were selected, and a worker pod
is running on each of them.

```shell
$ kubectl get pods
NAME                                   READY   STATUS    RESTARTS   AGE
demo-container-launcher-wcvcs   1/1     Running   0          5s
demo-container-worker-0         1/1     Running   0          5s
demo-container-worker-1         1/1     Running   0          5s
```

You can use kubectl to inspect the log to get your application's output:

```shell
$ kubectl logs demo-container-launcher-wcvcs
Defaulted container "nnf-container-example" out of: nnf-container-example, mpi-wait-for-worker-0 (init), mpi-wait-for-worker-1 (init), mpi-init-passwd (init)
Warning: Permanently added 'demo-container-worker-1.demo-container-worker.default.svc,10.244.1.6' (ECDSA) to the list of known hosts.
Warning: Permanently added 'demo-container-worker-0.demo-container-worker.default.svc,10.244.3.5' (ECDSA) to the list of known hosts.
Hello world from processor demo-container-worker-1, rank 1 out of 2 processors. NNF Storage path: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0, hostname: demo-container-worker-1
rank 1: test file: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0/0/testfile
rank 1: wrote file to '/mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0/0/testfile'
Hello world from processor demo-container-worker-0, rank 0 out of 2 processors. NNF Storage path: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0, hostname: demo-container-worker-0
rank 0: test file: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0/0/testfile
rank 0: wrote file to '/mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0/0/testfile'
```

You can see that the Storage path is passed into the container application and printed to the log:

```none
Hello world from processor demo-container-worker-1, rank 1 out of 2 processors. NNF Storage path: /mnt/nnf/100db033-c9f2-4cf8-b085-505aebf571c1-0, hostname: demo-container-worker-1
```

The next state is `PostRun`. When containers have exited cleanly, the status state will become `Completed`.

```shell
kubectl patch --type merge workflow demo-container --patch '{"spec": {"desiredState": "PostRun"}}'
```

```shell
$ kubectl get workflows
NAME                    STATE     READY   STATUS      AGE
demo-container   PostRun   true    Completed   13m
```

```shell
kubectl get pods
NAME                                   READY   STATUS      RESTARTS   AGE
demo-container-launcher-wcvcs   0/1     Completed   0          73s
demo-container-worker-0         1/1     Running     0          73s
demo-container-worker-1         1/1     Running     0          73s
```

You can then tear down the workflow:

```shell
kubectl patch --type merge workflow demo-container --patch '{"spec": {"desiredState": "Teardown"}}'
```

Once completed, the workflow and profile can be deleted. Again, you may need admin privileges to
remove the NNF Container Profile.

```shell
kubectl delete -f nnf-container-example.yaml
```

## Additional Info

### Communicating with Compute Node Applications

Compute node applications will have the ability to communicate with container applications using
ports. The container application can listen in on a port that is assigned to the NNF container. The
port number is made available to the compute node application via environment variables.

Port assignment is not yet implemented to the containers running on the NNF nodes.
