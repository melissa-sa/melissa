/**
 *
 * @file melissa_options.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef MELISSA_OPTIONS_H
#define MELISSA_OPTIONS_H

/**
 *******************************************************************************
 *
 * @struct melissa_options_s
 *
 * Structure to store options parsed from command line
 *
 *******************************************************************************/

struct melissa_options_s
{
    int                  nb_time_steps;    /**< numberb of time steps of the study                               */
    int                  nb_parameters;    /**< nb of variables parameters of the study                          */
    int                  sampling_size;    /**< nb of randomly drawn simulation parameter sets                   */
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
    char                 master_name[256]; /**< Melissa master node name                                         */
};

typedef struct melissa_options_s melissa_options_t; /**< type corresponding to melissa_options_s */

void melissa_get_options (int                 argc,
                          char              **argv,
                          melissa_options_t  *options);

void melissa_check_options (melissa_options_t  *options);

void melissa_print_options (melissa_options_t *options);

void melissa_write_options (melissa_options_t *options);

int melissa_read_options(melissa_options_t  *options);

#endif // MELISSA_OPTIONS_H
