###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENSE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################


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
