/**
 *
 * @file stats.h
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 * @defgroup stats_base Basic stats
 * @defgroup sobol Sobol related stats
 *
 **/

#ifndef MELISSA_OPTIONS_H
#define MELISSA_OPTIONS_H

/**
 *******************************************************************************
 *
 * @struct stats_options_s
 *
 * Structure to store options parsed from command line
 *
 *******************************************************************************/

struct stats_options_s
{
    int                  nb_time_steps;    /**< numberb of time steps of the study                               */
    int                  nb_parameters;    /**< nb of variables parameters of the study                          */
    int                  nb_groups;        /**< nb of simulation sets of the study (Sobol only)                  */
    int                  nb_simu;          /**< nb of simulation of the study                                    */
    int                  mean_op;          /**< 1 if the user needs to calculate the mean, 0 otherwise.          */
    int                  variance_op;      /**< 1 if the user needs to calculate the variance, 0 otherwise.      */
    int                  min_and_max_op;   /**< 1 if the user needs to calculate min and max, 0 otherwise.       */
    int                  threshold_op;     /**< 1 if the user needs to compute threshold exceedance, 0 otherwise */
    double               threshold;        /**< threshold used to compute threshold exceedance                   */
    int                  sobol_op;         /**< 1 if the user needs to compute sobol indices, 0 otherwise        */
    int                  sobol_order;      /**< max order of the computes sobol indices                          */
    int                  global_vect_size; /**< global size of input vector                                      */
    int                  restart;          /**< 1 if restart, 0 otherwise                                        */
};

typedef struct stats_options_s stats_options_t; /**< type corresponding to stats_options_s */

void stats_get_options (int               argc,
                        char            **argv,
                        stats_options_t  *options);

void stats_check_options (stats_options_t  *options);

void print_options (stats_options_t *options);

void write_options (stats_options_t *options);

int read_options(stats_options_t  *options);

#endif // MELISSA_OPTIONS_H
