#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H
#include "melissa_data.h"
#include "melissa_utils.h"
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI

struct field_s /**< Structure for a linked list of output fields */
{
    char            name[MAX_FIELD_NAME]; /**< name of the field                   */
    melissa_data_t *stats_data;           /**< stats_data structure                */
    struct field_s *next;                 /**< pointer to the next field structure */
};
typedef struct field_s field_t; /**< type corresponding to field_s */
typedef field_t* field_ptr; /**< pointer to a structure */

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

void add_field (field_ptr *field, char* field_name, int data_size);

melissa_data_t* get_data_ptr (field_ptr field, char* field_name);

void finalize_field_data (field_ptr          field,
                          comm_data_t       *comm_data,
                          pull_data_t       *pull_data,
                          melissa_options_t *options,
                          int               *local_vect_sizes
#ifdef BUILD_WITH_PROBES
                          , double *write_time
#endif // BUILD_WITH_PROBES
                          );

long int count_mbytes_written (melissa_options_t  *options);

int string_recv (void *socket,
                 char *recv_buff);

void global_confidence_sobol_martinez(field_ptr     field,
                                      comm_data_t  *comm_data,
                                      double       *interval1,
                                      double       *interval_tot);

#endif // SERVER_HELPER_H
