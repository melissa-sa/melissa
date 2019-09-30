#include <iostream>      // I/O
#include <string>        // argv conversion
#include <random>        // random number generator
#include <vector>        // matrix
#include <mpi.h>         // mpi framework
#include <melissa_api.h> // melissa API
#include <lifeUtils.hpp> // utils functions

int main(int argc, char *argv[])
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> randINT(0,100);
    int nRows, nCols, nTime, simuID, coupling;
    std::string fieldName("GOL");
    bool bMelissa;

    MPI::Init(argc,argv);
    auto mpisize = MPI::COMM_WORLD.Get_size();
    auto mpirank = MPI::COMM_WORLD.Get_rank();
    auto mpiroot = 0;
    
    if (argc != 6){
        std::cerr << "Invalid number of parameters" << std::endl;
        MPI::COMM_WORLD.Abort(1);
    }

    simuID = atoi(argv[1]);
    nRows = atoi(argv[2]);
    nCols = atoi(argv[3]);
    nTime = atoi(argv[4]);
    std::string melissa = std::string(argv[5]);
    bMelissa = melissa == "melissa";
    coupling = MELISSA_COUPLING_ZMQ;

    auto nRowsLocal = nRows / mpisize;
    if(mpirank == mpisize - 1){
        nRowsLocal += nRows % mpisize;
    }
    
    auto nRowsLocalWithPadding = nRowsLocal + 2;
    auto nColsLocalWithPadding = nCols + 2;
    
    std::vector<std::vector<int>> gameBoard(nRowsLocalWithPadding, std::vector<int>(nColsLocalWithPadding,0));
    std::vector<std::vector<int>> nextBoard(nRowsLocalWithPadding, std::vector<int>(nColsLocalWithPadding,0));

    for(auto iRow = 1; iRow <= nRowsLocal; ++iRow){
        for(auto iCol = 1; iCol <= nCols; ++iCol){
            gameBoard[iRow][iCol] = randomizeState(randINT(rng), 25);
        }
    }

    auto upperNeighbour = (mpirank == 0) ? mpisize - 1 : mpirank - 1;
    auto lowerNeighbour = (mpirank == mpisize - 1) ? mpiroot : mpirank + 1;
    auto local_vec_size = nCols * nRowsLocal;

    MPI_Comm world_comm; 
    MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
    
    if(bMelissa)
        melissa_init(fieldName.c_str(), local_vec_size, mpisize, mpirank, simuID, world_comm, coupling);

    // Time loop
    for (auto iTime = 0; iTime < nTime; ++iTime){

        communicateAndExchangeInfo(gameBoard, nColsLocalWithPadding, nCols, nRowsLocalWithPadding, nRowsLocal, upperNeighbour, lowerNeighbour);
        
        // ! not useful when doing simulations using melissa
        if(!bMelissa)
            displayTheBoard(gameBoard, mpirank, mpisize, nRowsLocal, nRows, nCols, iTime);
        if(bMelissa)
            sendMatrixToMelissa(gameBoard, nRowsLocal, nCols, fieldName.c_str());

        updateBoard(gameBoard, nextBoard, nRowsLocal, nCols);
       
    }
    if(bMelissa)
        melissa_finalize();
    MPI::Finalize();
    return 0;
}
