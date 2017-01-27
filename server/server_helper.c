
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#ifdef BUILD_WITH_ZMQ
#include <zmq.h>
#endif // BUILD_WITH_ZMQ
#include "melissa_data.h"
#include "server.h"

void comm_n_to_m_init (int           *rcounts,
                       int           *rdispls,
                       const int      global_vect_size,
                       const int     *server_vect_size,
                       int           *client_vect_size,
                       const int      nb_proc_client,
                       const int      rank,
                       pull_data_t   *pull_data)
{
    int i;
    int client_rank = 0;
    int client_count = 0;
    int server_rank = 0;
    int server_count = 0;
    int new_message = 0;
    int nb_messages = 0;
    int nb_elem_message = 0;

    pull_data->total_nb_messages = 1;
    pull_data->local_nb_messages = 0;
    pull_data->buff_size = 0;

    if (rank == 0)
    {
        pull_data->local_nb_messages = 1;
    }

    for (i=0; i<global_vect_size; i++)
    {
        if (client_count < client_vect_size[client_rank])
        {
            client_count += 1;
        }
        else
        {
            client_count = 1;
            client_rank += 1;
            new_message = 1;
        }
        if (server_count < server_vect_size[server_rank])
        {
            server_count += 1;
        }
        else
        {
            server_count = 1;
            server_rank += 1;
            new_message = 1;
        }
        if (server_rank == rank)
        {
            rcounts[client_rank] += 1;
        }

        if (new_message == 1)
        {
            pull_data->total_nb_messages += 1;
            if (server_rank == rank)
            {
                pull_data->local_nb_messages += 1;
            }
            new_message = 0;
        }
    }

    rdispls[0] = 0;
    for (i=0; i<nb_proc_client-1; i++)
    {
        rdispls[i+1] = rdispls[i] + rcounts[i];
    }

    new_message = 0;

    pull_data->push_rank = melissa_malloc (pull_data->total_nb_messages * sizeof(int));
    pull_data->pull_rank = melissa_malloc (pull_data->total_nb_messages * sizeof(int));
    pull_data->message_sizes = melissa_malloc (pull_data->total_nb_messages * sizeof(int));

    pull_data->push_rank[0] = 0;
    pull_data->pull_rank[0] = 0;
    client_rank  = 0;
    client_count = 0;
    server_rank  = 0;
    server_count = 0;
    for (i=0; i<global_vect_size; i++)
    {
        if (client_count < client_vect_size[client_rank])
        {
            client_count += 1;
        }
        else
        {
            client_count = 1;
            client_rank += 1;
            new_message = 1;
        }
        if (server_count < server_vect_size[server_rank])
        {
            server_count += 1;
        }
        else
        {
            server_count = 1;
            server_rank += 1;
            new_message = 1;
        }

        if (new_message == 1)
        {
            nb_messages += 1;
            pull_data->push_rank[nb_messages] = client_rank;
            pull_data->pull_rank[nb_messages] = server_rank;
            pull_data->message_sizes[nb_messages - 1] = nb_elem_message;
            new_message = 0;
            if (nb_elem_message > pull_data->buff_size)
            {
                pull_data->buff_size = nb_elem_message;
            }
            nb_elem_message = 0;
        }
        nb_elem_message += 1;
    }
    pull_data->message_sizes [pull_data->total_nb_messages - 1 ] = nb_elem_message;
    if (nb_elem_message > pull_data->buff_size)
    {
        pull_data->buff_size = nb_elem_message;
    }
}

void add_field (field_ptr *field, char* field_name, int data_size)
{
    int i;
    if (*field == NULL)
    {
        *field = melissa_malloc(sizeof(field_t));
        (*field)->stats_data = melissa_malloc (data_size * sizeof(melissa_data_t));
        for (i=0; i<data_size; i++)
        {
            (*field)->stats_data[i].is_valid=0;
        }
        strncpy((*field)->name, field_name, MAX_FIELD_NAME);
        (*field)->next = NULL;
    }
    else
    {
        add_field (&(*field)->next, field_name, data_size);
    }
}

melissa_data_t* get_data_ptr (field_ptr field, char* field_name)
{
    if (field != NULL)
    {
        if (strcmp(field->name, field_name) == 0)
        {
            return field->stats_data;
        }
        else
        {
            return get_data_ptr (field->next, field_name);
        }
    }
    return NULL;
}

void finalize_field_data (field_ptr         field,
                          comm_data_t       *comm_data,
                          pull_data_t       *pull_data,
                          melissa_options_t *options,
                          int               *local_vect_sizes
#ifdef BUILD_WITH_PROBES
                          , double *total_write_time
#endif // BUILD_WITH_PROBES
                          )
{
    double start_write_time, end_write_time;
    int i;
    if (field == NULL)
    {
        return;
    }
    else
    {
        finalize_field_data (field->next, comm_data, pull_data, options, local_vect_sizes
#ifdef BUILD_WITH_PROBES
                             , total_write_time
#endif // BUILD_WITH_PROBES
                             );
        for (i=0; i<comm_data->client_comm_size; i++)
        {
            if (comm_data->rcounts[i] > 0)
            {
                finalize_stats (&field->stats_data[i]);
            }
        }

#ifdef BUILD_WITH_PROBES
        start_write_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
//        write_stats (&(field->stats_data),
//                     options,
//                     comm_data,
//                     local_vect_sizes,
//                     field->name);
        write_stats_ensight (&(field->stats_data),
                             options,
                             comm_data,
                             local_vect_sizes,
                             field->name);
#ifdef BUILD_WITH_PROBES
        end_write_time = melissa_get_time();
        *total_write_time += end_write_time - start_write_time;
#endif // BUILD_WITH_PROBES

        for (i=0; i<comm_data->client_comm_size; i++)
        {
            if (comm_data->rcounts[i] > 0)
            {
                melissa_free_data (&field->stats_data[i]);
            }
        }
        melissa_free (field->stats_data);
        melissa_free (field);
        field = NULL;
    }
    return;
}

long int count_bytes_written (melissa_options_t  *options)
{
    long int bytes_written = 0;
    if (options->mean_op == 1)
    {
        bytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps;
    }
    if (options->variance_op == 1)
    {
        bytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps;
    }
    if (options->min_and_max_op == 1)
    {
        bytes_written += 2*options->global_vect_size*sizeof(float)*options->nb_time_steps;
    }
    if (options->threshold_op == 1)
    {
        bytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps;
    }
    if (options->sobol_op == 1)
    {
        bytes_written += options->nb_parameters * 2 *options->global_vect_size*sizeof(float)*options->nb_time_steps;
    }
    return bytes_written;
}

int string_recv (void  *socket,
                 char  *recv_buff)
{
    char buffer [256];
    int size = zmq_recv (socket, buffer, 255, 0);
    if (size == -1)
    {
        recv_buff[0] = 0;
        return 0;
    }
    if (size > 255)
        size = 255;
    buffer [size] = 0;
    memcpy (recv_buff, buffer, size * sizeof(char));
    return size;
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function computes the confidence interval for Martinez Sobol indices
 *
 *******************************************************************************
 *
 * @param[in] field
 * pointer to a field structure
 *
 * @param[out] *comm_data
 * comm data structure
 *
 * @param[out] *interval1
 * worst confidence interval (first order)
 *
 * @param[out] *interval_tot
 * worst confidence interval (total order)
 *
 *******************************************************************************/

void global_confidence_sobol_martinez(field_ptr     field,
                                      comm_data_t  *comm_data,
                                      double       *interval1,
                                      double       *interval_tot)
{
    int i, j, t, p;
    melissa_data_t *data;
    if (field == NULL)
    {
        return;
    }
    global_confidence_sobol_martinez(field->next, comm_data, interval1, interval_tot);

    for (i=0; i<comm_data->client_comm_size; i++)
    {
        if (comm_data->rcounts[i] > 0)
        {
            data = &(field->stats_data[i]);

            for (t=0; t<data->options->nb_time_steps; t++)
            {
                for (p=0; p<data->options->nb_parameters; p++)
                {
                    if (data->sobol_indices[t].iteration < 4)
                    {
                        return;
                    }
                    for (j=0; j< data->options->nb_parameters; j++)
                    {
                        if (data->sobol_indices[t].sobol_martinez[p].confidence_interval[0] > *interval1)
                        {
                            *interval1 = data->sobol_indices[t].sobol_martinez[p].confidence_interval[0];
                        }
                        if (data->sobol_indices[t].sobol_martinez[p].confidence_interval[1] > *interval_tot)
                        {
                            *interval_tot = data->sobol_indices[t].sobol_martinez[p].confidence_interval[1];
                        }
                    }
                }
            }
        }
    }
}