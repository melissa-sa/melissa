#!/bin/bash

# Copyright (c) 2020, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


set -e
set -u

raw_melissa_sa_source_dir="${1:?path to Melissa SA source directory missing}"
shift

cwd="$(pwd -P)"
num_jobs="$(getconf _NPROCESSORS_ONLN)"

canonicalize_path() {
	readlink -v -f "${1:?}"
}

# This function calls CMake 3.x on CentOS 7 when CMake 3.x was installed from
# the EPEL repository; the executables are called `cmake3` and `ctest3`. By
# default, CentOS 7 ships with CMake 2.8.12.
run() {
	local command="${1:?}"
	shift

	if [ "$command" != cmake -a "$command" != ctest ]; then
		>/dev/stderr echo "unknown CMake command '$command'"
		exit 1
	fi

	if $(which cmake3 >/dev/null 2>/dev/null); then
		"${command}3" $@
	else
		"$command" $@
	fi
}

melissa_sa_source_dir="$(canonicalize_path "$raw_melissa_sa_source_dir")"
melissa_sa_binary_dir="$cwd/build.melissa-sa"
melissa_sa_prefix_dir="$cwd/prefix.melissa-sa"


mkdir -- "$melissa_sa_binary_dir"
cd -- "$melissa_sa_binary_dir"
run cmake \
	-DCMAKE_INSTALL_PREFIX="$melissa_sa_prefix_dir" \
	$@ \
	-- "$melissa_sa_source_dir"
run cmake --build . -- --jobs="$num_jobs"
run ctest --output-on-failure --timeout 300
run cmake --build . --target install


find_and_link_source_dir="$cwd/find-and-link"
find_and_link_binary_dir="$cwd/build.find-and-link"

mkdir --parents -- "$find_and_link_source_dir"
cat >"$find_and_link_source_dir/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.2.3)
project(find-and-link-test-melissa-sa VERSION 0.7.0 LANGUAGES C CXX)
find_package(Melissa CONFIG REQUIRED)
add_executable(main main.c)
target_link_libraries(main melissa)
# do not add an executable or a shared library for C++ to avoid having to link
# against a library providing the C++ MPI bindings
add_library(main-cpp STATIC main.cpp)
target_link_libraries(main-cpp melissa)
EOF

cat >"$find_and_link_source_dir/main.c" <<EOF
#include <melissa/api.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    melissa_init("foo", 1, MPI_COMM_WORLD);
    melissa_send("foo", NULL);
    melissa_finalize();
    MPI_Finalize();
}
EOF

cat >"$find_and_link_source_dir/main.cpp" <<EOF
#include <melissa/api.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    melissa_init("foo", 1, MPI_COMM_WORLD);
    melissa_send("foo", NULL);
    melissa_finalize();
    MPI_Finalize();
}
EOF

mkdir -- "$find_and_link_binary_dir"
cd -- "$find_and_link_binary_dir"
run cmake \
	-DCMAKE_PREFIX_PATH="$melissa_sa_prefix_dir" \
	-- "$find_and_link_source_dir"
run cmake --build .
