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

import enum
import unittest


@enum.unique
class State(enum.Enum):
    # There job failed for reasons unrelated to program execution.
    # The program may have never run.
    ERROR = 1
    WAITING = 2
    RUNNING = 3
    # The job ran and the program terminated successfully.
    TERMINATED = 4
    # The job ran but the program terminated unsuccessfully.
    FAILED = 5

    def __str__(self) -> str:
        return self.name



class Job:
    def id(self):
        raise NotImplementedError("Job.id not implemented")

    def state(self):
        raise NotImplementedError("Job.state not implemented")

    def __repr__(self):
        raise NotImplementedError("Job.__repr__ not implemented")



class TestState(unittest.TestCase):
    def test_simple(self):
        self.assertEqual(State.ERROR, State.ERROR)
        self.assertNotEqual(State.ERROR, State.FAILED)

        self.assertEqual(str(State.WAITING), "WAITING")


if __name__ == "__main__":
    unittest.main()
