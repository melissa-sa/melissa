/**
 *
 * @file threshold.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef THRESHOLD_H
#define THRESHOLD_H

void update_threshold_exceedance (double    in_vect[],
                                  int       threshold_exceedance[],
                                  int       threshold,
                                  const int vect_size);

void write_threshold(int  **threshold,
                     int    vect_size,
                     int    nb_time_steps,
                     FILE*  f);

void read_threshold(int  **threshold,
                    int    vect_size,
                    int    nb_time_steps,
                    FILE*  f);

#endif // THRESHOLD_H
