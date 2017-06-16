# -*- coding: utf-8 -*-

"""
    Main module of Melissa Launcher.
    usage:
    pyton melissa_launcher.py path/to/options.py
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
    if len(sys.argv) == 2:
        options_path = sys.argv[1]
    else:
        options_path = cwd

    if not os.path.isfile(options_path+"/options.py"):
        print "ERROR no Melissa Launcher options file given"
    else:
        imp.load_source("options", options_path+"/options.py")
        from study import Study
        melissa_study = Study()
        melissa_study.run()

if __name__ == '__main__':
    main()
