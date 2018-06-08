#!/usr/bin/env python2

###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################

# -*- coding: utf-8 -*-

"""
    Main module of Melissa Launcher.

    usage:
    melissa_launcher path/to/options.py
"""

import os
import sys
import signal
import imp
signal.signal(signal.SIGINT, signal.SIG_DFL)

def main():
    """
        Import options from command line, and launch Melissa study
    """
    cwd = os.getcwd()
    sys.path.append('@CMAKE_INSTALL_PREFIX@/share/launcher')
    if len(sys.argv) == 2:
        options_path = sys.argv[1]
    else:
        options_path = cwd

    if not os.path.isfile(options_path+"/options.py"):
        print "ERROR no Melissa Launcher options file given"
    else:
        imp.load_source("options", options_path+"/options.py")
        from options import STUDY_OPTIONS as stdy_opt
        from options import MELISSA_STATS as ml_stats
        from options import USER_FUNCTIONS as usr_func
        from study import Study
        melissa_study = Study(stdy_opt, ml_stats, usr_func)
        melissa_study.run()

if __name__ == '__main__':
    main()
