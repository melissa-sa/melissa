import numpy as np
import numpy.random as rd
import subprocess
import socket
from study import *

def call_bash(s):
    proc = subprocess.Popen(s, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, universal_newlines=True)
    (out, err) = proc.communicate()
    print "out = "+str(out)
    print "err = "+str(err)
    return {'out':str(out[:len(out)-int(out[-1]=="\n")]) if len(out)>0 else "", 'err':str(err[:len(err)-int(err[-1]=="\n")]) if len(err)>0 else ""}


class melissa_matrices:
    def __init__(self, options):
        self.sobol = "sobol" in options.operations
        self.A = self.create_matrix(options)
        if (sobol == True):
            self.B = self.create_matrix(options)
            self.C = [self.create_matrix_k(A, B, i) for i in range(nb_parameters)]
        
    def create_matrix(self, options):
        for i in range(options.nb_parameters):
            A[:,i] = rd.uniform(options.range_min[i], options.range_max[i], options.sampling_size)
        return A

    def create_matrix_k(self, A, B, k):
        Ck = np.copy(A)
        Ck[:,k] = B[:,k]
        return Ck

        
if __name__ == '__main__':
    my_study = Study()
