/**
 *
 * @file threshold.c
 * @brief Threshold exceedance related functions.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <stdlib.h>
#include "stats.h"

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
