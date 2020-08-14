#!/bin/bash
# Copyright 2020 Inria (https://www.inria.fr/)

set -e
set -u

# This script checks the CMake build scripts.

raw_source_dir="${1:?}"
source_dir="$(readlink --canonicalize-existing "$raw_source_dir")"
now="$(date '+%Y%m%dT%H%M%S')"
tmpdir="$(mktemp --dir --tmpdir -- melissa-cmake-test."$now".XXXXXXXX)"
root_dir="$tmpdir/root"

mkdir --parents -- "$root_dir"
cd -- "$tmpdir"

# Competing packages:
# Install different ZeroMQ versions in different paths and test if CMake ends
# up using the correct one

fake_zmq_installation()
{
	local root_dir="${1:?}"
	local version="${2:?}"

	mkdir --parents -- "$root_dir/include"
	mkdir --parents -- "$root_dir/lib/pkgconfig"
	touch -- "$root_dir/include/zmq.h"
	touch -- "$root_dir/lib/libzmq.so"

	{
		echo "prefix=$root_dir"
		echo 'exec_prefix=${prefix}'
		echo 'libdir=${exec_prefix}/lib'
		echo 'includedir=${prefix}/include'
		echo ""
		echo 'Name: libzmq'
		echo 'Description: 0MQ c++ library'
		echo "Version: $version"
		echo 'Libs: -L${libdir} -lzmq'
		echo 'Cflags: -I${includedir}'
	} >"$root_dir/lib/pkgconfig/libzmq.pc"
}


wanted_zmq_version='1234'
wanted_zmq_root="$tmpdir/zeromq_$wanted_zmq_version"

fake_zmq_installation "$wanted_zmq_root" "$wanted_zmq_version"
fake_zmq_installation "$root_dir" 666


# run CMake
mkdir -- "$tmpdir/build"
cd -- "$tmpdir/build"
cmake \
	-DCMAKE_PREFIX_PATH="$root_dir" \
	-DZeroMQ_ROOT="$wanted_zmq_root" \
	-- "$source_dir" \
	>"$tmpdir/cmake-stdout.txt"


# Check ZeroMQ-related variables
zmq_library="$(fgrep ZeroMQ_LIBRARY:FILEPATH CMakeCache.txt | awk -F= '{print $2}')"
zmq_include_dir="$(fgrep ZeroMQ_INCLUDE_DIR:PATH CMakeCache.txt | awk -F= '{print $2}')"

if [ ! "$zmq_library" = "$wanted_zmq_root/lib/libzmq.so" ]
then
	>/dev/stderr echo "CMake using wrong ZeroMQ library $zmq_library"
fi

if [ ! "$zmq_include_dir" = "$wanted_zmq_root/include" ]
then
	>/dev/stderr echo "CMake using wrong ZeroMQ include directory $zmq_include_dir"
fi
