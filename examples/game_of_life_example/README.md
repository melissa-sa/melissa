# Compilation

melissa version: `mpicxx -g  -std=c++11 src/main.cpp -o game -I include/ -I ~/melissa/install/include -L ~/melissa/install/lib -l melissa_api -l zmq -Wall -pedantic`

non melissa: `mpicxx -g  -std=c++11 src/main_no_melissa.cpp -o game_no_melissa -I include/ -I ~/melissa/install/include -L ~/melissa/install/lib -l melissa_api -l zmq -Wall -pedantic`

# Usage 

`./game(_no_melissa) [simuID] [nRows] [nCols] [nTimesteps]`
