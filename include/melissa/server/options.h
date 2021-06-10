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

#ifndef MELISSA_OPTIONS_H
#define MELISSA_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

struct melissa_server_s;

/**
 * @struct melissa_options_s
 *
 * Structure to store options parsed from command line
 *
 */

struct melissa_options_s
{
    int                  nb_time_steps;           /**< number of time steps of the study                                */
    int                  nb_parameters;           /**< nb of variables parameters of the study                          */
    int                  sampling_size;           /**< nb of randomly drawn simulation parameter sets                   */
    int                  nb_simu;                 /**< nb of simulation of the study                                    */
    int                  nb_fields;               /**< nb of fields of the simulations                                  */
    int                  mean_op;                 /**< 1 if the user needs to compute the means, 0 otherwise.           */
    int                  variance_op;             /**< 1 if the user needs to compute the variances, 0 otherwise.       */
    int                  skewness_op;             /**< 1 if the user needs to compute the skewness, 0 otherwise.        */
    int                  kurtosis_op;             /**< 1 if the user needs to compute the kurtosis, 0 otherwise.        */
    int                  min_and_max_op;          /**< 1 if the user needs to compute min and max, 0 otherwise.         */
    int                  threshold_op;            /**< 1 if the user needs to compute threshold exceedance, 0 otherwise */
    int                  nb_thresholds;           /**< number of threshold exceedance to compute                        */
    double              *threshold;               /**< threshold values used to compute threshold exceedance            */
    int                  quantile_op;             /**< 1 if the user needs to compute quantiles, 0 otherwise            */
    int                  nb_quantiles;            /**< number of quantile fields                                        */
    double              *quantile_order;          /**< array of quantile orders                                         */
    int                  sobol_op;                /**< 1 if the user needs to compute sobol indices, 0 otherwise        */
    int                  sobol_order;             /**< max order of the computes sobol indices                          */
    int                  learning;                /**< > 1 if the user needs to do learning, 0 otherwise.               */
    int                  restart;                 /**< 1 if restart, 0 otherwise                                        */
    double               check_interval;          /**< interval between checkpoints                                     */
    int                  timeout_simu;            /**< time to wait for next simulation message before timeout          */
    char                 restart_dir[256];        /**< Melissa restart files directory                                  */
    char                 launcher_name[256];      /**< Melissa launcher node name                                       */
    int                  txt_pull_port;           /**< Melissa launcher pull port number                                */
    int                  txt_push_port;           /**< Melissa launcher push port number                                */
    int                  txt_req_port;            /**< Melissa launcher request port number                             */
    int                  data_port;               /**< Data port number                                                 */
    int                  verbose_lvl;             /**< requested level of verbosity                                     */
    int                  disable_fault_tolerance; /**< 1 to disable fault tolerance, 0 otherwise                        */
};

typedef struct melissa_options_s melissa_options_t; /**< type corresponding to melissa_options_s */

void melissa_get_options(int argc, char **argv, struct melissa_server_s* server);

void melissa_check_options (melissa_options_t  *options);

void melissa_print_options (melissa_options_t *options);


/**
 * This function initializes the list of fields with the field names passed on
 * the command line.
 *
 * @param[in,out] optarg The argument of the `-f` option
 * @param[in,out] server The server state
 */
int melissa_options_get_fields(char* optarg, struct melissa_server_s* server);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_OPTIONS_H
