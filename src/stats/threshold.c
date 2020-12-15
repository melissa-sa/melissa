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

/**
 *
 * @file threshold.c
 * @brief Threshold exceedance related functions.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <melissa/stats/threshold.h>
#include <melissa/utils.h>


/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a threshold structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *threshold
 * the threshold exceedance structure to initialize
 *
 * @param[in] value
 * thresholds
 *
 * @param[in] vect_size
 * size of the variance vector
 *
 *******************************************************************************/

void init_threshold (threshold_t  *threshold,
                     const int     vect_size,
                     const double  value)
{
    threshold->threshold_exceedance = melissa_calloc (vect_size, sizeof(int));
    threshold->value = value;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the number of values exceeding a given threshold
 *
 *******************************************************************************
 *
 * @param[in,out] threshold
 * number of threshold exceedance occurences
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void update_threshold_exceedance (threshold_t *threshold,
                                  double       in_vect[],
                                  const int    vect_size)
{
    int i;

    for (i=0; i<vect_size; i++)
    {
        if (in_vect[i] > threshold->value)
        {
            threshold->threshold_exceedance[i] += 1;
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
 * @param[in] threshold
 * threshold exceedance structure to save
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] nb_values
 * number of threshold
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void save_threshold(threshold_t **threshold,
                    int           vect_size,
                    int           nb_time_steps,
                    int           nb_values,
                    FILE*         f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<nb_values; j++)
        {
            fwrite(&threshold[i][j].value, sizeof(double), 1, f);
            fwrite(threshold[i][j].threshold_exceedance, sizeof(int), vect_size, f);
        }
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
 * @param[in] threshold
 * threshold exceedance structure to read
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] nb_values
 * number of threshold
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_threshold(threshold_t **threshold,
                    int           vect_size,
                    int           nb_time_steps,
                    int           nb_values,
                    FILE*         f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<nb_values; j++)
        {
            fread(&threshold[i][j].value, sizeof(double), 1, f);
            fread(threshold[i][j].threshold_exceedance, sizeof(int), vect_size, f);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a threshold structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *threshold
 * the threshold exceedance structure to free
 *
 *******************************************************************************/

void free_threshold (threshold_t *threshold)
{
    melissa_free (threshold->threshold_exceedance);
}
