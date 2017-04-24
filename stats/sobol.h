/**
 *
 * @file sobol.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef SOBOL_H
#define SOBOL_H

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * @struct sobol_jansen_s
 *
 * Structure containing a sobol indices vector with all structures needed by Jansen update formula
 *
 *******************************************************************************/

struct sobol_jansen_s
{
    double       *summ_a;                 /**< summ needed by Jansen formula */
    double       *summ_b;                 /**< summ needed by Jansen formula */
    double       *first_order_values;     /**< values of the sobol indices   */
    double       *total_order_values;     /**< values of the sobol indices   */
};

typedef struct sobol_jansen_s sobol_jansen_t; /**< type corresponding to sobol_martinez_s */

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * @struct sobol_martinez_s
 *
 * Structure containing a sobol indices vector with all structures needed by Martinez update formula
 *
 *******************************************************************************/

struct sobol_martinez_s
{
    covariance_t  first_order_covariance; /**< covariance needed by Martinez formula */
    covariance_t  total_order_covariance; /**< covariance needed by Martinez formula */
    variance_t    variance_k;             /**< variance needed by Martinez formula   */
    double       *first_order_values;     /**< values of the sobol indices           */
    double       *total_order_values;     /**< values of the sobol indices           */
    double        confidence_interval[2]; /**< interval for 95% confidence level     */
};

typedef struct sobol_martinez_s sobol_martinez_t; /**< type corresponding to sobol_martinez_s */

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * @struct sobol_array_s
 *
 * Structure containing an array of sobol index structures
 *
 *******************************************************************************/

struct sobol_array_s
{
    sobol_jansen_t   *sobol_jansen;   /**< array of sobol indices, size nb_parameters     */
    sobol_martinez_t *sobol_martinez; /**< array of sobol indices, size nb_parameters     */
    variance_t        variance_a;     /**< first set variance needed by Martinez formula  */
    variance_t        variance_b;     /**< second set variance needed by Martinez formula */
    int               iteration;      /**< number of computed groups                      */
};

typedef struct sobol_array_s sobol_array_t; /**< type corresponding to sobol_array_s */

void init_sobol_jansen (sobol_jansen_t *sobol_indices,
                        int             vect_size);

void init_sobol_martinez (sobol_martinez_t *sobol_indices,
                          int               vect_size);

void increment_sobol_jansen (sobol_array_t *sobol_array,
                             int            nb_parameters,
                             double       **in_vect_tab,
                             int            vect_size);

void increment_sobol_martinez (sobol_array_t *sobol_array,
                               int            nb_parameters,
                               double       **in_vect_tab,
                               int            vect_size);

void confidence_sobol_martinez(sobol_array_t *sobol_array,
                               int            nb_parameters,
                               int            vect_size);

int check_convergence_sobol_martinez(sobol_array_t **sobol_array,
                                     double          confidence_value,
                                     int             nb_time_steps,
                                     int             nb_parameters);

void write_sobol_martinez(sobol_array_t *sobol_array,
                          int            vect_size,
                          int            nb_time_steps,
                          int            nb_parameters,
                          FILE*          f);

void read_sobol_martinez(sobol_array_t *sobol_array,
                         int            vect_size,
                         int            nb_time_steps,
                         int            nb_parameters,
                         FILE*          f);

void free_sobol_jansen (sobol_jansen_t *sobol_indices);

void free_sobol_martinez (sobol_martinez_t *sobol_indices);

#endif // SOBOL_H
