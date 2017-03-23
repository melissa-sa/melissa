import subprocess

def call_bash(s):
    proc = subprocess.Popen(s, stdout=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()
    return out[:-1]
