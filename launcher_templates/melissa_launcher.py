import os, sys, signal
import imp
signal.signal(signal.SIGINT, signal.SIG_DFL)

if __name__ == '__main__':

    cwd = os.getcwd()
    if len(sys.argv) == 2:
        options_path = sys.argv[1]
    else:
        options_path = "./"

    if not os.path.isfile(options_path+"/options.py"):
        print "ERROR no Melissa Launcher options file given"
    else:
        imp.load_source("options", options_path+"/options.py")
        from study import *
        from options import *
        my_study = Study()
