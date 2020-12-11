#!/usr/bin/python3

import re
import subprocess

from .job import Job, State
from .scheduler import Scheduler


class OarJob(Job):
    def __init__(self, job_id):
        assert isinstance(job_id, int)

        super(OarJob, self).__init__()
        self.id_ = job_id
        self.state_ = State.WAITING

    def id(self):
        return self.id_

    def state(self):
        return self.state_

    def __repr__(self):
        return "OarJob(id={:d},state={:s})".format(self.id_, str(self.state_))


    def set_state(self, new_state):
        assert isinstance(new_state, State)

        self.state_ = new_state



class OarScheduler(Scheduler):
    def __init__(self, user_args):
        assert isinstance(user_args, list)

        super(OarScheduler, self).__init__()
        self.user_args = user_args


    def launch_job_impl(self, command, environment, name):
        # assume values are strings because the network protocol usually
        # requires special formatting, e.g., hexadecimal values with padding
        to_string = lambda key: "'{:s}={:s}'".format(key, environment[key])
        env_args = [to_string(k) for k in environment]
        command_str = " ".join(command)

        # avoid irritating `env executable arg1 arg2`
        if env_args == []:
            command_with_env = command_str
        else:
            command_with_env = "env " + " ".join(env_args) + " " + command_str

        maybe_job_name = [] if name is None else ["--name", name]

        oar_command = \
            ["oarsub", "--resource", "cpu=1"] \
            + maybe_job_name \
            + self.user_args \
            + ["--", command_with_env]

        oarsub = subprocess.run(
            oar_command,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            check=True
        )

        job_id_regexp = 'OAR_JOB_ID=(\d+)'
        m = re.compile(job_id_regexp).search(oarsub.stdout)

        if m is None:
            raise ValueError("no job id found in oarsub output: '{:s}'".format(oarsub.stdout))

        return OarJob(int(m.group(1)))


    def update_job_status(self, jobs):
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

        oarstat_output = re.compile("([0-9]+): ([A-Z][a-z]+)")
        jobs_map = { job.id(): job for job in jobs }
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


    def cancel_jobs(self, jobs):
        assert isinstance(jobs, list)

        jobs_str = ["{:d}".format(job.id) for job in jobs]
        command = ["oardel"] + jobs_str
        subprocess.run( \
            command, stderr=subprocess.PIPE, universal_newlines=True, check=True
        )
