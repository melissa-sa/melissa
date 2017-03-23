import os
import time
import sys
import numpy as np
import re
from call_bash import *

def check_job(batch_scheduler, username, job_id):
    state = "pending"
    if (batch_scheduler == "OAR"):
        if (not job_id in call_bash("oarstat -u --sql \"state = 'Pending'\"")):
            state = "running"
            if (not job_id in call_bash("oarstat -u --sql \"state = 'Running'\"")):
                state = "terminated"
    elif (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
        if (not "PENDING" in call_bash("squeue --job="+job_id+" -l")):
            state = "running"
            if (not "RUNNING" in call_bash("squeue --job="+job_id+" -l")):
                state = "terminated"
    return state
