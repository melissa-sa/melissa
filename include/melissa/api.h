// Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (Inria)
//               2017, EDF
//               2021, Institut National de Recherche en Informatique et en Automatique (Inria)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MELISSA_API_H
#define MELISSA_API_H

#include <melissa/config.h>

#include <mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function initializes the connection with the Melissa Server for MPI
 * simulations.
 * This function must be called once for every scalar field to be analyzed by
 * Melissa. A scalar field is an array of double-precision floating-point
 * numbers representing the simulation state at a given time step without any
 * other metadata (in particular, without mesh).
 *
 * @remark The field names must have been declared in the launcher options file.
 * @remark
 * Each rank in the given MPI communicator must participate in the
 * Melissa communications and call melissa_init, melissa_send, and
 * melissa_finalize.
 *
 * @pre MPI_Init was called before.
 *
 * @param[in] field_name name of the field to initialize
 * @param[in] vect_size The number of values
 * @param[in] comm MPI communicator
 */
void melissa_init(const char* field_name, const int vect_size, MPI_Comm comm);

/**
 * Fortran wrapper for melissa_init that converts the MPI communicator.
 *
 * @param[in] field_name name of the field to initialize
 * @param[in] local_vect_size size of the local data vector to send to the
 * library
 * @param[in] comm_fortran Fortran MPI communicator
 */
void melissa_init_f(
    const char* field_name, int* local_vect_size, MPI_Fint* comm_fortran);

/**
 * This function sends the of the values of the given field at the current
 * time step.
 *
 * @pre melissa_init was called for the field before.
 *
 * @param[in] field_name name of the field to send to Melissa Server
 * @param[in] send_vect local data array to send to the statistic library
 */
void melissa_send(const char* field_name, const double* send_vect);


/**
 * This function sends all remaining simulaton data queued by melissa_send and
 * then disconnects from the Melissa server.
 *
 * @pre MPI_Finalize was not called yet.
 */
void melissa_finalize();

/**
 * This function returns the version of the Melissa library.
 */
const char* melissa_version();

#ifdef __cplusplus
}
#endif

#endif // MELISSA_API_H
