# Table of Contents

* [overview](#overview)

# Overview

This file is about _what_ the code does, _why_, and to know _how_ read the comments in the code.

The Melissa server is a parallel and independant component that gathers the data from the simulations. The data can arrive in any order, and at any time, from any simulation. It starts before the first simulation, and stops after the end of the last simulation. The server computes the iteratives statistics in parallel from the in-transit data. The MPI processes of the server are mostly independent and not synchronized except at the beginning and at the end of the study. The Melissa server checkpoints regularly and communicates with the launcher about the status of the simulations. All the available iterative statistics are implemented in the `server/stats` folder. The server keeps the iterative statistics in memory until the end of the study.

# `server.cxx`

## Overview

The Melissa server can be decomposed in tree parts:
* `melissa_server_init`
* `melissa_server_run`
* `melissa_server_finalize`

## `melissa_server_t`

This data structure carries information between the calls to the server functions. It contains everything the server to be managed at run-time.


## `melissa_server_init`

This function is called once when the server starts. First it allocates the server handle that will contain every variable that the server needs to run and initialize some variables. This data structure will be passed by reference to the other functions in this README.

*TODO* 2020-08-12 Is the `connexion_responder` still present in the code?

Then the server creates five ZeroMQ sockets:
1. `connexion_responder`: This is a req/rep communication channel with the individual simulations
2. `data_puller`: A pull port for receiving simulation data
3. `text_puller`: A one-way communication channel receiving from the launcher
4. `text_pusher`: A one-way communication channel send to the launcher
5. `text_requester`: This is a req/rep communication channel with the launcher

`melissa_server_init` initializes MPI, gets the options given by the launcher through the command line and opens all communication channels. Each MPI process sends its node name to the launcher and initialize all the fields and fault-tolerance structures.


## `melissa_server_run`

This is the main loop of the Melissa server. In this loop, the Melissa server checks the heartbeats of the simulations and the launcher. The launcher sends a regular heartbeat but for the simulations, the Melissa server uses data messages. To select an appropriate time-out, it is necessary to have a good estimate of the duration of one simulation timestep. The Melissa server checkpoints whenever the last checkpoint is older than the amount of time given in `check_interval`.

In its main event loop, the Melissa server waits for messages on the first three ZeroMQ sockets. ~~If no message is detected after 100 ms, then the loop cycle. Else, melissa first checks the launcher messages, then the simulation connexion messages, and finaly the data messages.~~

Only the MPI rank 0 process can receive simulation connection messages. It then replies with the server information. Before the first connection is accepted, all the other MPI ranks are blocked in `mpi_bcast`. When the rank 0 process receives its first message, it enters `mpi_bcast` and unblocks the other processes. At this point, the Melissa server creates the right number of field data structures but does not allocate memory for the iterative statistics as it does not know the size of the fields yet.

When the Melissa server receives a data message, it gets the corresponding field data structure and allocate it as the first message from this field. Then it updates the simulation vector to keep track of which simulations sent which timesteps. It it is the first data message and the server is in a `restart` state, it will try to read the checkpointed statistics from the checkpoint files. If the message corresponds to a timestep and a simulation that has already be computed, then the Melissa server drops the message and continues. Otherwise it updates all the statistics required by the user on that field.

After that, the server counts the number of finished simulations and loops until all the simulations sent all their messages.


## `melissa_server_finalize`

This function closes all sockets and frees the allocated memory.
