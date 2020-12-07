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

from .. import config
from ..scheduler.job import State


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

        executable = os.path.join(server.path, "melissa_server")
        cmd = [executable] + server.cmd_opt
        server.job_id = scheduler.submit_job( \
            cmd, name="melissa-server", options=options
        )

    return make_restore_current_working_directory_fn(launch_server)


def make_launch_group_fn(scheduler, simulation_path, options, study_options):
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

        commands = []
        for i, sid in enumerate(group.simu_id):
            cmd = [ \
                "env",
                "MELISSA_SIMU_ID={:d}".format(sid),
                simulation_path,
                *[str(p) for p in group.param_set[i]]
            ]

            commands.append(cmd)

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
        raise NotImplementedError("cannot handle job state {:s}".format(job.state))

    return check_job


def make_kill_job_fn(scheduler):
    def kill_job(entity):
        job = entity.job_id
        scheduler.cancel_jobs([job])

    return kill_job
