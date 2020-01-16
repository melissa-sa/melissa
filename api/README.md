# Table of content
* [overview](#overview)
* [melissa_api.c](#melissa_api.c)
* [melissa_api.h]
* [melissa_api_no_mpi.h]
* [melissa_api.f]
* [melissa_api.f90]
* [melissa_api.py]
* [melissa_api_no_mpi.py]

# overview

This file is about "what" the code does and "why", to know "how" read the comments in the code.

Melissa_API contains the logic to connect the simulations to the server. It defines a NxM data redistribution patern. All the data transfert between the simulations and the server relie on ZeroMQ.
The rank 0 of the simulation first send a request message to the process 0 of the server. This one replies with the informations to enable the simulation to send data to all the server processes.
All the messages to send the actual data are asyncronous.

# melissa_api.c

## overview

melissa_api.c contains all the code that manage the data redistribution from the simulation to the server. It contains the code for the tree API functions:

* melissa_init
* melissa_send
* melissa_finalize

## global_data and field_data
are static structures that store some persistent data between the calls to the different API functions.
*field_data is a chained list, and comes with get_field_data, get_last_field and free_field_data functions. field_data returns a pointer to the field_data_t corresponding to the name field_name.


## my_free and print_zmq_error
are zmq utilities.

## gatherv_init
sets the gatherv_rcvcnt and gatherv_displs attributes of a field from a list of local vect size for the mpi_gatherv function.

## comm_1_to_m_init and comm_n_to_m_init
staticaly define a N*M redistribution between the simulation and the server. the same distribution is computed on the server side.
It sets some variables of a field_data_t structure

## melissa_init_internal
the core melissa_init function.
It contacts the server, allocate the persistent structures, compute the data redistribution patern and the internal communications.
It must be called once for each field in the simulation, through one of the different wrappers.
It also sets the communication chanels between members of a Sobol' group.
To understand what appens internaly, read the code and the comments.

## melissa_init_f
This fortran wrapper converts a Fortran MPI communicator to a C mpi communicatior, and calls melissa_init_internal.
This function is hidden from the user by the melissa_api.f90 and the melissa_api.f interface files.

## melissa_init_no_mpi
This no MPI wrapper provides a function without MPI, it call melissa_init_internal with fake MPI informations.

## melissa_init_no_mpi_f
The same than melissa_init_no_mpi but for fortran. We need this function to pass the arguments by reference.
This function is hidden from the user by the melissa_api.f90 and the melissa_api.f interface files.

## melissa_finalize
This function closes the connexions and release the memory. It can also wait for the permission to disconect it Melissa is compiled with -DCHECK_DECONNEXION=ON

# melissa_api.h
The header file for C + MPI melissa API

# melissa_api_no_mpi.h
The header file for C melissa API

# melissa_api.f
Formated fortran interface for all the melissa API functions

# melissa_api.f90
Unformated fortran interface for all the melissa API functions

# melissa_api.py
Python + MPI wrapper for melissa API (requires mpi4py)

# melissa_api_no_mpi.py
Python wrapper for melissa API
