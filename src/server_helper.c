
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef BUILD_WITH_ZMQ
#include <zmq.h>
#endif // BUILD_WITH_ZMQ
#include "stats.h"
#include "server.h"

double stats_get_time ()
{
#ifdef BUILD_WITH_MPI
    return MPI_Wtime();
#else // BUILD_WITH_MPI
    return (double)time(NULL);
#endif // BUILD_WITH_MPI
}

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

    pull_data->push_rank = malloc (pull_data->total_nb_messages * sizeof(int));
    pull_data->pull_rank = malloc (pull_data->total_nb_messages * sizeof(int));
    pull_data->message_sizes = malloc (pull_data->total_nb_messages * sizeof(int));

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
        *field = malloc(sizeof(field_t));
        (*field)->stats_data = malloc (data_size * sizeof(stats_data_t));
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

stats_data_t* get_data_ptr (field_ptr field, char* field_name)
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

void finalize_field_data (field_ptr        field,
                          comm_data_t     *comm_data,
                          pull_data_t     *pull_data,
                          stats_options_t *options,
                          int             *local_vect_sizes
#ifdef BUILD_WITH_PROBES
                          , double *write_time
#endif // BUILD_WITH_PROBES
                          )
{
    int i;
    if (field == NULL)
    {
        return;
    }
    else
    {
        finalize_field_data (field->next, comm_data, pull_data, options, local_vect_sizes
#ifdef BUILD_WITH_PROBES
                             , &total_write_time
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
        start_write_time = stats_get_time();
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
        end_write_time = stats_get_time();
        *write_time += end_write_time - start_write_time;
#endif // BUILD_WITH_PROBES

        for (i=0; i<comm_data->client_comm_size; i++)
        {
            if (comm_data->rcounts[i] > 0)
            {
                free_data (&field->stats_data[i]);
            }
        }
        free (field->stats_data);
        free (field);
        field = NULL;
    }
    return;
}

long int count_bytes_written (stats_options_t  *options)
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

void print_zmq_error(int         ret,
                     const char* port_name)
{
    fprintf(stderr,"ERROR on binding (%s)\n", port_name);
    if (ret == EINVAL)
    {
        fprintf(stderr, "The endpoint supplied is invalid.\n");
    }
    else if (ret == EPROTONOSUPPORT)
    {
        fprintf(stderr, "The requested transport protocol is not supported.\n");
    }
    else if (ret == ENOCOMPATPROTO)
    {
        fprintf(stderr, "The requested transport protocol is not compatible with the socket type.\n");
    }
    else if (ret == EADDRINUSE)
    {
        fprintf(stderr, "The requested address is already in use.\n");
    }
    else if (ret == EADDRNOTAVAIL)
    {
        fprintf(stderr, "The requested address was not local.\n");
    }
    else if (ret == ENODEV)
    {
        fprintf(stderr, "The requested address specifies a nonexistent interface.\n");
    }
    else if (ret == ETERM)
    {
        fprintf(stderr, "The ZeroMQ context associated with the specified socket was terminated.\n");
    }
    else if (ret == ENOTSOCK)
    {
        fprintf(stderr, "The provided socket was invalid.\n");
    }
    else if (ret == EMTHREAD)
    {
        fprintf(stderr, "No I/O thread is available to accomplish the task.\n");
    }
    else
    {
        fprintf(stderr, "Unknown error.\n");
    }
    exit(0);
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
    stats_data_t *data;
    if (field == NULL)
    {
        return;
    }
    global_confidence_sobol_martinez(field->next, comm_data, interval1, interval_tot);

    for (i=0; i<comm_data->client_comm_size; i++)
    {
        if (comm_data->rcounts[i] > 0)
        {
            data = &field->stats_data[i];
            fprintf (stdout, "client %d, rank \n", i, comm_data->rank);

            for (t=0; t<data->options->nb_time_steps; t++)
            {
                fprintf (stdout, "    time step %d, rank \n", t, comm_data->rank);
                for (p=0; p<data->options->nb_parameters; p++)
                {
                    fprintf (stdout, "        parameter %d, rank \n", p, comm_data->rank);
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
