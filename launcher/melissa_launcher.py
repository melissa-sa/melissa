#!@PYTHON_EXECUTABLE@

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
import getopt
import logging
signal.signal(signal.SIGINT, signal.SIG_DFL)

def usage():
    print("Usage:")
    print("  melissa_launcher [option [argument]]")
    print("  option:  long option:  argument:               description:")
    print("  -o       --options     <path/to/options/file>  Path to user defined option file")
    print("  -h       --help        NONE                    Print this help")

def main():
    """
        Import options from command line, and launch Melissa study
    """
    cwd = os.getcwd()

    options_path = ""

    try:
        opts, args = getopt.getopt(sys.argv[1:], "ho:", ["help", "options="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    output = None
    verbose = False
    for o, a in opts:
        if o == "-v":
            verbose = True
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-o", "--options"):
            options_path = a
        else:
            assert False, "unhandled option"

    if len(sys.argv) == 2:
        options_path = sys.argv[1]

    if not os.path.isfile(options_path):
        if not os.path.isfile(options_path+"/options.py"):
            print("ERROR no Melissa Launcher options file given")
            usage()
            sys.exit(2)
        else:
            option_file = options_path+"/options.py"
    else:
        option_file = options_path

    imp.load_source("options", option_file)
    from options import STUDY_OPTIONS as stdy_opt
    from options import MELISSA_STATS as ml_stats
    from options import USER_FUNCTIONS as usr_func
    from launcher.study import Study

    
    # init log for launcher
    # TODO should align the verbosity option (1,2,...) with the logging level (à, 10, 20 ....)
    os.makedirs(stdy_opt['working_directory'],exist_ok=True)
    logging.basicConfig(format='%(asctime)s %(message)s',
                            datefmt='%m/%d/%Y %I:%M:%S %p',
                            filename= stdy_opt['working_directory']+'/melissa_launcher.log',
                            filemode='w',
                            level=logging.DEBUG)
    

    melissa_study = Study(stdy_opt, ml_stats, usr_func)
    try:
        melissa_study.run()
    except Exception as e:
        print(e)
        sys.exit(1)

if __name__ == '__main__':
    main()
