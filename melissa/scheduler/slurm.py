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
import re
import subprocess

from .. import config
from .job import Job, State
from .options import Options
from .scheduler import Scheduler


class SlurmJob(Job):
    def __init__(self, job_id):
        super(SlurmJob, self).__init__()
        self.job_id_ = job_id
        self.state_ = State.WAITING

    def id(self):
        return self.job_id_

    def state(self):
        return self.state_

    def __repr__(self):
        r = "SlurmJob(id={:d},state={:s})".format(self.id(), str(self.state()))
        return r

    def set_state(self, new_state):
        assert isinstance(new_state, State)

        self.state_ = new_state



class Slurm(Scheduler):
    def sanity_check_impl(self, options):
        args = options.raw_arguments
        errors = []

        for a in args:
            if a[0] != "-":
                errors.append("non-option argument `{:s}` detected".format(a))
            elif a in ["-n", "--ntasks", "--test-only"]:
                errors.append("remove `{:s}` argument".format(a))

        command = ["srun", "--test-only", "--ntasks=1"] + args + ["--", "true"]
        srun = subprocess.run( \
            command,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )

        if srun.returncode != 0:
            e = "srun error on trial execution: {:s}".format(srun.stderr)
            errors.append(e)

        return errors


    def submit_heterogeneous_job_impl( \
        self, commands, environment, name, options
    ):
        sbatch_env = os.environ.copy()
        sbatch_env.update(environment)

        num_commands = len(commands)
        num_tasks = options.num_processes * num_commands

        sbatch_args = ["--ntasks={:d}".format(num_tasks)]
        sbatch_commands = []
        for i, cmd in enumerate(commands):
            executable = os.path.basename(cmd[0])

            if " " in executable:
                e = "Scheduler cannot handle whitespace in executable name '{:s}'"
                raise RuntimeError(e.format(executable))

            srun_cmd = [ \
                    "srun",
                    "--ntasks={:d}".format(options.num_processes),
                    "--output={:s}.%J.stdout".format(executable),
                    "--error={:s}.%J.stderr".format(executable),
                    "--"
                ] \
                + cmd

            sbatch_cmd = \
                options.raw_arguments \
                + ["--wrap={:s}".format(" ".join(srun_cmd))] \
                + ([":"] if i+1 < len(commands) else [])

            sbatch_commands = sbatch_commands + sbatch_cmd


        sbatch_call = ["sbatch"] + sbatch_args + sbatch_commands
        print(sbatch_call)

        sbatch = subprocess.run(
            sbatch_call,
            env=sbatch_env,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            check=True
        )

        stderr = sbatch.stderr

        if stderr != "":
            print("sbatch: {:s}".format(stderr), file=sys.stderr)

        job_id_regexp = "Submitted batch job (\d+)"
        match = re.compile(job_id_regexp, re.ASCII).search(sbatch.stdout)

        if match is None:
            e = "no job ID found in sbatch output: '{:s}'"
            raise ValueError(e.format(sbatch.stdout))

        return SlurmJob(int(match.group(1)))


    def cancel_jobs_impl(self, jobs):
        job_list = ["--job={:d}".format(j.id()) for j in jobs]
        scancel_command = ["scancel", "--batch", "--quiet"] + job_list
        scancel = subprocess.run(
            scancel_command,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            check=True
        )


    def update_jobs_impl(self, jobs):
        job_list = ["--job={:d}".format(j.id()) for j in jobs]
        sacct_command = \
            ["sacct", "--parsable", "--format=jobid,state"] \
            + job_list
        sacct = subprocess.run(
            sacct_command,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            check=True
        )

        line_regexp = "\A(\d+)[|](\w+)[|]"
        pattern = re.compile(line_regexp)
        job_map = dict([(j.id(), j) for j in jobs])
        slurm_states_waiting = [ \
            "PENDING", "REQUEUED", "SUSPENDED", "RESIZING"
        ]
        slurm_states_failure = [ \
            "BOOT_FAIL", "CANCELLED", "DEADLINE", "FAILED", "NODE_FAIL",
            "OUT_OF_MEMORY", "PREEMPTED", "TIMEOUT"
        ]

        for line in sacct.stdout:
            match = pattern.fullmatch(line[:-1])

            if match is None:
                continue

            job_id = int(match.group(1))
            slurm_state = match.group(2)

            assert job_id in job_map

            j = job_map[job_id]

            if slurm_state == "REVOKED":
                e = "cannot handle Slurm siblings like job {:d}"
                raise NotImplementedError(e.format(job_id))

            if slurm_state in slurm_states_waiting:
                j.set_state(State.WAITING)
            elif slurm_state in ["RUNNING"]:
                j.set_state(State.RUNNING)
            elif slurm_state in ["COMPLETED"]:
                j.set_state(State.TERMINATED)
            elif slurm_state in slurm_states_failure:
                j.set_state(State.FAILED)
            else:
                e = "unknown Slurm job state '{:s}'"
                raise RuntimeError(e.format(slurm_state))
