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

#ifndef STATS_H
#define STATS_H

#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI

/**
 *******************************************************************************
 *
 * @enum return_status
 *
 * Values that can be returned by functions depending on what appened
 *
 *******************************************************************************/

enum return_status
{
    SUCCESS,                 /**< nothing to report                          */
    WARNING_NOTING_RETURNED, /**< a in/out object is returned unmodified     */
    ERROR_BAD_PARAMETER      /**< a bad parameter was givent to the function */
};

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct mean_s
 *
 * Structure containing an array of means, and the corresponding increment
 *
 *******************************************************************************/

struct mean_s
{
    double *mean;      /**< mean[vect_size]        */
    int     increment; /**< increment of this mean */
};

typedef struct mean_s mean_t; /**< type corresponding to mean_s */

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct variance_s
 *
 * Structure containing an array of variances and the corresponding mean structure
 *
 *******************************************************************************/

struct variance_s
{
    double *variance;       /**< variance[vect_size] */
    mean_t  mean_structure; /**< corresponding mean  */
};

typedef struct variance_s variance_t; /**< type corresponding to variance_s */

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct min_max_s
 *
 * Structure containing two arrays of min and max values
 *
 *******************************************************************************/

struct min_max_s
{
    double *min;     /**< min[vect_size]                          */
    double *max;     /**< max[vect_size]                          */
    int     is_init; /**< 0 before the first update, 1 otherwhise */
};

typedef struct min_max_s min_max_t; /**< type corresponding to min_max_s */

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct covariance_s
 *
 * Structure containing an array of covariances and the corresponding mean structures
 *
 *******************************************************************************/

struct covariance_s
{
    double *covariance; /**< covariance[vect_size] */
    mean_t  mean1;      /**< corresponding mean    */
    mean_t  mean2;      /**< corresponding mean    */
    int     increment;  /**< increment             */
};

typedef struct covariance_s covariance_t; /**< type corresponding to covariance_s */

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * @struct conditional_mean_s
 *
 * Recursive structure of conditional means, needs to be more documented
 *
 *******************************************************************************/

struct conditional_mean_s
{
    mean_t                     mean;                   /**< mean structure                                                  */
    int                       *indices;                /**< -1 if variable parameter, the indice of the parameter otherwise */
    int                        order;                  /**< order of this mean                                              */
    int                        is_leaf;                /**< to kwnow if this mean have sons                                 */
    int                       *indice_ptr;             /**< ptr to the indices in next_conditional_means                    */
    int                        indice_ptr_size;        /**< size of *indice_ptr                                             */
    struct conditional_mean_s *next_conditional_means; /**< sons of conditional_mean                                        */
};

typedef struct conditional_mean_s conditional_mean_t; /**< type corresponding to conditional_mean_s */

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * @struct conditional_variance_s
 *
 * Structure containing a conditional variance and its corresponding mean
 *
 *******************************************************************************/

struct conditional_variance_s
{
    variance_t  variance;      /**< variance structure                                       */
    int        *fixed_indices; /**< 1 if fixed parameter, 0 otherwise (size nb_parameters)   */
    int         order;         /**< order of this variance (nb of 1 in fixed_indices)        */
};

typedef struct conditional_variance_s conditional_variance_t; /**< type corresponding to conditional_variance_s */

///**
// *******************************************************************************
// *
// * @ingroup sobol
// *
// * @struct sobol_index_s
// *
// * Structure containing a conditional variance and its corresponding sobol index vector
// *
// *******************************************************************************/

//struct sobol_index_s
//{
//    conditional_variance_t conditional_variance; /**< numerators of this sobol indices */
//    double                 *values;              /**< values of this sobol indices     */
//};

//typedef struct sobol_index_s sobol_index_t; /**< type corresponding to sobol_index_s */

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
    sobol_martinez_t *sobol_martinez; /**< array of sobol indices, size nb_parameters     */
    variance_t        variance_a;     /**< first set variance needed by Martinez formula  */
    variance_t        variance_b;     /**< second set variance needed by Martinez formula */
};

typedef struct sobol_array_s sobol_array_t; /**< type corresponding to sobol_array_s */

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

/**
 *******************************************************************************
 *
 * @struct stats_data_s
 *
 * Structure to store global parameters
 *
 *******************************************************************************/

struct stats_data_s
{
    int                  vect_size;     /**< local size of input vectors                               */
    stats_options_t     *options;       /**< pointer to an option structure                            */
    int                  is_valid;      /**< 1 if the structure has been checked                       */
    mean_t              *means;         /**< array of mean structures, size nb_time_steps              */
    variance_t          *variances;     /**< array of variance structures, size nb_time_steps          */
    min_max_t           *min_max;       /**< array of min and max structures, size nb_time_steps       */
    int                **thresholds;    /**< array of threshold exceedance vectors, size nb_time_steps */
    conditional_mean_t  *cond_means;    /**< array of conditional mean structures, size nb_time_steps  */
    sobol_array_t       *sobol_indices; /**< array of sobol array structures, size nb_time_steps       */
    char                *computed;      /**< 1 if stats already computed, size nb_time_steps           */
};

typedef struct stats_data_s stats_data_t; /**< type corresponding to stats_data_s */

/**
 *******************************************************************************
 *
 * @struct comm_data_s
 *
 * Structure to store communications parameters
 *
 *******************************************************************************/

struct comm_data_s
{
    int       rank;             /**< rank of the MPI process (0 if sequential)      */
    int       comm_size;        /**< size of the MPI communicator (1 if sequential) */
    int       client_comm_size; /**< size of the clients communicators              */
    int      *rcounts;          /**< counts for receiving datas                     */
    int      *rdispls;          /**< displacements for receiving datas              */
#ifdef BUILD_WITH_MPI
    MPI_Comm  comm;             /**< MPI communicator                               */
#endif // BUILD_WITH_MPI
};

typedef struct comm_data_s comm_data_t; /**< type corresponding to comm_data_s */

void stats_check_options (stats_options_t  *options);

void init_mean(mean_t    *mean,
               const int  vect_size);

void init_variance(variance_t *variance,
                   const int   vect_size);

void init_min_max(min_max_t *min_max,
                  const int  vect_size);

void init_covariance(covariance_t *covariance,
                     const int     vect_size);

void increment_mean (double     in_vect[],
                     mean_t    *partial_mean,
                     const int  vect_size);

void increment_mean_and_variance (double      in_vect[],
                                  variance_t *partial_variance,
                                  const int   vect_size);

void increment_variance (double      in_vect[],
                         variance_t *partial_variance,
                         const int   vect_size);

void min_and_max (double     in_vect[],
                  min_max_t *min_max,
                  const int  vect_size);

void increment_covariance (double        in_vect1[],
                           double        in_vect2[],
                           covariance_t *partial_covariance,
                           const int     vect_size);

void update_mean (mean_t    *mean1,
                  mean_t    *mean2,
                  mean_t    *updated_mean,
                  const int  vect_size);

void update_variance (variance_t *variance1,
                      variance_t *variance2,
                      variance_t *updated_variance,
                      const int   vect_size);

void update_covariance (covariance_t *covariance1,
                        covariance_t *covariance2,
                        covariance_t *updated_covariance,
                        const int     vect_size);

#ifdef BUILD_WITH_MPI
void update_global_mean (mean_t    *mean,
                         const int  vect_size,
                         const int  rank,
                         const int  comm_size,
                         MPI_Comm   comm);

void update_global_variance (variance_t *variance,
                             const int   vect_size,
                             const int   rank,
                             const int   comm_size,
                             MPI_Comm    comm);

void update_global_mean_and_variance (variance_t *variance,
                                      const int   vect_size,
                                      const int   rank,
                                      const int   comm_size,
                                      MPI_Comm    comm);
#endif // BUILD_WITH_MPI

void update_threshold_exceedance (double    in_vect[],
                                  int       threshold_exceedance[],
                                  int       threshold,
                                  const int vect_size);

void free_mean(mean_t *mean);

void free_variance (variance_t *variance);

void free_min_max (min_max_t *min_max);

void free_covariance (covariance_t *covariance);

//void init_conditional_means (conditional_mean_t *conditional_means,
//                             stats_data_t       *data);

//void init_next_conditional_mean (conditional_mean_t *conditional_means,
//                                 const int           depth,
//                                 const int           new_fixed_parameter[],
//                                 const int           previous_indices[],
//                                 stats_data_t       *data);

//void increment_conditional_mean (conditional_mean_t *conditional_means,
//                                 double              in_vect[],
//                                 const int           parameters[],
//                                 stats_data_t       *data);

//int get_conditional_mean (conditional_mean_t *conditional_means,
//                          double             *out_mean,
//                          int                *out_increment,
//                          const int           parameters[],
//                          stats_data_t       *data);

//void free_conditional_mean (conditional_mean_t *conditional_means);

//void init_conditional_variance (conditional_variance_t *conditional_variance,
//                                const int               indices_to_fix[],
//                                stats_data_t           *data);

//int compute_conditional_variance (conditional_variance_t *conditional_variance,
//                                  conditional_mean_t     *conditional_means,
//                                  stats_data_t           *data);

//void free_conditional_variance (conditional_variance_t *conditional_variance);

void init_sobol_martinez (sobol_martinez_t *sobol_indices,
                          int               vect_size);

void increment_sobol_martinez (sobol_array_t *sobol_array,
                               int            nb_parameters,
                               double       **in_vect_tab,
                               int            vect_size);

void free_sobol_martinez (sobol_martinez_t *sobol_indices);

void stats_get_options (int               argc,
                        char            **argv,
                        stats_options_t  *options);

void print_options (stats_options_t *options);

//void free_options (stats_options_t *options);

long int mem_conso (stats_options_t *options);

void init_data (stats_data_t    *data,
                stats_options_t *options,
                int              vect_size);

void check_data (stats_data_t *data);

void free_data (stats_data_t *data);

void compute_stats (stats_data_t  *data,
                    const int      time_step,
                    const int      nb_vect,
                    double       **in_vect_tab);

void write_stats(stats_data_t    **data,
                 stats_options_t  *options,
                 comm_data_t      *comm_data,
                 int              *local_vect_sizes,
                 char             *field);

void write_stats_ensight(stats_data_t    **data,
                         stats_options_t  *options,
                         comm_data_t      *comm_data,
                         int              *local_vect_sizes,
                         char             *field);

void finalize_stats (stats_data_t *data);

#endif // STATS_H
