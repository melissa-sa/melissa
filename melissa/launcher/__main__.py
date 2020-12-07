#!/usr/bin/python3

# Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
#               2017, EDF (https://www.edf.fr/)
#               2020, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
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


import argparse
import importlib.util
import logging
import os
import shutil
import sys

from ..scheduler.options import Options as SchedulerOptions
from ..scheduler.oar import OarScheduler
from ..scheduler.openmpi import OpenMpiScheduler
from ..scheduler.slurm import Slurm
from . import job_management as jm
from .study import Study


def main():
    cwd = os.getcwd()

    parser = argparse.ArgumentParser(prog="launcher", description="Melissa SA Launcher")
    parser.add_argument( \
        "scheduler",
        choices=["oar", "openmpi", "slurm"],
        default="openmpi"
    )
    parser.add_argument("options")
    parser.add_argument("simulation")
    parser.add_argument("--scheduler-arg", action="append", default=[])
    parser.add_argument("--scheduler-arg-client", action="append", default=[])
    parser.add_argument("--scheduler-arg-server", action="append", default=[])
    parser.add_argument("--num-client-processes", type=int, default=1)
    parser.add_argument("--num-server-processes", type=int, default=1)

    args = parser.parse_args()

    if args.scheduler == "oar":
        scheduler = OarScheduler()
    elif args.scheduler == "openmpi":
        scheduler = OpenMpiScheduler()
    elif args.scheduler == "slurm":
        scheduler = Slurm()
    else:
        assert False
        sys.exit("BUG: unknown scheduler '{:s}'".format(args.scheduler))

    options_file = os.path.realpath(args.options)
    client_options = SchedulerOptions( \
        args.num_client_processes, \
        args.scheduler_arg + args.scheduler_arg_client
    )
    server_options = SchedulerOptions( \
        args.num_server_processes,
        args.scheduler_arg + args.scheduler_arg_server
    )

    # check scheduler options
    def print_scheduler_argument_error(errors):
        for e in errors:
            fmt = "error in server options: {:s}"
            print(fmt.format(e), file=sys.stderr)

        if errors != []:
            sys.exit(1)

    print_scheduler_argument_error(scheduler.sanity_check(server_options))
    print_scheduler_argument_error(scheduler.sanity_check(client_options))

    # check if the simulation executable exists
    # do not try to figure out if it is executable for the launcher
    if shutil.which(args.simulation) is None:
        return "simulation executable '{:s}' not found".format(args.simulation)

    # try to open the options file for reading because the importlib module
    # returns only `None` on error
    try:
        with open(options_file, "r") as f:
            pass
    except Exception as e:
        return str(e)

    options_spec = \
        importlib.util.spec_from_file_location("options", options_file)
    options = importlib.util.module_from_spec(options_spec)
    sys.modules["options"] = options
    options_spec.loader.exec_module(options)

    from options import STUDY_OPTIONS as stdy_opt
    from options import MELISSA_STATS as ml_stats
    from options import USER_FUNCTIONS as usr_func

    usr_func["launch_group"] = jm.make_launch_group_fn( \
        scheduler, args.simulation, client_options, stdy_opt
    )
    launch_server = \
        jm.make_launch_server_fn(scheduler, server_options)
    check_job = jm.make_check_job_fn(scheduler)
    check_load = lambda: True
    kill_job = jm.make_kill_job_fn(scheduler)

    usr_func['launch_server'] = launch_server
    usr_func['check_server_job'] = check_job
    usr_func['check_group_job'] = check_job
    usr_func['restart_server'] = launch_server
    usr_func['check_scheduler_load'] = check_load
    usr_func['cancel_job'] = kill_job


    # init log for launcher
    # TODO should align the verbosity option (1,2,...) with the logging level (à, 10, 20 ....)

    melissa_study = Study(stdy_opt, ml_stats, usr_func)
    try:
        melissa_study.run()
    except Exception as e:
        print(e)
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())
