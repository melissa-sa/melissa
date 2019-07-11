#pragma once

int randomizeState(int result, int chanceOfAlive){
    return result < chanceOfAlive ? 1 : 0;
}

void updateCell(int& nextCell, const int& currentCell, const int& aliveNeighbours){
    if (aliveNeighbours < 2)
        nextCell = 0;
    else if (currentCell == 1 && (aliveNeighbours == 2 || aliveNeighbours == 3))
        nextCell = 1;
    else if (aliveNeighbours > 3)
        nextCell = 0;
    else if (currentCell == 0 && aliveNeighbours == 3)
        nextCell = 1;
}

void communicateAndExchangeInfo(std::vector<std::vector<int>>& gameBoard, const int& nColsLocalWithPadding, const int& nCols, const int& nRowsLocalWithPadding, const int& nRowsLocal, const int& upperNeighbour, const int& lowerNeighbour){
    // Send top row above
    MPI::COMM_WORLD.Send(&gameBoard[1][0], nColsLocalWithPadding, MPI::INT, upperNeighbour,0);
    
    // Send bottom row below
    MPI::COMM_WORLD.Send(&gameBoard[nRowsLocal][0], nColsLocalWithPadding, MPI::INT, lowerNeighbour, 0);

    //recieve bottom row from below
    MPI::COMM_WORLD.Recv(&gameBoard[nRowsLocal + 1][0], nColsLocalWithPadding, MPI::INT, lowerNeighbour, 0);

    //recieve top row from above
    MPI::COMM_WORLD.Recv(&gameBoard[0][0], nColsLocalWithPadding, MPI::INT, upperNeighbour,0);

    //ghost columns
    for (auto iRow = 0; iRow < nRowsLocalWithPadding; ++iRow){
        gameBoard[iRow][0] = gameBoard[iRow][nCols];
        gameBoard[iRow][nCols + 1] = gameBoard[iRow][1];

    }
}

void displayTheBoard(const std::vector<std::vector<int>>& gameBoard, const int& mpirank, const int& mpisize, const int& nRowsLocal, const int& nRows, const int& nCols, const int& iTime){

    if (mpirank != 0){
        for (auto iRow = 1; iRow <= nRowsLocal; ++iRow)
            MPI::COMM_WORLD.Send(&gameBoard[iRow][1], nCols, MPI::INT, 0, 0);
    }
    //print own part of board
    else{
        std::cout << "iTime: " << iTime << " ------------------" << std::endl;
        for (auto iRow = 1; iRow <= nRowsLocal; ++iRow){
            for (auto iCol = 1; iCol <= nCols; ++iCol){
                std::cout << gameBoard[iRow][iCol] << " "; 
            }
            std::cout << std::endl;
        }

        //recieve rows from other processes
        for (auto iSendingProcess = 1; iSendingProcess < mpisize; ++iSendingProcess){
            auto nRecv = nRows / mpisize;
            if (iSendingProcess == mpisize - 1) nRecv += nRows % mpisize;

            std::vector<int> buffer(nCols, 0);
            for (auto iRecv = 0; iRecv < nRecv; ++iRecv){
                MPI::COMM_WORLD.Recv(&buffer[0], nCols, MPI::INT, iSendingProcess, 0);
                for (auto i : buffer) std::cout << i << " ";
                std::cout << std::endl;
            }
        }
    }
}

void updateBoard(std::vector<std::vector<int>>& gameBoard, std::vector<std::vector<int>> nextBoard, const int& nRowsLocal, const int& nCols){
    for (auto iRow = 1; iRow <= nRowsLocal; ++iRow){
            for (auto iCol = 1; iCol <= nCols; ++iCol){

                auto nAliveNeighbours = 0;

                for (auto jRow = iRow - 1; jRow <= iRow + 1; ++jRow){
                    for (auto jCol = iCol - 1; jCol <= iCol + 1; ++jCol){
                        if((jRow != iRow || jCol != iCol) && gameBoard[jRow][jCol] == 1)
                            ++nAliveNeighbours;
                    }
                }

                updateCell(nextBoard[iRow][iCol], gameBoard[iRow][iCol], nAliveNeighbours);

                for (auto iRow = 1; iRow <= nRowsLocal; ++iRow)
                    for (auto iCol = 1; iCol <= nCols; ++iCol)
                        gameBoard[iRow][iCol] = nextBoard[iRow][iCol];

            }
        }
}

void sendMatrixToMelissa(const std::vector<std::vector<int>>& gameBoard, const int& nRowsLocal, const int& nCols, const char* fieldName){
    
    //convert 2d vector array to contiguous memory
    double* arrayToSend = new double[nRowsLocal*nCols];
    int arrayIterator = 0;
    for (int iRow = 1; iRow <= nRowsLocal; ++iRow){
        for (int iCol = 1; iCol <= nCols; ++iCol){
            arrayToSend[arrayIterator] = static_cast<double>(gameBoard[iRow][iCol]);
            ++arrayIterator;
        }
    }
    melissa_send(fieldName, arrayToSend);

    delete[] arrayToSend;
}