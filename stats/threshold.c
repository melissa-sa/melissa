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
 * @file threshold.c
 * @brief Threshold exceedance related functions.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#ifdef BUILD_WITH_OPENMP
#include <omp.h>
#endif // BUILD_WITH_OPENMP
#include "threshold.h"
#include "melissa_utils.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the number of values exceeding a given threshold
 *
 *******************************************************************************
 *
 * @param[in,out] threshold_exceedance[]
 * number of threshold exceedance occurences
 *
 * @param[in] threshold
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void update_threshold_exceedance (int       threshold_exceedance[],
                                  double    threshold,
                                  double    in_vect[],
                                  const int vect_size)
{
    int     i;

#pragma omp parallel for schedule(static)
    for (i=0; i<vect_size; i++)
    {
        if (in_vect[i] > threshold)
        {
            threshold_exceedance[i] += 1;
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of threshold exceedance vectors on disc
 *
 *******************************************************************************
 *
 * @param[in] **threshold_exceedance
 * threshold exceedance array of vectors to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void save_threshold(int  **threshold_exceedance,
                    int    vect_size,
                    int    nb_time_steps,
                    FILE*  f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(threshold_exceedance[i], sizeof(int), vect_size, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of threshold exceedance vectors on disc
 *
 *******************************************************************************
 *
 * @param[in] **threshold_exceedance
 * threshold exceedance array of vectors to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_threshold(int  **threshold_exceedance,
                    int    vect_size,
                    int    nb_time_steps,
                    FILE*  f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(threshold_exceedance[i], sizeof(int), vect_size, f);
    }
}
