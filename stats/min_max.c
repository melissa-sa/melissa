/**
 *
 * @file min_max.c
 * @brief Min and max related functions.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "min_max.h"
#include "melissa_utils.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a min and max structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *min_max
 * the min and max structure to initialize
 *
 * @param[in] vect_size
 * size of the vectors
 *
 *******************************************************************************/

void init_min_max (min_max_t *min_max,
                   const int  vect_size)
{
    min_max->min = melissa_malloc (vect_size * sizeof(double));
    min_max->max = melissa_malloc (vect_size * sizeof(double));
    min_max->is_init = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the min and the max values of min and max vectors
 * using the input vector.
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] *min_max
 * the min and max structure
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void min_and_max (double     in_vect[],
                  min_max_t *min_max,
                  const int  vect_size)
{
    if (min_max->is_init == 0)
    {
        memcpy (min_max->min, in_vect, vect_size * sizeof(double));
        memcpy (min_max->max, in_vect, vect_size * sizeof(double));
        min_max->is_init = 1;
    }
    else
    {
        int     i;
#pragma omp parallel for
        for (i=0; i<vect_size; i++)
        {
            if (min_max->min[i] > in_vect[i])
                min_max->min[i] = in_vect[i];
            if (min_max->max[i] < in_vect[i])
                min_max->max[i] = in_vect[i];
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of min and max structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *minmax
 * min and max structures to save, size nb_time_steps
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

void write_min_max(min_max_t *minmax,
                   int        vect_size,
                   int        nb_time_steps,
                   FILE*      f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(minmax[i].min, sizeof(double), vect_size, f);
        fwrite(minmax[i].max, sizeof(double), vect_size, f);
        fwrite(&minmax[i].is_init, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of min and max structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *minmax
 * min and max structures to read, size nb_time_steps
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

void read_min_max(min_max_t *minmax,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(minmax[i].min, sizeof(double), vect_size, f);
        fread(minmax[i].max, sizeof(double), vect_size, f);
        fread(&minmax[i].is_init, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a min and max structure.
 *
 *******************************************************************************
 *
 * @param[in] *min_max
 * the min and max structure to free
 *
 *******************************************************************************/

void free_min_max (min_max_t *min_max)
{
    melissa_free (min_max->min);
    melissa_free (min_max->max);
}
