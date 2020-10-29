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
set -o pipefail
set -u

raw_melissa_sa_source_dir="${1:?path to Melissa SA source directory missing}"
build_type="${2:-Debug}"

cwd="$(pwd -P)"
num_jobs="$(getconf _NPROCESSORS_ONLN)"

canonicalize_path() {
	readlink -v -f "${1:?}"
}

melissa_sa_source_dir="$(canonicalize_path "$raw_melissa_sa_source_dir")"
melissa_sa_binary_dir="$cwd/build.melissa-sa"
melissa_sa_prefix_dir="$cwd/prefix.melissa-sa"


mkdir -- "$melissa_sa_binary_dir"
cd -- "$melissa_sa_binary_dir"
cmake \
	-DCMAKE_BUILD_TYPE="$build_type" \
	-DCMAKE_INSTALL_PREFIX="$melissa_sa_prefix_dir" \
	-- "$melissa_sa_source_dir"
cmake --build . -- --jobs="$num_jobs"
cmake --build . --target install

cd -- "$melissa_sa_prefix_dir/share/melissa/examples/heat_example"
mkdir build
cd -- build
cmake -DCMAKE_BUILD_TYPE="$build_type" -- ../solver
cmake --build . -- --jobs="$num_jobs"
cmake --build . --target install

cd -- "$melissa_sa_binary_dir"
source "$melissa_sa_prefix_dir/bin/melissa_set_env.sh"
ctest --output-on-failure --timeout 300