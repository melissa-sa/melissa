# Table of content
* [overview](#overview)

# overview

This file is about "what" the code does and "why", to know "how" read the comments in the code.

Melissa_server is a parallel and independant component that gathers the data from the simulations. The data can arrive in any order, and at any time, from any simulation.
It starts before the first simulation, and stops after the end of the last simulation.
Melissa_server computes the iteratives statistics in parallel from the in-transit data. The MPI processes of the server are mostly independant and not syncronized, except at the begining and at the end of the study.
It checkpoints regularly, and communicates with the launcher about the status of the simulations.
All the available iterative statistics are implemented in the stats folder.
Melissa_server keeps the iteratives statistics in memory until the end of the study.

# server.cxx

## overview

Melissa_server can be decomposed in tree parts:
- melissa_server_init
- melissa_server_run
- melissa_server_finalize

## melissa_server_t

melissa_server_t is a data structure used to carry informations between the calls to the server functions. It contains everything the server needs all along its run time.

## melissa_server_init

This function is called once when the server starts.
It firts allocate the server_handle that will contain every variable that the server needs to run, and initialize some variables.
This data structure will be passed as a pointer to the other functions.

Then the server creates 5 or 6 zetroMQ sockets.
The first one is the connexion_responder, and is a req/rep comunication chanel with the simulations.
The optional deconnexion_responder is an other req/rep comunication chanel with the simulations.
 It should be enabled in the cmake command by -DCHECK_SIMU_DECONNECTION. This option enable the simulation to ask the server befor deconecting.
The data_puller is a pull port for receiving simulation data.
The text_puller and text_puller are one way comunication chanels to and from the launcher.
The text_requester is a req/rep comunication chanel with the launcher.

Melissa_server inits MPI and get the options given by the launcher through the comand line, open the comunication ports.

Each MPI process sends its node names to the launcher, and initialize all the fields and fault tolerance structures.

## melissa_server_run
This is the main loop of Melissa_server.
In this loop, melissa server checks the heartbeats of the simulations and the launcher.
The launcher sends a regular heartbeat, but for the simulations, Melissa Server uses the data messages. That's why one have to estimate the diration of a simulation timestep to define the right timeout.
Melissa Server then checkpoints if it didn't do it for the last check_interval time interval.

Then, it enters the poll on the messages ports.

Melissa server try to get messages on the three or four ports.
If no message is detected after 100 ms, then the loop cycle.
Else, melissa first checks the launcher messages, then the simulation connexion messages, and finaly the data messages.

Only rank 0 can receive simulation connexion messages. It then replies with the server informations.
Before the firs connexion, all the other ranks are blocked in the mpi_bcast. When the rank 0 recieve its first message, it enters the mpi_bcast and unlock the other processes.
At this point, Melissa Server creates the right number of field data structures, but doesn't allocate the memory for the statistics, as it doesn't know the size of the fields yet.

When melissa server receive a data message, it get the corresponding field data structure and allocate it it is the first message from this field.
Then it updates the simulation vector to keep track of which simulations sent which timesteps. It it is the first data message and the server is in a "restart" state, it will try to read the checkpointed statistics from the checkpoint files.
If the message corresponds to a timestep and a simulation that has already be computed, then Melissa Server drops it and continue.
otherwhise, it updates all the statistics required by the user on that field.

After that, the server counts the number of finished simulations and cycle the main loop until all the simulations sent all their messages.


## melissa_server_finalize

Release all the ports and deallocate memory.
