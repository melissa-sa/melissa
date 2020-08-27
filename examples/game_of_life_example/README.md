# Compilation

```bash
mpicxx \
    -Wall -std=c++11 -pedantic -g \
    src/main.cpp -o game \
    -Iinclude/ -I$HOME/melissa/install/include \
    -L$HOME/melissa/install/lib -lmelissa_api -lzmq
```

# Usage 

`./game(_no_melissa) [simuID] [nRows] [nCols] [nTimesteps] [melissa or nonMelissa]`

To run game of life with melissa turned on type `melissa` in `[melissa or nonMelissa]` parameter, `nonMelissa` for off
