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

from .job import Job

class Scheduler:
    def submit_job(self, command, *args, **kwargs):
        return self.submit_heterogeneous_job([command], *args, **kwargs)


    def submit_heterogeneous_job( \
        self, commands, environment={}, name=None, options=None
    ):
        assert isinstance(commands, list)
        assert isinstance(environment, dict)
        assert name is None or isinstance(name, str)
        assert options is None or isinstance(options, list)

        # allow None as dictionary value so that callers can pass, for example,
        #   env = { "PYTHONPATH": os.getenv("PYTHONPATH") }
        # without having to check if the environment variable was set
        keys = [k for k in environment.keys()]
        for key in keys:
            if environment[key] is None:
                del environment[key]
            else:
                assert isinstance(key, str)
                assert isinstance(environment[key], str)

        job = self.submit_heterogeneous_job_impl( \
            commands, environment, name, options
        )

        assert isinstance(job, Job)
        return job


    def cancel_jobs(self, jobs):
        assert isinstance(jobs, list)
        for j in jobs:
            assert isinstance(j, Job)

        self.cancel_jobs_impl(jobs)


    def update_jobs(self, jobs):
        assert isinstance(jobs, list)
        for j in jobs:
            assert isinstance(j, Job)

        self.update_jobs_impl(jobs)


    def submit_heterogeneous_job_impl(self, command, environment, name):
        raise NotImplementedError("Scheduler.submit_heterogeneous_job_impl not implemented")

    def cancel_jobs_impl(self, jobs):
        raise NotImplementedError("Scheduler.cancel_jobs_impl not implemented")

    def update_jobs_impl(self, jobs):
        raise NotImplementedError("Scheduler.update_jobs_impl not implemented")
