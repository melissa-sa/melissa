
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <zmq.h>
#include "compute_stats.h"
#include "melissa_options.h"
#include "melissa_data.h"
#include "melissa_io.h"
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

int check_simu_state(melissa_field_t *field,
                     int              nb_fields,
                     int              group_id,
                     int              nb_time_steps,
                     comm_data_t     *comm_data)
{
    int i, j, t;

    if (field == NULL)
    {
        return 0;
    }
    else
    {
        for (j=0; j<nb_fields; j++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    if (field[j].stats_data[i].is_valid == 1)
                    {
                        for (t=0; t<nb_time_steps; t++)
                        {
                            if (test_bit (field->stats_data[i].step_simu[group_id], t) == 0)
                            {
                                return 1;
                            }
                        }
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
        }
    }
    return 2;
}

long int count_mbytes_written (melissa_options_t  *options)
{
    long int mbytes_written = 0;
    if (options->mean_op == 1)
    {
        mbytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
    }
    if (options->variance_op == 1)
    {
        mbytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
    }
    if (options->min_and_max_op == 1)
    {
        mbytes_written += 2*options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
    }
    if (options->threshold_op == 1)
    {
        mbytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
    }
    if (options->sobol_op == 1)
    {
        mbytes_written += options->nb_parameters * 2 *options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
    }
    return mbytes_written;
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
 * array of field structures
 *
 * @param[in] nb_fields
 * size of field array
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

void global_confidence_sobol_martinez(melissa_field_t *field,
                                      int              nb_fields,
                                      comm_data_t     *comm_data,
                                      double          *interval1,
                                      double          *interval_tot)
{
    int i, j, t, p, f;
    melissa_data_t *data;
    if (field == NULL)
    {
        return;
    }

    for (f=0; f<nb_fields; f++)
    {
        for (i=0; i<comm_data->client_comm_size; i++)
        {
            if (comm_data->rcounts[i] > 0)
            {
                data = &(field[f].stats_data[i]);

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
}
