#!/usr/bin/python3

# Copyright (c) 2020, Institut National de Recherche en Informatique et en Automatique (Inria)
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

import os
import subprocess

from .. import config
from .job import Job, State
from .scheduler import Scheduler


class OpenMpiJob(Job):
    def __init__(self, process):
        assert isinstance(process, subprocess.Popen)

        super(OpenMpiJob, self).__init__()
        self.process_ = process
        self.state_ = State.RUNNING

    def id(self):
        return self.process_.pid

    def state(self):
        return self.state_

    def __repr__(self):
        r = "OpenMpiJob(id={:d},state={:s})".format(self.id(), str(self.state_))
        return r

    def set_state(self, new_state):
        assert isinstance(new_state, State)

        self.state_ = new_state



class OpenMpiScheduler(Scheduler):
    def __init__(self):
        super(OpenMpiScheduler, self).__init__()

        try:
            mpirun = subprocess.run( \
                ["mpirun", "-x", "ABC=1", "-n", "1", "true"],
                check=True
            )
        except subprocess.CalledProcessError as e:
            raise RuntimeError("error in mpirun test call") from e


    def submit_heterogeneous_job_impl( \
        self, commands, environment, name, options
    ):
        assert isinstance(environment, dict)
        assert isinstance(commands, list)
        assert name is None or isinstance(name, str)
        assert options is None or isinstance(options, list)

        # Approach to environment variables:
        # Follow OpenMPI mpirun man page advice, that is,
        # * set `VARIABLE=VALUE` in mpirun environment,
        # * pass `-x VARIABLE` on the mpirun command line.

        ompi_env = os.environ.copy()
        env_args = []
        for key in sorted(environment.keys()):
            assert isinstance(key, str)

            ompi_env[key] = environment[key]
            env_args += ["-x", key]

        if options is None:
            ompi_options = ["-n", "1"]
        else:
            ompi_options = options

        ompi_commands = []
        for i, cmd in enumerate(commands):
            ompi_cmd = \
                ompi_options \
                + env_args \
                + ["--"] \
                + ["python3", os.path.join(config.bindir, "redirect-io")] \
                + ["stdbuf", "--output=L"] \
                + cmd \
                + ([":"] if i+1 < len(commands) else [])

            ompi_commands = ompi_commands + ompi_cmd


        ompi_call = [ "mpirun" ] + ompi_commands
        print(ompi_call)

        mpirun = subprocess.Popen(
            ompi_call,
            env=ompi_env,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )

        return OpenMpiJob(mpirun)


    def cancel_jobs_impl(self, jobs):
        for j in jobs:
            if j.process_.poll() is None:
                j.process_.terminate()

        timeout_sec = 5

        for j in jobs:
            try:
                j.process_.wait(timeout_sec)
            except TimeoutExpired:
                j.process_.kill()

            j.state_ = State.TERMINATED


    def update_jobs_impl(self, jobs):
        for j in jobs:
            returncode = j.process_.poll()
            if returncode is None:
                state = State.RUNNING
            elif returncode == 0:
                state = State.TERMINATED
            else:
                state = State.FAILED

            j.set_state(state)


if __name__ == "__main__":
    import time

    s = OpenMpiScheduler()
    job = s.submit_heterogeneous_job([["echo", "foo"], ["echo", "bar"]])

    while job.state() in [State.WAITING, State.RUNNING]:
        time.sleep(1)
        s.update_jobs([job])

    if job.state() == State.FAILED:
        out, err = job.process_.communicate()
        print("out", out)
        print("err", err)
