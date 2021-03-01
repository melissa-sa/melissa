#!/usr/bin/python3

import os
import re
import subprocess
from typing import Dict, List

from .job import Job, State
from .scheduler import Scheduler
from .options import Options


class OarJob(Job):
    def __init__(self, job_id: int):
        assert isinstance(job_id, int)

        super(OarJob, self).__init__()
        self.id_ = job_id
        self.state_ = State.WAITING

    def id(self) -> int:
        return self.id_

    def state(self) -> State:
        return self.state_

    def __repr__(self) -> str:
        return "OarJob(id={:d},state={:s})".format(self.id_, str(self.state_))

    def set_state(self, new_state: State) -> None:
        assert isinstance(new_state, State)

        self.state_ = new_state


class OarScheduler(Scheduler):
    def __init__(self, mpi_provider: str):
        assert isinstance(mpi_provider, str)

        if mpi_provider not in ["openmpi"]:
            raise ValueError(
                "unknown MPI implementation '{:s}'".format(mpi_provider))

        super(OarScheduler, self).__init__()
        self.mpi_provider = mpi_provider

    def submit_heterogeneous_job_impl(self, commands: list,
                                      environment: Dict[str, str], name: str,
                                      options: Options) -> OarJob:
        assert isinstance(commands, list)
        assert isinstance(environment, dict)
        assert name is None or isinstance(name, str)

        # OAR filters the environment. All environment variables must be passed
        # inside the script:
        # * call `env` to set all environment variables
        # * make MPI pass the environment to the client code, e.g., with
        #   OpenMPI `-x` option
        for key in ["LD_LIBRARY_PATH", "PATH"]:
            if key not in environment and key in os.environ:
                environment[key] = os.environ[key]

        make_env_arg = lambda key: "'{:s}={:s}'".format(key, environment[key])
        env_args = [make_env_arg(key) for key in sorted(environment.keys())]
        mpirun_env_args = []

        if self.mpi_provider == "openmpi":
            machinefile_arg = ["-machinefile", "$OAR_NODE_FILE"]

            for key in sorted(environment.keys()):
                mpirun_env_args.extend(["-x", key])
        else:
            fmt = "OAR scheduler implementation for MPI provider '{:s}' missing"
            raise NotImplementedError(fmt.format(self.mpi_provider))

        # assemble mpirun arguments
        num_procs = options.num_processes
        mpirun_args = []

        for cmd in commands:
            args = ["-n", str(num_procs)] + mpirun_env_args + ["--"] + cmd
            args_str = " ".join(args)
            mpirun_args.append(args_str)

        mpirun_command = "mpirun" + " " + " ".join(
            machinefile_arg) + " " + " : ".join(mpirun_args)

        # avoid irritating `env executable arg1 arg2`
        # DEBUG
        oar_script = mpirun_command
        if env_args == []:
            oar_script = mpirun_command
        else:
            oar_script = "env " + " ".join(env_args) + " " + mpirun_command

        maybe_job_name = ["--name", name] if name else []
        total_num_procs = num_procs * len(commands)

        oar_command = \
            ["oarsub",
             "--directory={:s}".format(os.getcwd()),
             "--resource=core={:d}".format(total_num_procs)
            ] \
            + maybe_job_name \
            + options.raw_arguments \
            + ["--", oar_script]

        oarsub = subprocess.run(oar_command,
                                stdin=subprocess.DEVNULL,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True,
                                check=True)

        job_id_regexp = 'OAR_JOB_ID=(\d+)'
        m = re.compile(job_id_regexp).search(oarsub.stdout)

        if m is None:
            raise ValueError("no job id found in oarsub output: '{:s}'".format(
                oarsub.stdout))

        return OarJob(int(m.group(1)))

    def update_jobs_impl(self, jobs: List[OarJob]) -> None:
        assert isinstance(jobs, list)

        if jobs == []:
            return

        job_args = []
        for job in jobs:
            job_args += ["--job", "{:d}".format(job.id())]

        command = ["oarstat", "--state"] + job_args
        oarstat = subprocess.run( \
            command,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            universal_newlines=True,
            check=True
        )

        oarstat_output = re.compile("([0-9]+): ([A-Za-z]+)")
        jobs_map = {job.id(): job for job in jobs}
        states_map = { \
            "Error": State.ERROR,
            "Finishing": State.TERMINATED,
            "Hold": State.WAITING,
            "Launching": State.WAITING,
            "Running": State.RUNNING,
            "Terminated": State.TERMINATED,
            "toAckReservation": State.WAITING,
            "toLaunch": State.WAITING,
            "Waiting": State.WAITING
        }

        for line in oarstat.stdout.split("\n"):
            if line == "":
                continue

            m = oarstat_output.fullmatch(line)

            if m is None:
                fmt = "cannot parse oarstat ouput '{:s}'"
                raise RuntimeError(fmt.format(line))

            job_id = int(m.group(1))
            oar_state = m.group(2)

            if oar_state in states_map:
                jobs_map[job_id].set_state(states_map[oar_state])
            else:
                fmt = "unknown OAR job state '{:s}'"
                raise RuntimeError(fmt.format(oar_state))

    def cancel_jobs_impl(self, jobs: List[OarJob]) -> None:
        assert isinstance(jobs, list)

        jobs_str = ["{:d}".format(job.id()) for job in jobs]
        command = ["oardel"] + jobs_str

        try:
            subprocess.run( \
                command, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, universal_newlines=True, check=True
            )
        except subprocess.CalledProcessError as e:
            job_already_killed_code = 6
            if e.returncode != job_already_killed_code:
                raise e
