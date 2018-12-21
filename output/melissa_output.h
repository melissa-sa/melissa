/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

/**
 *
 * @file melissa_output.h
 * @brief Writes stats on disc.
 * @author Terraz Théophile
 * @date 2017-19-01
 *
 * @defgroup melissa_output output statistics on disc
 *
 **/

#ifndef SERVER_OUTPUT_H
#define SERVER_OUTPUT_H

//#include "hdf5.h"
#include "melissa_data.h"
#include "melissa_utils.h"

void melissa_write_stats_seq(melissa_data_t    **data,
                             melissa_options_t  *options,
                             comm_data_t        *comm_data,
                             char               *field);

void write_stats_txt(melissa_data_t    **data,
                     melissa_options_t  *options,
                     comm_data_t        *comm_data,
                     char               *field);

void write_stats_bin(melissa_data_t    **data,
                     melissa_options_t  *options,
                     comm_data_t        *comm_data,
                     char               *field);

void write_stats_ensight(melissa_data_t    **data,
                         melissa_options_t  *options,
                         comm_data_t        *comm_data,
                         char               *field);

#endif // SERVER_OUTPUT_H
