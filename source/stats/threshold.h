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
 * @file threshold.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef THRESHOLD_H
#define THRESHOLD_H

void update_threshold_exceedance (int       threshold_exceedance[],
                                  double    threshold,
                                  double    in_vect[],
                                  const int vect_size);

void save_threshold(int  **threshold_exceedance,
                    int    vect_size,
                    int    nb_time_steps,
                    FILE*  f);

void read_threshold(int  **threshold_exceedance,
                    int    vect_size,
                    int    nb_time_steps,
                    FILE*  f);

#endif // THRESHOLD_H
