#!/usr/bin/python3

# Copyright (c) 2021, Institut National de Recherche en Informatique et en Automatique (Inria)
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

import unittest

from .. import config
from .job import Job, State
from .options import Options
from .scheduler import Scheduler


class DummyJob(Job):
    def __init__(self, jid):
        super(DummyJob, self).__init__()
        self.id_ = jid
        self.state_ = State.RUNNING

    def id(self):
        return self.id_

    def state(self):
        return self.state_

    def __repr__(self):
        r = "DummyJob(id={:d},state={:s})".format(self.id(), str(self.state()))
        return r

    def set_state(self, new_state):
        assert isinstance(new_state, State)

        self.state_ = new_state


class DummyScheduler(Scheduler):
    """
    This class implements a scheduler that never runs the submitted jobs.
    """
    def __init__(self):
        super(DummyScheduler, self).__init__()
        self.counter_ = 0

    def submit_heterogeneous_job_impl( \
        self, commands, environment, name, options
    ):
        job_id = self.counter_
        self.counter_ += 1

        return DummyJob(job_id)

    def cancel_jobs_impl(self, jobs):
        for j in jobs:
            j.set_state(State.FAILED)

    def update_jobs_impl(self, jobs):
        pass


class TestDummyScheduler(unittest.TestCase):
    def test_simple(self):
        scheduler = DummyScheduler()
        job = scheduler.submit_job(["echo", "Hello, World!"])

        self.assertEqual(job.state(), State.RUNNING)
        scheduler.update_jobs([job])
        self.assertEqual(job.state(), State.RUNNING)
        scheduler.cancel_jobs([job])
        self.assertEqual(job.state(), State.FAILED)


if __name__ == "__main__":
    unittest.main()
