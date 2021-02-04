#!/usr/bin/python3

# Copyright (c) 2020, 2021, Institut National de Recherche en Informatique et en Automatique (Inria)
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
import sys
import tempfile
import unittest

from .simulation import Job, MultiSimuGroup, Server
from .. import config
from ..scheduler.job import State
from ..scheduler import dummy


def make_restore_current_working_directory_fn(fn):
    cwd = os.getcwd()

    def call_fn(*args, **kwargs):
        ret = fn(*args, **kwargs)
        os.chdir(cwd)
        return ret

    return call_fn


def make_launch_server_fn(scheduler, options):
    def launch_server(server):
        assert isinstance(server.cmd_opt, list)

        executable = os.path.join(server.path, "melissa-server")
        cmd = [executable] + server.cmd_opt
        server.job_id = scheduler.submit_job( \
            cmd, name="melissa-server", options=options
        )

    return make_restore_current_working_directory_fn(launch_server)


def make_launch_group_fn(scheduler, simulation_path, options, study_options,
                         with_simulation_setup):
    assert isinstance(study_options, dict)

    def launch_group(group):
        cwd = os.getcwd()
        working_directory = "group{:d}".format(group.group_id)
        working_directory_path = os.path.join(cwd, working_directory)

        if not os.path.isdir(working_directory_path):
            os.mkdir(working_directory_path)

        os.chdir(working_directory_path)

        # Sobol group: In the case of Sobol' indices computation, all the
        # simulations of a Sobol' group must be in the same job. In that case,
        # simu_id is a list of the simulation IDs inside the Sobol' group (the
        # ones you will pass to melissa_init), and rank is the ID of the group.
        # param_set is a list of size group.nb_param + 2 of numpy arrays of
        # size group.nb_param, corresponding to the sets of n parameters of the
        # n+2 simulations in the Sobol' group.
        if group.ml_stats["sobol_indices"] \
            and study_options["coupling"] == "MELISSA_COUPLING_FLOWVR":
            raise NotImplementedError("launch_group with FlowVR coupling")

        if not group.ml_stats["sobol_indices"]:
            assert "sampling_size" in study_options
            assert "batch_size" in study_options

            sampling_size = study_options["sampling_size"]
            batch_size = study_options["batch_size"]
            assert sampling_size % batch_size == 0

        env = {
            "MELISSA_COUPLING": "{:d}".format(group.coupling),
            "MELISSA_SERVER_NODE_NAME": group.server_node_name
        }

        # OpenMPI and Slurm set up the environment automatically and the
        # dictionaries passed to them contain only additional environment
        # variables in contrast to the subprocess module which expects the user
        # to set all or none of the environment variables manually.
        subprocess_env = os.environ.copy()
        subprocess_env.update(env)

        commands = []
        for i, sid in enumerate(group.simu_id):
            cmd_no_params = [ \
                "env",
                "MELISSA_SIMU_ID={:d}".format(sid),
                simulation_path
            ]
            params = [str(p) for p in group.param_set[i]]

            if with_simulation_setup:
                cmd = cmd_no_params + ["execute"] + params
            else:
                cmd = cmd_no_params + params

            commands.append(cmd)

            try:
                if with_simulation_setup:
                    setup_args = cmd_no_params + ["initialize"] + params
                    setup_timeout_sec = 10
                    s = subprocess.run(setup_args,
                                       env=subprocess_env,
                                       stdin=subprocess.DEVNULL,
                                       timeout=setup_timeout_sec,
                                       check=True)
            except subprocess.TimeoutExpired as e:
                fmt = "simulation ID {:d} initialization timed out (time-out={:d}s)"
                raise RuntimeError(fmt.format(sid, setup_timeout_sec)) from e
            except subprocess.CalledProcessError as e:
                fmt = "simulation ID {:d} initialization had non-zero exit code"
                raise RuntimeError(fmt.format(sid)) from e

        job_name = "melissa-group-{:d}".format(group.group_id)
        group.job_id = scheduler.submit_heterogeneous_job( \
            commands, environment=env, name=job_name, options=options
        )

    return make_restore_current_working_directory_fn(launch_group)


def make_check_job_fn(scheduler):
    def check_job(entity):
        job = entity.job_id
        scheduler.update_jobs([job])

        if job.state() == State.WAITING:
            return 0
        if job.state() == State.RUNNING:
            return 1
        if job.state() == State.TERMINATED:
            return 2
        if job.state() == State.FAILED:
            return 2
        if job.state() == State.ERROR:
            return 2

        assert False
        raise NotImplementedError("cannot handle job state {:s}".format(
            job.state))

    return check_job


def make_kill_job_fn(scheduler):
    def kill_job(entity):
        job = entity.job_id
        scheduler.cancel_jobs([job])

    return kill_job


class Test_make_launch_group_fn(unittest.TestCase):
    def setUp(self):
        self.cwd = os.getcwd()
        # preserving the temporary directory:
        # * the TemporaryDirectory instance always cleans up the tmpdir
        # * unittest always calls the tearDown method of this class; this
        #   method calls `TemporaryDirectory.cleanup()` to suppress warnings
        self.tmpdir = tempfile.TemporaryDirectory(
            prefix="melissa.launcher.test.")
        os.chdir(self.tmpdir.name)

    def tearDown(self):
        os.chdir(self.cwd)
        self.tmpdir.cleanup()

    # This method tests if the setup script is run by providing a dummy setup
    # script to `make_kill_job_fn` that creates certain files.
    def test_simple(self):
        scheduler = dummy.DummyScheduler()
        simulation_path = os.path.realpath("melissa-launcher-jm-test.sh")
        options = None
        study_options = {"coupling": "MELISSA_COUPLING_MPI"}
        launch_group = make_launch_group_fn(scheduler,
                                            simulation_path,
                                            options,
                                            study_options,
                                            with_simulation_setup=True)

        simulation_shell_code = """#!/bin/sh
set -e
set -u
filename="setup-$MELISSA_SIMU_ID.txt"
# filename should not exist at this point
if [ "$1" = 'initialize' ]; then
    if [ -f "$filename" ]; then
        >/dev/stderr echo "file $filename already exists"
        exit 1
    fi
    echo $@ >"$filename"
elif [ "$1" = 'execute' ]; then
else
    >/dev/stderr echo "unknown stage '$1'"
fi
"""
        with open(simulation_path, mode="x") as f:
            f.write(simulation_shell_code)

            os.chmod(f.fileno(), mode=0o755)

        os._exit(0)

        param_sets = [[1], [2], [3]]
        gid = 19
        group = MultiSimuGroup(param_sets)
        group.group_id = gid
        group.ml_stats["sobol_indices"] = True
        launch_group(group)

        for simu_id, _ in enumerate(param_sets):
            setup_file_fmt = os.path.join(os.getcwd(), "group{:d}",
                                          "setup-{:d}.txt")
            setup_file = setup_file_fmt.format(gid, simu_id)

            self.assertTrue(os.path.isfile(setup_file))

        self.assertEqual(group.job_id.state(), State.RUNNING)
        scheduler.cancel_jobs([group.job_id])
        self.assertEqual(group.job_id.state(), State.FAILED)


if __name__ == "__main__":
    unittest.main()
