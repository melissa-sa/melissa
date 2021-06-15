/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENSE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

#include <melissa/server/options.h>
#include <melissa/server/server.h>

#include <mpi.h>

int main(int argc, char **argv)
{
    melissa_server_t server;

    MPI_Init(&argc, &argv);
    melissa_get_options(argc, argv, &server);
    melissa_print_options(&server.melissa_options);

    MPI_Finalize();
}
