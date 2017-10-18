/**
 *
 * @file server.h
 * @author Terraz Théophile
 * @date 2016-15-03
 *
 **/

#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H
#include "melissa_fields.h"
#include "melissa_data.h"
#include "melissa_utils.h"
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI

struct pull_data_s /**< Helper structure for push pull socket */
{
    int   *pull_rank;         /**< array of receiver ranks, size total_nb_messages */
    int   *push_rank;         /**< array of sender ranks, size total_nb_messages   */
    int   *message_sizes;     /**< messages sizes, size total_nb_messages          */
    int    total_nb_messages; /**< total number of messages                        */
    int    local_nb_messages; /**< local number of messages                        */
    int    buff_size;         /**< recieve buffer size                             */
};

typedef struct pull_data_s pull_data_t; /**< type corresponding to pull_data_s */

void comm_n_to_m_init (int           *rcounts,
                       int           *rdispls,
                       const int      global_vect_size,
                       const int     *server_vect_size,
                       int           *client_vect_size,
                       const int      nb_proc_client,
                       const int      rank,
                       pull_data_t   *pull_data);

//void add_field (field_ptr *field,
//                char*      field_name,
//                int        data_size);

int check_simu_state(melissa_field_t *field,
                     int              nb_fields,
                     int              group_id,
                     int              nb_time_steps,
                     comm_data_t     *comm_data);

long int count_mbytes_written (melissa_options_t  *options);

int string_recv (void *socket,
                 char *recv_buff);

void global_confidence_sobol_martinez(melissa_field_t *field,
                                      int              nb_fields,
                                      comm_data_t     *comm_data,
                                      double          *interval1,
                                      double          *interval_tot);

#endif // SERVER_HELPER_H
