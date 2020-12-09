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
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);

	if(argc < 2)
	{
		const char* executable = (argc > 0) ? argv[0] : "a.out";
		fprintf(stderr, "usage: %s <seed>\n", executable);
		return EXIT_FAILURE;
	}


	unsigned short prng_state[3] = { 0 };
	// parse command line
	{
		char* first = argv[1];
		char* last = NULL;
		int base = 0;
		// use a signed value because strtoull does not return errors when
		// parsing negative values
		long long raw_seed = strtoll(first, &last, base);

		assert(last);

		if(first == last || *last != '\0') {
			fprintf(
				stderr,
				"cannot parse seed '%s' as integer\n", first
			);
			exit(EXIT_FAILURE);
		}

		const int64_t SEED_MAX = (INT64_C(1) << 48) - 1;

		if(raw_seed < 0 || raw_seed > SEED_MAX) {
			fprintf(
				stderr,
				"expected seed in interval [0, 2**48), got %s\n",
				first
			);
			exit(EXIT_FAILURE);
		}

		uint64_t seed = raw_seed;
		memcpy(&prng_state, &seed, sizeof(prng_state));
	}

	const char field_name[] = "heat1";
	const size_t vector_size = 2;
	double data[2] = { 0, NAN };

	melissa_init(field_name, vector_size, MPI_COMM_WORLD);

	const unsigned num_iterations = 2;
	for(unsigned it = 0; it < num_iterations; ++it) {
		data[1] = erand48(prng_state) - 0.5;

		melissa_send(field_name, data);
		assert(data[0] == 0);
	}

	melissa_finalize();
	MPI_Finalize();
}
