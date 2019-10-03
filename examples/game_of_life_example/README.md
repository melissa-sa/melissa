# Compilation

`mpicxx -g  -std=c++11 src/main.cpp -o game -I include/ -I ~/melissa/install/include -L ~/melissa/install/lib -l melissa_api -l zmq -Wall -pedantic`

# Usage 

`./game(_no_melissa) [simuID] [nRows] [nCols] [nTimesteps] [melissa or nonMelissa]`

To run game of life with melissa turned on type `melissa` in `[melissa or nonMelissa]` parameter, `nonMelissa` for off
