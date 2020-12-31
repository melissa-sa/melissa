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

#include <melissa/server/options.h>
#include <melissa/server/server.h>

#include <string.h>
#include <stdlib.h>


#define enforce(test) \
	do { \
		if(!(test)) { \
			fprintf( \
				stderr, \
				"%s:%u: condition %s is false\n", \
				__FILE__, __LINE__, #test \
			); \
			abort(); \
		} \
	} while(0)



int call_get_fields(
	const char* argument, melissa_server_t* server
) {
	// function argument is const so that callers can use `call_this("input")`
	char* argument_copy = strdup(argument);
	int ret = melissa_options_get_fields(argument_copy, server);
	free(argument_copy);
	return ret;
}


int main() {
	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		int ret = call_get_fields("alpha", &server);

		enforce(ret == 0);
		enforce(server.fields);
		enforce(server.melissa_options.nb_fields == 1);
		enforce(strcmp(server.fields[0].name, "alpha") == 0);

		free(server.fields);
	}

	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		int ret = call_get_fields("beta:gamma", &server);

		enforce(ret == 0);
		enforce(server.fields);
		enforce(server.melissa_options.nb_fields == 2);
		enforce(strcmp(server.fields[0].name, "beta") == 0);
		enforce(strcmp(server.fields[1].name, "gamma") == 0);

		free(server.fields);
	}

	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		int ret = call_get_fields(":invalid:argument", &server);

		enforce(ret < 0);
		enforce(!server.fields);
	}

	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		int ret = call_get_fields("also:invalid:", &server);

		enforce(ret < 0);
		enforce(!server.fields);
	}

	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		int ret = call_get_fields("consecutive::colons", &server);

		enforce(ret < 0);
		enforce(!server.fields);
		enforce(server.melissa_options.nb_fields == 0);
	}

	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		int ret = call_get_fields("delta::", &server);

		enforce(ret < 0);
		enforce(!server.fields);
	}

	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		char too_long_name[2 * MAX_FIELD_NAME_LEN] = {0};
		memset(too_long_name, 'A', sizeof(too_long_name) - 1);

		int ret = melissa_options_get_fields(too_long_name, &server);

		enforce(ret < 0);
		enforce(!server.fields);
	}

	{
		melissa_server_t server;
		memset(&server, 0, sizeof(server));

		char buffer[2 * MAX_FIELD_NAME_LEN] = {0};
		memset(buffer, 'B', sizeof(buffer) - 1);
		// do not use strcpy because of terminating null byte
		memcpy(buffer, "firstName:", strlen("firstName:"));

		int ret = melissa_options_get_fields(buffer, &server);

		enforce(ret < 0);
		enforce(!server.fields);
	}
}
