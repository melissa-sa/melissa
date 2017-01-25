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
#include "threshold.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the number of values exceeding a given threshold
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] threshold_exceedance[]
 * number of threshold exceedance occurences
 *
 * @param[in] threshold
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void update_threshold_exceedance (double    in_vect[],
                                  int       threshold_exceedance[],
                                  int       threshold,
                                  const int vect_size)
{
    int     i;

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
    {
        if (in_vect[i] > threshold)
            threshold_exceedance[i] += 1;
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
 * @param[in] **threshold
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

void write_threshold(int  **threshold,
                     int    vect_size,
                     int    nb_time_steps,
                     FILE*  f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(threshold[i], sizeof(int), vect_size, f);
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
 * @param[in] **threshold
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

void read_threshold(int  **threshold,
                    int    vect_size,
                    int    nb_time_steps,
                    FILE*  f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(threshold[i], sizeof(int), vect_size, f);
    }
}