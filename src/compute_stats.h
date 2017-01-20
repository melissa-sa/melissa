/**
 *
 * @file compute_stats.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef COMPUTE_STATS_H
#define COMPUTE_STATS_H
#include "melissa_data.h"

void compute_stats (melissa_data_t *data,
                    const int       time_step,
                    const int       nb_vect,
                    double        **in_vect_tab);

void finalize_stats (melissa_data_t *data);

#endif // COMPUTE_STATS_H
