import subprocess

def call_bash(s):
    proc = subprocess.Popen(s, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, universal_newlines=True)
    (out, err) = proc.communicate()
    print "out = "+str(out)
    print "err = "+str(err)
    return {'out':str(out[:len(out)-int(out[-1]=="\n")]) if len(out)>0 else "", 'err':str(err[:len(err)-int(err[-1]=="\n")]) if len(err)>0 else ""}
#    return str(out[:len(out)-int(out[-1]=="\n")])
