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

#ifndef COMPUTE_STATS_H
#define COMPUTE_STATS_H

#include <melissa/server/data.h>

#ifdef __cplusplus
extern "C" {
#endif

void compute_stats (melissa_data_t  *data,
                    const int        time_step,
                    const int        simu_id,
                    const int        nb_vect,
                    double         **in_vect_tab);

void finalize_stats (melissa_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // COMPUTE_STATS_H
