To patch code_saturne:

1) code_saturn-4.2.1

in configure, line 28835 and 28836, replace <path_to_melissa_install> by the path to melissa install directory, and <path_to_zeromq_install> by the path to zeromq install directory
in src/fvm/Makefile.in, line 630, replace <path_to_melissa_install> by the path to melissa install directory.
Download and extract code_saturn-4.2.1 tarball
copy the files from this repository to the code_saturne-4.2.1 folder
install code_saturn normaly

2) code_saturn-4.3.1

in configure, line 29092 and 29093, replace <path_to_melissa_install> by the path to melissa install directory, and <path_to_zeromq_install> by the path to zeromq install directory
in src/fvm/Makefile.in, line 634, replace <path_to_melissa_install> by the path to melissa install directory.
Download and extract code_saturn-4.3.1 tarball
copy the files from this repository to the code_saturne-4.3.1 folder
install code_saturn normaly
