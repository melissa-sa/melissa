// Copyright (c) 2020, Institut National de Recherche en Informatique et en Automatique (Inria)
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

#include <melissa/api.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>


int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);

	void* p_appnum = NULL;
	int info = 0;
	MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_APPNUM, &p_appnum, &info);

	assert(info != 0);

	int appnum = -1;
	memcpy(&appnum, p_appnum, sizeof(appnum));

	assert(appnum >= 0);

	MPI_Comm app;
	MPI_Comm_split(MPI_COMM_WORLD, appnum, 0, &app);

	const char* simu_id_str = getenv("MELISSA_SIMU_ID");

    if(!simu_id_str) {
        exit(EXIT_FAILURE);
    }

    int sid = atoi(simu_id_str);
	const unsigned num_iterations = 2;
	const char field_name[] = "heat1";
	const size_t vector_size = 2;
	double data[2] = { 0, NAN };

	melissa_init(field_name, vector_size, app);

	for(unsigned it = 0; it < num_iterations; ++it) {
		data[1] = 1.0 * it * sid;

		melissa_send(field_name, data);
		assert(data[0] == 0);
		assert(data[1] == 1.0 * it * sid);
	}

	melissa_finalize();
	MPI_Finalize();
}
