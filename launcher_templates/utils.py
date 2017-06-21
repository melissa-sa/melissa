
"""
    usefull module
"""

import subprocess

def call_bash(string):
    """
        Launches subprocess and returns out and err messages
    """
    proc = subprocess.Popen(string,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            shell=True,
                            universal_newlines=True)
    (out, err) = proc.communicate()
    return{'out':remove_end_of_line(out),
           'err':remove_end_of_line(err)}

def remove_end_of_line(string):
    """
        remove "\n" at end of a string
    """
    if len(string) > 0:
        return str(string[:len(string)-int(string[-1] == "\n")])
    else:
        return ""
