# Table of Contents

* [overview](#overview)
* [melissa_api.c](#melissa_api.c)
* [melissa_api.h]
* [melissa_api_no_mpi.h]
* [melissa_api.f]
* [melissa_api.f90]
* [melissa_api.py]
* [melissa_api_no_mpi.py]

# Overview

This file is about _what_ the code does, _why_, and to know _how_ read the comments in the code.

`Melissa_API` contains the logic to connect the simulations to the server. It defines a NxM data redistribution patern. All the data transfers between the simulations and the server rely on ZeroMQ. The rank 0 of the simulation first sends a request message to the process 0 of the server. This one replies with the informations to enable the simulation to send data to all the server processes. All the messages to send the actual data are asyncronous.

# `melissa_api.c`

## overview

`melissa_api.c` contains all the code that manage the data redistribution from the simulation to the server. It contains the code for the tree API functions:

* `melissa_init`
* `melissa_send`
* `melissa_finalize`

## `global_data` and `field_data`

These two identifiers reference static structures that store some persistent data between the calls to the different API functions.
`field_data` is a linked list and comes with `get_field_data`, `get_last_field`, and `free_field_data` functions. `field_data` returns a pointer to the `field_data_t` corresponding to the name `field_name`.


## `my_free` and `print_zmq_error`

These functions are zmq utilities.

## `gatherv_init`

This function sets the `gatherv_rcvcnt` and `gatherv_displs` attributes of a field from a list of local vector size for the `mpi_gatherv` function.

## `comm_1_to_m_init` and `comm_n_to_m_init`

These functions staticaly define a N*M redistribution between the simulation and the server. The same distribution is computed on the server side.
It sets some variables of a `field_data_t` structure.

## `melissa_init_internal`

`melissa_init_internal` is the core melissa_init function. It contacts the server, allocate the persistent structures, compute the data redistribution patern and the internal communications. It must be called once for each field in the simulation, through one of the different wrappers.  It also sets the communication chanels between members of a Sobol' group. To understand what appens internaly, read the code and the comments.

## `melissa_init_f`

This Fortran wrapper converts a Fortran MPI communicator to a C MPI communicator and calls `melissa_init_internal`.

This function is hidden from the user by the interface files `melissa_api.f90` and `melissa_api.f`


## `melissa_finalize`

This function closes the connections and releases all memory. It can also wait for the permission to disconect if Melissa is compiled with `-DCHECK_DECONNEXION=ON`.


# `melissa_api.h`

The header file for the Melissa API for C with MPI


# `melissa_api.f`

A specially formatted Fortran interface file for the Melissa API for Fortran


# `melissa_api.f90`

An unformatted Fortran interface file for the Melissa API for Fortran

# `melissa_api.py`

The Python wrapper for the Melissa API with MPI

This package requires `mpi4py`.
