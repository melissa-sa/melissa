import subprocess

def call_bash(s):
    proc = subprocess.Popen(s, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, , universal_newlines=True)
    (out, err) = proc.communicate()
    return {'out':str(out[:len(out)-int(out[-1]=="\n")]), 'err':str(err[:len(out)-int(out[-1]=="\n")])}
#    return str(out[:len(out)-int(out[-1]=="\n")])
