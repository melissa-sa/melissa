// Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (Inria)
//               2017, EDF (https://www.edf.fr/)
//               2020, Institut National de Recherche en Informatique et en Automatique (Inria)
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

#include <melissa/server/output.h>

void write_output_d(const char   *file_name,
                        const char   *statistics_name,
                        const char   *field,
                        const int     t,
                        const size_t  vec_size,
                        const double  vec[])
{
    (void) statistics_name;
    (void) field;
    (void) t;
    FILE* f;

    f = fopen(file_name, "w");
    for (size_t i=0; i<vec_size; i++)
    {
        fprintf (f, "%.*e\n", 12, vec[i]);
    }
    fclose(f);
}

void write_output_i(const char   *file_name,
                        const char   *statistics_name,
                        const char   *field,
                        const int     t,
                        const size_t  vec_size,
                        const int     vec[])
{
    (void) statistics_name;
    (void) field;
    (void) t;
    FILE* f;

    f = fopen(file_name, "w");
    for (size_t i=0; i<vec_size; i++)
    {
        fprintf (f, "%d\n", vec[i]);
    }
    fclose(f);
}
