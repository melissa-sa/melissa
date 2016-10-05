
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#ifdef BUILD_WITH_ZMQ
#include <zmq.h>
#endif // BUILD_WITH_ZMQ
#include "stats.h"

#ifndef MPI_MAX_PROCESSOR_NAME
#define MPI_MAX_PROCESSOR_NAME 256
#endif

#ifndef MAX_FIELD_NAME
#define MAX_FIELD_NAME 128
#endif

static volatile int end_signal = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1 || signo == SIGINT || signo == SIGUSR2)
        end_signal = 1;
}

struct field_s /**< Structure for a linked list of output fields */
{
    char            name[MAX_FIELD_NAME]; /**< name of the field                   */
    stats_data_t   *stats_data;           /**< stats_data structure                */
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

#ifdef BUILD_WITH_PROBES
static double start_time;
static double total_comm_time = 0;
static double start_comm_time;
static double end_comm_time;
static double total_computation_time = 0;
static double start_computation_time;
static double end_computation_time;
static double total_wait_time = 0;
static double start_wait_time;
static double end_wait_time;
static double total_write_time = 0;
static double start_write_time;
static double end_write_time;
static long int total_bytes_recv = 0;
static long int total_bytes_written = 0;
#endif // BUILD_WITH_PROBES

static double stats_get_time ()
{
#ifdef BUILD_WITH_MPI
    return MPI_Wtime();
#else // BUILD_WITH_MPI
    return (double)time(NULL);
#endif // BUILD_WITH_MPI
}

static inline void comm_n_to_m_init (int           *rcounts,
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
}

static inline void add_field (field_ptr *field, char* field_name, int data_size)
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

static inline stats_data_t* get_data_ptr (field_ptr field, char* field_name)
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

static inline void finalize_field_data (field_ptr        field,
                                        comm_data_t     *comm_data,
                                        pull_data_t     *pull_data,
                                        stats_options_t *options,
                                        int             *local_vect_sizes)
{
    int i;
    if (field == NULL)
    {
        return;
    }
    else
    {
        finalize_field_data (field->next, comm_data, pull_data, options, local_vect_sizes);
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
        total_write_time += end_write_time - start_write_time;
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
        bytes_written += options->global_vect_size*sizeof(double)*options->nb_time_steps;
    }
    if (options->variance_op == 1)
    {
        bytes_written += options->global_vect_size*sizeof(double)*options->nb_time_steps;
    }
    if (options->min_and_max_op == 1)
    {
        bytes_written += 2*options->global_vect_size*sizeof(double)*options->nb_time_steps;
    }
    if (options->threshold_op == 1)
    {
        bytes_written += options->global_vect_size*sizeof(int)*options->nb_time_steps;
    }
    return bytes_written;
}

int main (int argc, char **argv)
{
    stats_options_t     stats_options;
    stats_data_t       *data_ptr;
    field_ptr           field = NULL;
    comm_data_t         comm_data;
    int                 time_step;
    int                 buff_size, i, j, ret, client_rank;
    char               *buffer, *buf_ptr;
    double            **buff_tab_ptr;
    int                 iteration = 0, nb_iterations = 0;
    int                 port_no;
    char                port_name[128] = {0};
    char               *node_names;
    int                 sinit_tab[2], rinit_tab[2];
    void               *context = zmq_ctx_new ();
    char                node_name[MPI_MAX_PROCESSOR_NAME];
    void               *connexion_responder = zmq_socket (context, ZMQ_REP);
    void              **init_responder;
    void              **init_responder2;
    void               *data_puller = zmq_socket (context, ZMQ_PULL);
    void               *sobol_ready_responder = zmq_socket (context, ZMQ_REP);
    void              **sobol_data_responder;
    void              **sobol_data_responder2;
    int                 nb_fields = 0;
    int                 first_init = 1;
    int                 first_connect = 1;
    int                 get_next_message = 0;
    int                *client_vect_sizes, *local_vect_sizes;
    int                 nb_simu = 1;
//    int                 nb_sobol_groups = 2;
    pull_data_t         pull_data;
    int                 nb_bufferized_messages = 50;
    char               *field_name_ptr;

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
    comm_data.comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm_data.comm, &comm_data.comm_size);
    MPI_Comm_rank (comm_data.comm, &comm_data.rank);
    int resultlen;
    MPI_Get_processor_name(node_name, &resultlen);
#else
    comm_data.comm_size       = 1;
    comm_data.rank            = 0;
    gethostname(node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

#ifdef BUILD_WITH_PROBES
    start_time = stats_get_time();
#endif // BUILD_WITH_PROBES

    if (signal(SIGINT, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGINT\n");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGUSR1\n");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGUSR2\n");

    stats_get_options (argc, argv, &stats_options);
    print_options (&stats_options);
//    parameters = calloc (stats_options.nb_parameters, sizeof(int));

    nb_iterations = stats_options.nb_simu * stats_options.nb_time_steps ;
    fprintf(stderr,"nb simu : %d, nb time steps : %d, nb groups : %d, nb param : %d \n",
            stats_options.nb_simu, stats_options.nb_time_steps, stats_options.nb_groups, stats_options.nb_parameters);

    port_no = 32123 + comm_data.rank;
    sprintf (port_name, "tcp://*:%d", port_no);
    zmq_setsockopt (data_puller, ZMQ_RCVHWM, &nb_bufferized_messages, sizeof(int));
    ret = zmq_bind (data_puller, port_name);
    if (ret != 0)
    {
        fprintf(stderr,"ERROR on binding\n");
        exit(0);
    }

    port_no = 123 + comm_data.rank;
    sprintf (port_name, "tcp://*:10%d", port_no);
    ret = zmq_bind (sobol_ready_responder, port_name);
    fprintf(stderr,"open 10%d\n", port_no);
    if (ret != 0)
    {
        fprintf(stderr,"ERROR on binding\n");
        exit(0);
    }

    if (comm_data.rank == 0)
    {
        if (stats_options.sobol_op == 1)
        {
            init_responder = malloc ((stats_options.nb_groups) * sizeof (void*));
            for (i=0; i<stats_options.nb_groups; i++)
            {
                init_responder[i] = zmq_socket (context, ZMQ_REP);
                if (i<10)
                {
                  sprintf (port_name, "tcp://*:210%d", i);
                }
                else
                {
                    sprintf (port_name, "tcp://*:21%d", i);
                }
                ret = zmq_bind (init_responder[i], port_name);
                if (ret != 0)
                {
                    fprintf(stderr,"ERROR on binding\n");
                    exit(0);
                }
            }
            init_responder2 = malloc ((stats_options.nb_groups) * sizeof (void*));
            for (i=0; i<stats_options.nb_groups; i++)
            {
                init_responder2[i] = zmq_socket (context, ZMQ_REP);
                if (i<10)
                {
                  sprintf (port_name, "tcp://*:220%d", i);
                }
                else
                {
                    sprintf (port_name, "tcp://*:22%d", i);
                }
                ret = zmq_bind (init_responder2[i], port_name);
                if (ret != 0)
                {
                    fprintf(stderr,"ERROR on binding\n");
                    exit(0);
                }
            }
        }
        else
        {
            init_responder = malloc (sizeof (void*));
            init_responder[0] = zmq_socket (context, ZMQ_REP);
            ret = zmq_bind (init_responder[0], "tcp://*:21000");
            if (ret != 0)
            {
                fprintf(stderr,"ERROR on binding\n");
                exit(0);
            }
        }
        ret = zmq_bind (connexion_responder, "tcp://*:20000");
        if (ret != 0)
        {
            fprintf(stderr,"ERROR on binding\n");
            exit(0);
        }

        node_names = malloc (MPI_MAX_PROCESSOR_NAME * comm_data.comm_size * sizeof(char));

        first_connect = 2;
        first_init = 2;
    }

#ifdef BUILD_WITH_MPI
    MPI_Gather(node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm_data.comm);
#else // BUILD_WITH_MPI
    memcpy (node_names, node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

    sinit_tab[0] = comm_data.comm_size;
    sinit_tab[1] = MPI_MAX_PROCESSOR_NAME;
    while (1)
    {

#ifdef BUILD_WITH_PROBES
        start_wait_time = stats_get_time();
#endif // BUILD_WITH_PROBES
        zmq_pollitem_t items [] = {
            { connexion_responder, 0, ZMQ_POLLIN, 0 },
            { data_puller, 0, ZMQ_POLLIN, 0 },
            { sobol_ready_responder, 0, ZMQ_POLLIN, 0 }
        };
        zmq_poll (items, 3, 100);
#ifdef BUILD_WITH_PROBES
        end_wait_time = stats_get_time();
        total_wait_time += end_wait_time - start_wait_time;
#endif // BUILD_WITH_PROBES

        if (items[0].revents & ZMQ_POLLIN)
        {
            if (comm_data.rank == 0)
            {
#ifdef BUILD_WITH_PROBES
                start_comm_time = stats_get_time();
#endif // BUILD_WITH_PROBES
                // new simulation wants to connect
                zmq_recv (connexion_responder, rinit_tab, 2 * sizeof(int), 0);
                zmq_send (connexion_responder, sinit_tab, 2 * sizeof(int), 0);
                get_next_message = 1;
                if (first_init == 2)
                {
                    first_init = 1;
                }
#ifdef BUILD_WITH_PROBES
                end_comm_time = stats_get_time();
                total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
            }
        }

        if (first_init == 1)
        {
#ifdef BUILD_WITH_MPI
            MPI_Bcast(rinit_tab, 2, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
            comm_data.client_comm_size = rinit_tab[0];
            client_vect_sizes = malloc (comm_data.client_comm_size * sizeof(int));
            first_init = 0;
#ifdef BUILD_WITH_PROBES
                fprintf (stdout, "Client MPI communicator size = %d\n", comm_data.client_comm_size);
#endif // BUILD_WITH_PROBES
        }

        if (get_next_message == 1)
        {
#ifdef BUILD_WITH_PROBES
            start_comm_time = stats_get_time();
#endif // BUILD_WITH_PROBES
            // new simulation wants to connect, step two
            if (stats_options.sobol_op == 1)
            {
                for (i=0; i<stats_options.nb_parameters+2; i++)
                {
                    zmq_recv (init_responder[rinit_tab[1]], rinit_tab, 2 * sizeof(int), 0);
                    fprintf(stderr, "wait responder 1 ...");
                    zmq_send (init_responder[rinit_tab[1]], sinit_tab, 2 * sizeof(int), 0);
                    fprintf(stderr, "OK\n");
                }
                for (i=0; i<stats_options.nb_parameters+2; i++)
                {
                    zmq_recv (init_responder2[rinit_tab[1]], client_vect_sizes, comm_data.client_comm_size * sizeof(int), 0);
                    fprintf(stderr, "wait responder 2 ...");
                    zmq_send (init_responder2[rinit_tab[1]], node_names, comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
                    fprintf(stderr, "OK\n");
                }
            }
            else
            {
                zmq_recv (init_responder[0], client_vect_sizes, comm_data.client_comm_size * sizeof(int), 0);
                zmq_send (init_responder[0], node_names, comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
            }
            get_next_message = 0;
            if (first_connect == 2)
            {
                first_connect = 1;
            }
#ifdef BUILD_WITH_PROBES
            end_comm_time = stats_get_time();
            total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
        }
        if (first_connect == 1 && first_init == 0)
        {
#ifdef BUILD_WITH_MPI
            MPI_Bcast(client_vect_sizes, comm_data.client_comm_size, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
            stats_options.global_vect_size = 0;
            for (i=0; i< comm_data.client_comm_size; i++)
            {
                stats_options.global_vect_size += client_vect_sizes[i];
            }
            local_vect_sizes = malloc (comm_data.comm_size * sizeof(int));
            for (i=0; i<comm_data.comm_size; i++)
            {
                local_vect_sizes[i] = stats_options.global_vect_size / comm_data.comm_size;
                if (i < stats_options.global_vect_size % comm_data.comm_size)
                    local_vect_sizes[i] += 1;
            }

            comm_data.rcounts = calloc (comm_data.client_comm_size, sizeof(int));
            comm_data.rdispls = calloc (comm_data.client_comm_size, sizeof(int));

            comm_n_to_m_init (comm_data.rcounts,
                              comm_data.rdispls,
                              stats_options.global_vect_size,
                              local_vect_sizes,
                              client_vect_sizes,
                              comm_data.client_comm_size,
                              comm_data.rank,
                              &pull_data);
            nb_iterations *= pull_data.local_nb_messages;
            buff_size = pull_data.buff_size * sizeof(double) + MAX_FIELD_NAME * sizeof(char) + 2 * sizeof(int);
            if (stats_options.sobol_op == 1)
            {
                buff_size += (2 * sizeof(int));
            }
            buffer = malloc (buff_size);

            if (stats_options.sobol_op == 1)
            {
                buff_tab_ptr = malloc ((stats_options.nb_parameters + 2) * sizeof(double*));
                for (i=0; i<stats_options.nb_parameters+2; i++)
                {
                    buff_tab_ptr[i] = malloc (pull_data.buff_size * sizeof(double));
                }
                sobol_data_responder = malloc (stats_options.nb_groups * comm_data.client_comm_size * sizeof(void*));

                for (i=0; i<stats_options.nb_groups; i++)
                {
                    for (j=0; j<comm_data.client_comm_size; j++)
                    {
                        port_no = 12 + comm_data.rank;
                        sobol_data_responder[i*comm_data.client_comm_size + j] = zmq_socket (context, ZMQ_REP);
                        sprintf (port_name, "tcp://*:20%d%d", port_no, comm_data.client_comm_size*i + j);
                        ret = zmq_bind (sobol_data_responder[i*comm_data.client_comm_size + j], port_name);
                        fprintf(stderr,"open port 20%d%d\n", port_no, comm_data.client_comm_size*i + j);
                        if (ret != 0)
                        {
                            fprintf(stderr,"ERROR on binding\n");
                            exit(0);
                        }
                    }
                }
                sobol_data_responder2 = malloc (stats_options.nb_groups * comm_data.client_comm_size * sizeof(void*));
                for (i=0; i<stats_options.nb_groups; i++)
                {
                    for (j=0; j<comm_data.client_comm_size; j++)
                    {
                        port_no = 12 + comm_data.rank;
                        sobol_data_responder2[i*comm_data.client_comm_size + j] = zmq_socket (context, ZMQ_REP);
                        sprintf (port_name, "tcp://*:30%d%d", port_no, comm_data.client_comm_size*i + j);
                        ret = zmq_bind (sobol_data_responder2[i*comm_data.client_comm_size + j], port_name);
                        fprintf(stderr,"open port 30%d%d\n", port_no, comm_data.client_comm_size*i + j);
                        if (ret != 0)
                        {
                            fprintf(stderr,"ERROR on binding\n");
                            exit(0);
                        }
                    }
                }
            }
            else
            {
                buff_tab_ptr = malloc (sizeof(double*));
            }
            first_connect = 0;
        }

        if (items[1].revents & ZMQ_POLLIN)
        {
#ifdef BUILD_WITH_PROBES
            start_comm_time = stats_get_time();
#endif // BUILD_WITH_PROBES
            zmq_recv (data_puller, buffer, buff_size, 0);
#ifdef BUILD_WITH_PROBES
            end_comm_time = stats_get_time();
            total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES

            buf_ptr = buffer;
            time_step = *buf_ptr;
            buf_ptr += sizeof(int);
            client_rank = *buf_ptr;
            buf_ptr += sizeof(int);
//            memcpy(parameters, buf_ptr, stats_options.nb_parameters * sizeof(int));
//            buf_ptr += stats_options.nb_parameters * sizeof(int);
            field_name_ptr = buf_ptr;
            if (field == NULL)
            {
                add_field(&field, field_name_ptr, comm_data.client_comm_size);
                data_ptr = get_data_ptr (field, field_name_ptr);
                nb_fields += 1;
            }
            else
            {
                data_ptr = get_data_ptr (field, field_name_ptr);
                if (data_ptr == NULL)
                {
                    add_field(&field, field_name_ptr, comm_data.client_comm_size);
                    data_ptr = get_data_ptr (field, field_name_ptr);
                    nb_fields += 1;
                }
            }
            if (data_ptr[client_rank].is_valid != 1)
            {
                init_data (&data_ptr[client_rank], &stats_options, comm_data.rcounts[client_rank]);
            }
            buf_ptr += MAX_FIELD_NAME * sizeof(char);
#ifdef BUILD_WITH_PROBES
            total_bytes_recv += buff_size;
            start_computation_time = stats_get_time();
#endif // BUILD_WITH_PROBES

            buff_tab_ptr[0] = (double*)buf_ptr;
            compute_stats (&data_ptr[client_rank], time_step-1, 1, buff_tab_ptr);
            iteration++;
#ifdef BUILD_WITH_PROBES
            end_computation_time = stats_get_time();
            total_computation_time += end_computation_time - start_computation_time;
#endif // BUILD_WITH_PROBES
            printf("iteration %d / %d - field %s - process %d\n", iteration, nb_iterations, field_name_ptr, comm_data.rank);
        }

        if (items[2].revents & ZMQ_POLLIN)
        {
            fprintf(stderr, "                            -- Connexion -- \n");
            int simu_id, group_id;
            zmq_recv (sobol_ready_responder, buffer, buff_size, 0);
            zmq_send (sobol_ready_responder, &comm_data.rank, sizeof(int), 0);

            buf_ptr = buffer;
            time_step = *buf_ptr;
            buf_ptr += sizeof(int);
            simu_id = *buf_ptr;
            buf_ptr += sizeof(int);
            group_id = *buf_ptr;
            buf_ptr += sizeof(int);
            client_rank = *buf_ptr;
            buf_ptr += sizeof(int);
            field_name_ptr = buf_ptr;
            if (field == NULL)
            {
                add_field(&field, field_name_ptr, comm_data.client_comm_size);
                data_ptr = get_data_ptr (field, field_name_ptr);
                nb_fields += 1;
            }
            else
            {
                data_ptr = get_data_ptr (field, field_name_ptr);
                if (data_ptr == NULL)
                {
                    add_field(&field, field_name_ptr, comm_data.client_comm_size);
                    data_ptr = get_data_ptr (field, field_name_ptr);
                    nb_fields += 1;
                }
            }
            if (data_ptr[client_rank].is_valid != 1)
            {
                init_data (&data_ptr[client_rank], &stats_options, comm_data.rcounts[client_rank]);
            }
            buf_ptr += MAX_FIELD_NAME * sizeof(char);
            fprintf(stderr, "                           goupe %d\n", group_id);
            memcpy(buff_tab_ptr[simu_id], buf_ptr, pull_data.buff_size * sizeof(double));

            for (i=1; i<stats_options.nb_parameters+2; i++)
            {
                zmq_recv (sobol_data_responder[group_id*comm_data.client_comm_size + client_rank], buffer, buff_size, 0);
                zmq_send (sobol_data_responder[group_id*comm_data.client_comm_size + client_rank], &comm_data.rank, sizeof(int), 0);
                buf_ptr = buffer;
                if (buffer[0] != time_step)
                    fprintf (stdout, "\n error \n\n");
                buf_ptr += sizeof(int);
                simu_id = *buf_ptr;
                fprintf(stderr, "                           data from simulation %d group %d (%d/%d)\n", simu_id, group_id, i, stats_options.nb_parameters+2);
                buf_ptr += 3 * sizeof(int) + MAX_FIELD_NAME * sizeof(char);
                memcpy(buff_tab_ptr[simu_id], buf_ptr, pull_data.buff_size * sizeof(double));
            }
            for (i=0; i<stats_options.nb_parameters+2; i++)
            {
                zmq_recv (sobol_data_responder2[group_id*comm_data.client_comm_size + client_rank], buffer, sizeof(int), 0);
                zmq_send (sobol_data_responder2[group_id*comm_data.client_comm_size + client_rank], &comm_data.rank, sizeof(int), 0);
                fprintf(stderr, "                           release simulation %d group %d\n", buffer[0], group_id);
                iteration++;
            }
            compute_stats (&data_ptr[client_rank], time_step-1, stats_options.nb_parameters+2, buff_tab_ptr);
            fprintf(stderr, "iteration %d / %d - field %s - process %d\n", iteration, nb_iterations, field_name_ptr, comm_data.rank);
        }

        if (end_signal != 0)
        {
            fprintf (stderr, "INTERUPTED\n");
            break;
        }

        if (nb_fields > 0)
        {
            if (iteration >= nb_iterations*nb_fields)
            {
                break;
            }
        }
    }

    zmq_close (connexion_responder);
    if (stats_options.sobol_op == 1)
    {
        for (i=0; i<stats_options.nb_groups; i++)
        {
            if (comm_data.rank == 0)
            {
                zmq_close (init_responder[i]);
                zmq_close (init_responder2[i]);
            }
            zmq_close (sobol_data_responder[i]);
            zmq_close (sobol_data_responder2[i]);
        }
    }
    else
    {
        if (comm_data.rank == 0)
        {
            zmq_close (init_responder[0]);
        }
    }

    zmq_close (data_puller);
    zmq_close (sobol_ready_responder);
    zmq_ctx_term (context);
    if (stats_options.sobol_op == 1)
    {
        if (comm_data.rank == 0)
        {
            free (init_responder);
            free (init_responder2);
        }
        free (sobol_data_responder);
        free (sobol_data_responder2);
        for (i=0; i<stats_options.nb_parameters+2; i++)
        {
            free (buff_tab_ptr[i]);
        }
    }
    free (buff_tab_ptr);

    if (comm_data.rank == 0)
    {
        free(node_names);
    }
    finalize_field_data (field, &comm_data, &pull_data, &stats_options, local_vect_sizes);
    free (buffer);
//    free (parameters);
    free (comm_data.rcounts);
    free (comm_data.rdispls);

#ifdef BUILD_WITH_PROBES
#ifdef BUILD_WITH_MPI
    double temp;
    MPI_Reduce (&total_comm_time, &temp, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    total_comm_time = temp / comm_data.comm_size;
    MPI_Reduce (&total_bytes_recv, &temp, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    total_bytes_recv = temp;
#endif // BUILD_WITH_MPI
    if (comm_data.rank==0)
    {
        fprintf (stdout, " --- Number of simulations:      %d\n", nb_simu);
        fprintf (stdout, " --- Number of simulation cores: %d\n", comm_data.client_comm_size);
        fprintf (stdout, " --- Number of analysis cores:   %d\n", comm_data.comm_size);
        fprintf (stdout, " --- Average communication time: %g s\n", total_comm_time);
        fprintf (stdout, " --- Calcul time:                %g s\n", total_computation_time);
        fprintf (stdout, " --- Waiting time:               %g s\n", total_wait_time);
        fprintf (stdout, " --- Writing time:               %g s\n", total_write_time);
        fprintf (stdout, " --- Total time:                 %g s\n", stats_get_time() - start_time);
        fprintf (stdout, " --- Bytes recieved:             %ld bytes\n",total_bytes_recv);
        fprintf (stdout, " --- Stats structures memory:    %ld bytes\n", mem_conso(&stats_options));
        fprintf (stdout, " --- Bytes written:              %ld bytes\n", count_bytes_written(&stats_options)*nb_fields);
    }
#endif // BUILD_WITH_PROBES

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
