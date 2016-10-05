/**
 *
 * @file stats_api.c
 * @brief API Functions.
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zmq.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#include "stats_api.h"
#endif // BUILD_WITH_MPI
#include "stats_api_no_mpi.h"

#ifndef MPI_MAX_PROCESSOR_NAME
#define MPI_MAX_PROCESSOR_NAME 256 /**< maximum size of processor names */
#endif

#ifndef MAX_FIELD_NAME
#define MAX_FIELD_NAME 128 /**< maximum size of field names */
#endif

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * @struct zmq_data_s
 *
 * Structure containing some data needed by zmq
 *
 *******************************************************************************/

struct zmq_data_s
{
    void  *context;                           /**< ZeroMQ context                                             */
    void  *connexion_requester;               /**< connexion ZeroMQ port                                      */
    void  *init_requester;                    /**< initialization ZeroMQ port                                 */
    void  *init_requester2;                   /**< initialization ZeroMQ port (Sobol only)                    */
    void **data_pusher;                       /**< push data ZeroMQ ports                                     */
    void **sobol_init_requester;              /**< initialization ZeroMQ port                                 */
    void **sobol_requester;                   /**< data ZeroMQ Sobol port                                     */
    void **sobol_release_requester;           /**< release ZeroMQ Sobol port                                  */
    char   port_name[MPI_MAX_PROCESSOR_NAME]; /**< name of the third ZeroMQ port                              */
    int    rinit_tab[2];                      /**< array used to receive data                                 */
    int    sinit_tab[2];                      /**< array used to send data                                    */
    int    nb_proc_server;                    /**< number of MPI processes of the library                     */
    int   *server_vect_size;                  /**< local vect size for the library                            */
    char  *buffer;                            /**< buffer used to send data to the library                    */
    int    buff_size;                         /**< size of this buffer                                        */
    int   *send_counts;                       /**< number of elements to send to server rank i                */
    int   *sdispls;                           /**< displacement to which data should be sent to server rank i */
    int   *pull_rank;                         /**< rank of the pulling process for the message i              */
    int   *push_rank;                         /**< rank of the pushing process for the message i              */
    int   *message_sizes;                     /**< size of the message i                                      */
    int    total_nb_messages;                 /**< total number of messages                                   */
    int    local_nb_messages;                 /**< local number of messages                                   */
};

typedef struct zmq_data_s zmq_data_t; /**< type corresponding to zmq_data_s */

static zmq_data_t zmq_data;

#ifdef BUILD_WITH_PROBES
static double total_comm_time = 0;
static double start_comm_time;
static double end_comm_time;
static int total_bytes_sent = 0;
#endif // BUILD_WITH_PROBES

static double stats_get_time ()
{
#ifdef BUILD_WITH_MPI
    return MPI_Wtime();
#else // BUILD_WITH_MPI
    return (double)time(NULL);
#endif // BUILD_WITH_MPI
}

static inline void comm_1_to_m_init (zmq_data_t *data)
{
    int  i;

    data->sdispls[0] = 0;
    for (i=0; i<data->nb_proc_server-1; i++)
    {
        data->send_counts[i] = data->server_vect_size[i];
        data->sdispls[i+1] = data->sdispls[i] + data->send_counts[i];
    }
    data->send_counts[data->nb_proc_server] = data->server_vect_size[data->nb_proc_server];

    data->total_nb_messages = data->nb_proc_server;
    data->local_nb_messages = data->nb_proc_server;
}

static inline void comm_n_to_m_init (zmq_data_t *data,
                                     const int   vect_size,
                                     int        *my_vect_size,
                                     const int   rank)
{
    int  i;
    int  client_rank  = 0;
    int  client_count = 0;
    int  server_rank  = 0;
    int  server_count = 0;
    int  new_message = 0;
    int *server_vect_size = data->server_vect_size;
    int  nb_proc_server = data->nb_proc_server;
    int  nb_messages = 0;
    int  nb_elem_message;

    data->total_nb_messages = 1;
    data->local_nb_messages = 0;
    data->buff_size = 0;

    if (rank == 0)
    {
        data->local_nb_messages = 1;
    }

    for (i=0; i<vect_size; i++)
    {
        if (client_count < my_vect_size[client_rank])
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

        if (client_rank == rank)
        {
            data->send_counts[server_rank] += 1;
        }

        if (new_message == 1)
        {
            data->total_nb_messages += 1;
            if (client_rank == rank)
            {
                data->local_nb_messages += 1;
            }
            new_message = 0;
        }
    }

    data->sdispls[0] = 0;
    for (i=0; i<nb_proc_server-1; i++)
    {
        data->sdispls[i+1] = data->sdispls[i] + data->send_counts[i];
    }

    new_message = 0;

    data->push_rank = malloc (data->total_nb_messages * sizeof(int));
    data->pull_rank = malloc (data->total_nb_messages * sizeof(int));
    data->message_sizes = malloc (data->total_nb_messages * sizeof(int));

    nb_messages = 0;
    nb_elem_message = 0;
    data->push_rank[0] = 0;
    data->pull_rank[0] = 0;
    client_rank  = 0;
    client_count = 0;
    server_rank  = 0;
    server_count = 0;
    for (i=0; i<vect_size; i++)
    {
        if (client_count < my_vect_size[client_rank])
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
            data->push_rank[nb_messages] = client_rank;
            data->pull_rank[nb_messages] = server_rank;
            data->message_sizes[nb_messages - 1] = nb_elem_message;
            new_message = 0;
            if (nb_elem_message > data->buff_size)
            {
                data->buff_size = nb_elem_message;
            }
            nb_elem_message = 0;
        }
        nb_elem_message += 1;
    }
    if (data->total_nb_messages == 1)
    {
        data->buff_size = nb_elem_message;
    }

    data->message_sizes [data->total_nb_messages - 1 ] = nb_elem_message;
}

#ifdef BUILD_WITH_MPI
/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function initialise connexion with the stats library
 *
 *******************************************************************************
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *global_vect_size
 * sise of the global data vector to send to the library
 *
 * @param[in] *rank
 * MPI rank
 *
 * @param[in] *comm
 * MPI communicator
 *
 *******************************************************************************/

void connect_to_stats (const int *local_vect_size,
                       const int *comm_size,
                       const int *rank,
                       MPI_Comm  *comm)
{
    char  *node_names;
    char   server_node_name[MPI_MAX_PROCESSOR_NAME];
    int    server_name_size;
    int    port_no, i, j;
    FILE*  file = NULL;
    int   *my_vect_size;
    int    global_vect_size = 0;
    int    nb_bufferized_messages = 50;
    int    linger = -1;

    zmq_data.context = zmq_ctx_new ();
    zmq_data.connexion_requester = zmq_socket (zmq_data.context, ZMQ_REQ);
    zmq_data.init_requester = zmq_socket (zmq_data.context, ZMQ_REQ);

    if (*rank == 0)
    {
        file = fopen("../../DATA/server_name.txt", "r");

        if (file == NULL)
        {
            file = fopen("server_name.txt", "r");

            if (file == NULL)
            {
              strcpy (server_node_name, "localhost");
              fprintf(stdout,"WARNING: Server name set to \"localhost\"\n");
            }
        }

        if (file != NULL)
        {
            fgets(server_node_name, MPI_MAX_PROCESSOR_NAME, file);
            fclose(file);
        }

        zmq_data.sinit_tab[0] = *comm_size;
        zmq_data.sinit_tab[1] = 1;

        sprintf (zmq_data.port_name, "tcp://%s:20000", server_node_name);
        zmq_connect (zmq_data.connexion_requester, zmq_data.port_name);
        zmq_send (zmq_data.connexion_requester, zmq_data.sinit_tab, 2 * sizeof(int), 0);
        zmq_recv (zmq_data.connexion_requester, zmq_data.rinit_tab, 2 * sizeof(int), 0);
    }

    my_vect_size = malloc(*comm_size * sizeof(int));

    if (*comm_size > 1)
    {
        MPI_Bcast(zmq_data.rinit_tab, 2, MPI_INT, 0, *comm);
        i = *local_vect_size;
        MPI_Allgather(&i, 1, MPI_INT, my_vect_size, 1, MPI_INT, *comm);
        for (i=0; i<*comm_size; i++)
        {
            global_vect_size += my_vect_size[i];
        }
    }
    else
    {
        my_vect_size[0]  = *local_vect_size;
        global_vect_size = *local_vect_size;
    }

    zmq_data.nb_proc_server = zmq_data.rinit_tab[0];
    server_name_size = zmq_data.rinit_tab[1];

    zmq_data.server_vect_size = calloc (zmq_data.nb_proc_server, sizeof(int));

    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_data.server_vect_size[i] = global_vect_size / zmq_data.nb_proc_server;
        if (i < global_vect_size % zmq_data.nb_proc_server)
            zmq_data.server_vect_size[i] += 1;
    }

    zmq_data.send_counts = calloc (zmq_data.nb_proc_server, sizeof(int));
    zmq_data.sdispls     = calloc (zmq_data.nb_proc_server, sizeof(int));
    comm_n_to_m_init (&zmq_data, global_vect_size, my_vect_size, *rank);

    node_names = malloc (zmq_data.nb_proc_server * server_name_size * sizeof(char));

    if (*rank == 0)
    {
        sprintf (zmq_data.port_name, "tcp://%s:21000", server_node_name);
        zmq_connect (zmq_data.init_requester, zmq_data.port_name);
        zmq_send (zmq_data.init_requester, my_vect_size, *comm_size * sizeof(int), 0);
        zmq_recv (zmq_data.init_requester, node_names, zmq_data.rinit_tab[0] * zmq_data.rinit_tab[1] * sizeof(char), 0);
    }

    if (*comm_size > 1)
    {
        MPI_Bcast (node_names, zmq_data.nb_proc_server * server_name_size, MPI_CHAR, 0, *comm);
    }

    zmq_data.data_pusher = malloc (zmq_data.local_nb_messages * sizeof(void*));

    j = 0;
    for (i=0; i<zmq_data.total_nb_messages; i++)
    {
        if (*rank == zmq_data.push_rank[i])
        {
            zmq_data.data_pusher[j] = zmq_socket (zmq_data.context, ZMQ_PUSH);
            zmq_setsockopt (zmq_data.data_pusher[j], ZMQ_SNDHWM, &nb_bufferized_messages, sizeof(int));
            zmq_setsockopt (zmq_data.data_pusher[j], ZMQ_LINGER, &linger, sizeof(int));
            port_no = 32123 + zmq_data.pull_rank[i];
            sprintf (zmq_data.port_name, "tcp://%s:%d", &node_names[server_name_size * zmq_data.pull_rank[i]], port_no);
            zmq_connect (zmq_data.data_pusher[j], zmq_data.port_name);
            j += 1;
        }
    }

    zmq_data.buff_size *= sizeof(double);
    zmq_data.buff_size += 2 * sizeof(int) + MAX_FIELD_NAME * sizeof(char);
    zmq_data.buffer   = malloc (zmq_data.buff_size);

    free (my_vect_size);
    free (node_names);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function initialise connexion with the stats library for Sobol indices
 *
 *******************************************************************************
 *
 * @param[in] *nb_parameters
 * number of variable parameters of the parametric study
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *global_vect_size
 * sise of the global data vector to send to the library
 *
 * @param[in] *comm
 * MPI communicator
 *
 *******************************************************************************/

void connect_to_stats_sobol (const int *local_vect_size,
                             const int *comm_size,
                             const int *rank,
                             const int *sobol_rank,
                             const int *sobol_group,
                             MPI_Comm  *comm)
{
    char  *node_names;
    char   server_node_name[MPI_MAX_PROCESSOR_NAME];
    int    server_name_size;
    int    port_no, i, j;
    FILE*  file = NULL;
    int   *my_vect_size;
    int    global_vect_size = 0;
    int    linger = -1;

    zmq_data.context = zmq_ctx_new ();
    zmq_data.connexion_requester = zmq_socket (zmq_data.context, ZMQ_REQ);
    zmq_data.init_requester = zmq_socket (zmq_data.context, ZMQ_REQ);
    zmq_data.init_requester2 = zmq_socket (zmq_data.context, ZMQ_REQ);

    if (*rank == 0)
    {
        file = fopen("../../DATA/server_name.txt", "r");

        if (file == NULL)
        {
            file = fopen("server_name.txt", "r");

            if (file == NULL)
            {
              strcpy (server_node_name, "localhost");
              fprintf(stdout,"WARNING: Server name set to \"localhost\"\n");
            }
        }

        if (file != NULL)
        {
            fgets(server_node_name, MPI_MAX_PROCESSOR_NAME, file);
            fclose(file);
        }

        zmq_data.sinit_tab[0] = *comm_size;
        zmq_data.sinit_tab[1] = *sobol_group;

        if (*sobol_rank == 0)
        {
            sprintf (zmq_data.port_name, "tcp://%s:20000", server_node_name);
            zmq_connect (zmq_data.connexion_requester, zmq_data.port_name);
            zmq_send (zmq_data.connexion_requester, zmq_data.sinit_tab, 2 * sizeof(int), 0);
            zmq_recv (zmq_data.connexion_requester, zmq_data.rinit_tab, 2 * sizeof(int), 0);
        }
        if (*sobol_group<10)
        {
          sprintf (zmq_data.port_name, "tcp://%s:210%d", server_node_name, *sobol_group);
        }
        else
        {
            sprintf (zmq_data.port_name, "tcp://%s:21%d", server_node_name, *sobol_group);
        }
        zmq_connect (zmq_data.init_requester, zmq_data.port_name);
        zmq_send (zmq_data.init_requester, zmq_data.sinit_tab, 2 * sizeof(int), 0);
        zmq_recv (zmq_data.init_requester, zmq_data.rinit_tab, 2 * sizeof(int), 0);
        if (*sobol_group<10)
        {
          sprintf (zmq_data.port_name, "tcp://%s:220%d", server_node_name, *sobol_group);
        }
        else
        {
            sprintf (zmq_data.port_name, "tcp://%s:22%d", server_node_name, *sobol_group);
        }
        zmq_connect (zmq_data.init_requester2, zmq_data.port_name);
    }

    my_vect_size = malloc(*comm_size * sizeof(int));

    if (*comm_size > 1)
    {
        MPI_Bcast(zmq_data.rinit_tab, 2, MPI_INT, 0, *comm);
        i = *local_vect_size;
        MPI_Allgather(&i, 1, MPI_INT, my_vect_size, 1, MPI_INT, *comm);
        for (i=0; i<*comm_size; i++)
        {
            global_vect_size += my_vect_size[i];
        }
    }
    else
    {
        my_vect_size[0]  = *local_vect_size;
        global_vect_size = *local_vect_size;
    }

    zmq_data.nb_proc_server = zmq_data.rinit_tab[0];
    server_name_size = zmq_data.rinit_tab[1];

    zmq_data.server_vect_size = calloc (zmq_data.nb_proc_server, sizeof(int));

    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_data.server_vect_size[i] = global_vect_size / zmq_data.nb_proc_server;
        if (i < global_vect_size % zmq_data.nb_proc_server)
            zmq_data.server_vect_size[i] += 1;
    }

    zmq_data.send_counts = calloc (zmq_data.nb_proc_server, sizeof(int));
    zmq_data.sdispls     = calloc (zmq_data.nb_proc_server, sizeof(int));

    comm_n_to_m_init (&zmq_data, global_vect_size, my_vect_size, *rank);

    node_names = malloc (zmq_data.nb_proc_server * server_name_size * sizeof(char));

    if (*rank == 0)
    {
        zmq_send (zmq_data.init_requester2, my_vect_size, *comm_size * sizeof(int), 0);
        zmq_recv (zmq_data.init_requester2, node_names, zmq_data.rinit_tab[0] * zmq_data.rinit_tab[1] * sizeof(char), 0);
    }

    if (*comm_size > 1)
    {
        MPI_Bcast (node_names, zmq_data.nb_proc_server * server_name_size, MPI_CHAR, 0, *comm);
    }

    if (*sobol_rank == 0)
    {
        zmq_data.sobol_init_requester = malloc (zmq_data.local_nb_messages * sizeof(void*));
    }
    zmq_data.sobol_requester = malloc (zmq_data.local_nb_messages * sizeof(void*));
    zmq_data.sobol_release_requester = malloc (zmq_data.local_nb_messages * sizeof(void*));

    j = 0;
    for (i=0; i<zmq_data.total_nb_messages; i++)
    {
        if (*rank == zmq_data.push_rank[i] && *sobol_rank == 0)
        {
            zmq_data.sobol_init_requester[j] = zmq_socket (zmq_data.context, ZMQ_REQ);
            zmq_setsockopt (zmq_data.sobol_init_requester[j], ZMQ_LINGER, &linger, sizeof(int));
            port_no = 123 + zmq_data.pull_rank[i];
            sprintf (zmq_data.port_name, "tcp://%s:10%d", &node_names[server_name_size * zmq_data.pull_rank[i]], port_no);
            zmq_connect (zmq_data.sobol_init_requester[j], zmq_data.port_name);
            fprintf(stderr,"connect to 10%d\n", port_no);
            j += 1;
        }
    }

    j = 0;
    for (i=0; i<zmq_data.total_nb_messages; i++)
    {
        if (*rank == zmq_data.push_rank[i])
        {
            zmq_data.sobol_requester[j] = zmq_socket (zmq_data.context, ZMQ_REQ);
            zmq_setsockopt (zmq_data.sobol_requester[j], ZMQ_LINGER, &linger, sizeof(int));
            port_no = 12 + zmq_data.pull_rank[i];
            sprintf (zmq_data.port_name,
                     "tcp://%s:20%d%d",
                     &node_names[server_name_size * zmq_data.pull_rank[i]],
                     port_no,
                     *comm_size * *sobol_group + *rank);
            zmq_connect (zmq_data.sobol_requester[j], zmq_data.port_name);
            fprintf(stderr,"connect to 20%d%d\n", port_no, *comm_size * *sobol_group + *rank);
            j += 1;
        }
    }

    j = 0;
    for (i=0; i<zmq_data.total_nb_messages; i++)
    {
        if (*rank == zmq_data.push_rank[i])
        {
            zmq_data.sobol_release_requester[j] = zmq_socket (zmq_data.context, ZMQ_REQ);
            zmq_setsockopt (zmq_data.sobol_release_requester[j], ZMQ_LINGER, &linger, sizeof(int));
            port_no = 12 + zmq_data.pull_rank[i];
            sprintf (zmq_data.port_name,
                     "tcp://%s:30%d%d",
                     &node_names[server_name_size * zmq_data.pull_rank[i]],
                     port_no,
                     *comm_size * *sobol_group + *rank);
            zmq_connect (zmq_data.sobol_release_requester[j], zmq_data.port_name);
            fprintf(stderr,"connect to 30%d%d\n", port_no, *comm_size * *sobol_group + *rank);
            j += 1;
        }
    }

    zmq_data.buff_size *= sizeof(double);
    zmq_data.buff_size += 4 * sizeof(int) + MAX_FIELD_NAME * sizeof(char);
    zmq_data.buffer = malloc (zmq_data.buff_size);

    free (my_vect_size);
    free (node_names);
}
#endif // BUILD_WITH_MPI

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function initialise connexion with the stats library
 *
 *******************************************************************************
 *
 * @param[in] *vect_size
 * sise of the local data vector to send to the library
 *
 *******************************************************************************/

void connect_to_stats_no_mpi (int *vect_size)
{
    char *node_names;
    char  server_node_name[MPI_MAX_PROCESSOR_NAME];
    int   server_name_size;
    int   port_no, i;
    FILE* file = NULL;
    int   my_vect_size[1];
    int   nb_bufferized_messages = 50;

    zmq_data.context = zmq_ctx_new ();
    zmq_data.connexion_requester = zmq_socket (zmq_data.context, ZMQ_REQ);
    zmq_data.init_requester = zmq_socket (zmq_data.context, ZMQ_REQ);

    file = fopen("server_name.txt", "r");

    if (file != NULL)
    {
        fgets(server_node_name, MPI_MAX_PROCESSOR_NAME, file);

        fclose(file);
    }
    else
    {
        strcpy (server_node_name, "localhost");
        fprintf(stdout,"WARNING: Server name set to \"localhost\"\n");
    }

    zmq_data.sinit_tab[0] = 1;
    zmq_data.sinit_tab[1] = 1;

    sprintf (zmq_data.port_name, "tcp://%s:20000", server_node_name);
    zmq_connect (zmq_data.connexion_requester, zmq_data.port_name);
    zmq_send (zmq_data.connexion_requester, zmq_data.sinit_tab, 2 * sizeof(int), 0);
    zmq_recv (zmq_data.connexion_requester, zmq_data.rinit_tab, 2 * sizeof(int), 0);

    my_vect_size[0]  = *vect_size;
    zmq_data.nb_proc_server = zmq_data.rinit_tab[0];
    server_name_size = zmq_data.rinit_tab[1];

    zmq_data.server_vect_size = calloc (zmq_data.nb_proc_server, sizeof(int));

    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_data.server_vect_size[i] = *vect_size / zmq_data.nb_proc_server;
        if (i < *vect_size % zmq_data.nb_proc_server)
            zmq_data.server_vect_size[i] += 1;
    }

    zmq_data.send_counts = calloc (zmq_data.nb_proc_server, sizeof(int));
    zmq_data.sdispls     = calloc (zmq_data.nb_proc_server, sizeof(int));
    comm_1_to_m_init (&zmq_data);

    node_names = malloc (zmq_data.nb_proc_server * server_name_size * sizeof(char));

    sprintf (zmq_data.port_name, "tcp://%s:21000", server_node_name);
    zmq_connect (zmq_data.init_requester, zmq_data.port_name);
    zmq_send (zmq_data.init_requester, my_vect_size, sizeof(int), 0);
    zmq_recv (zmq_data.init_requester, node_names, zmq_data.rinit_tab[0] * zmq_data.rinit_tab[1] * sizeof(char), 0);

    zmq_data.data_pusher = malloc (zmq_data.local_nb_messages * sizeof(void*));
    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_data.data_pusher[i] = zmq_socket (zmq_data.context, ZMQ_PUSH);
        port_no = 32123 + i;
        zmq_setsockopt (zmq_data.data_pusher[i], ZMQ_SNDHWM, &nb_bufferized_messages, sizeof(int));
        sprintf (zmq_data.port_name, "tcp://%s:%d", &node_names[server_name_size * i], port_no);
        zmq_connect (zmq_data.data_pusher[i], zmq_data.port_name);
    }

    zmq_data.buff_size = 2 * sizeof(int) + zmq_data.server_vect_size[0] * sizeof(double);
    zmq_data.buffer   = calloc (zmq_data.buff_size, sizeof (char));

    free (node_names);
}

#ifdef BUILD_WITH_MPI
/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * Fortran wrapper for connect_to_stats
 *
 *******************************************************************************
 *
 * @param[in] *nb_parameters
 * number of variable parameters of the parametric study
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *global_vect_size
 * sise of the global data vector to send to the library
 *
 * @param[in] *comm_fortran
 * Fortran MPI communicator
 *
 *******************************************************************************/

void connect_from_fortran (int       *local_vect_size,
                           int       *comm_size,
                           int       *rank,
                           MPI_Fint  *comm_fortran)
{
    MPI_Comm comm = MPI_Comm_f2c(*comm_fortran);
    connect_to_stats(local_vect_size, comm_size, rank, &comm);

}
#endif // BUILD_WITH_MPI

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function sends data to the stats library
 *
 *******************************************************************************
 *
 * @param[in] time_step
 * current time step of the simulation
 *
 * @param[in] *parameters
 * array of parameters
 *
 * @param[in] *nb_parameters
 * size of *parameters
 *
 * @param[in] *send_vect
 * local data array to send to the statistic library
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *comm
 * MPI communicator
 *
 *******************************************************************************/

void send_to_stats (const int  *time_step,
                    const char *field_name,
                    double     *send_vect,
                    const int  *rank)
{
    int   i, j;
    char *buff_ptr;

    buff_ptr = zmq_data.buffer;
    memcpy(buff_ptr, time_step, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy(buff_ptr, rank, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, field_name, MAX_FIELD_NAME);
    buff_ptr += MAX_FIELD_NAME * sizeof(char);

#ifdef BUILD_WITH_PROBES
    start_comm_time = stats_get_time();
#endif // BUILD_WITH_PROBES
    j = 0;
    for (i=0; i<zmq_data.total_nb_messages; i++)
    {
        if (*rank == zmq_data.push_rank[i])
        {
            memcpy (buff_ptr, &send_vect[zmq_data.sdispls[zmq_data.pull_rank[i]]], zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double));
            zmq_send (zmq_data.data_pusher[j], zmq_data.buffer, zmq_data.buff_size, 0);
            j += 1;
#ifdef BUILD_WITH_PROBES
            total_bytes_sent += zmq_data.buff_size;
#endif // BUILD_WITH_PROBES
        }
    }
#ifdef BUILD_WITH_PROBES
    end_comm_time = stats_get_time();
    total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
}

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function sends data to the stats library
 *
 *******************************************************************************
 *
 * @param[in] time_step
 * current time step of the simulation
 *
 * @param[in] *parameters
 * array of parameters
 *
 * @param[in] *nb_parameters
 * size of *parameters
 *
 * @param[in] *send_vect
 * local data array to send to the statistic library
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *comm
 * MPI communicator
 *
 *******************************************************************************/

void send_to_stats_sobol (const int  *time_step,
                          const char *field_name,
                          double     *send_vect,
                          const int  *rank,
                          const int  *sobol_rank,
                          const int  *sobol_group)
{
    int   i=0, j, k;
    char *buff_ptr;
    double temp=0;

    buff_ptr = zmq_data.buffer;
    memcpy(buff_ptr, time_step, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy(buff_ptr, sobol_rank, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy(buff_ptr, sobol_group, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy(buff_ptr, rank, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, field_name, MAX_FIELD_NAME * sizeof(char));
    buff_ptr += MAX_FIELD_NAME * sizeof(char);

#ifdef BUILD_WITH_PROBES
    start_comm_time = stats_get_time();
#endif // BUILD_WITH_PROBES

    j = 0;
    for (i=0; i<zmq_data.total_nb_messages; i++)
    {
        if (*rank == zmq_data.push_rank[i])
        {
            memcpy (buff_ptr, &send_vect[zmq_data.sdispls[zmq_data.pull_rank[i]]], zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double));
            if (*sobol_rank == 0)
            {
                fprintf(stderr, "send from %d %d\n", *sobol_rank, *sobol_group);
                zmq_send (zmq_data.sobol_init_requester[j], zmq_data.buffer, zmq_data.buff_size, 0);
                fprintf(stderr, "wait from %d %d\n", *sobol_rank, *sobol_group);
                zmq_recv (zmq_data.sobol_init_requester[j], &k, sizeof(int), 0);
                fprintf(stderr, "recv (%d) from %d %d\n", k, *sobol_rank, *sobol_group);
            }
            if (*sobol_rank != 0)
            {
                fprintf(stderr, "send data from %d %d\n", *sobol_rank, *sobol_group);
                zmq_send (zmq_data.sobol_requester[j], zmq_data.buffer, zmq_data.buff_size, 0);
                fprintf(stderr, "wait (%d %d)\n", *sobol_rank, *sobol_group);
                zmq_recv (zmq_data.sobol_requester[j], &k, sizeof(int), 0);
                fprintf(stderr, "recv (%d %d)\n", *sobol_rank, *sobol_group);
#ifdef BUILD_WITH_PROBES
                total_bytes_sent += zmq_data.buff_size;
#endif // BUILD_WITH_PROBES
            }
            fprintf(stderr, "send release request from %d %d\n", *sobol_rank, *sobol_group);
            zmq_send (zmq_data.sobol_release_requester[j], sobol_rank, sizeof(int), 0);
            fprintf(stderr, "wait release (%d %d)\n", *sobol_rank, *sobol_group);
            zmq_recv (zmq_data.sobol_release_requester[j], &k, sizeof(int), 0);
            fprintf(stderr, "%d %d released\n", *sobol_rank, *sobol_group);
            j += 1;
        }
    }
#ifdef BUILD_WITH_PROBES
    end_comm_time = stats_get_time();
    total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
}

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function sends data to the stats library
 *
 *******************************************************************************
 *
 * @param[in] time_step
 * current time step of the simulation
 *
 * @param[in] *parameters
 * array of parameters
 *
 * @param[in] *nb_parameters
 * size of *parameters
 *
 * @param[in] *send_vect
 * local data array to send to the statistic library
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 *******************************************************************************/

void send_to_stats_no_mpi (const int  *time_step,
                           const char *field_name,
                           double     *send_vect)
{
    int rank = 0;
    send_to_stats (time_step,
                   field_name,
                   send_vect,
                   &rank);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function disconects the simulation from the statistic library
 *
 *******************************************************************************/

void disconnect_from_stats ()
{
    int i;
    zmq_close (zmq_data.connexion_requester);
    zmq_close (zmq_data.init_requester);
    for (i=0; i<zmq_data.local_nb_messages; i++)
    {
        zmq_close (zmq_data.data_pusher[i]);
    }
    zmq_ctx_destroy (zmq_data.context);
    free(zmq_data.data_pusher);
    free(zmq_data.send_counts);
    free(zmq_data.sdispls);
    free(zmq_data.server_vect_size);
    free(zmq_data.buffer);

#ifdef BUILD_WITH_PROBES
    fprintf (stdout, " --- Simulation comm time: %g s\n",total_comm_time);
    fprintf (stdout, " --- Bytes sent: %d bytes\n",total_bytes_sent);
#endif // BUILD_WITH_PROBES
}

/**
 *******************************************************************************
 *
 * @ingroup stats_api
 *
 * This function disconects the simulation from the statistic library
 *
 *******************************************************************************/

void disconnect_from_stats_sobol (int *sobol_rank)
{
    int i;
    zmq_close (zmq_data.connexion_requester);
    zmq_close (zmq_data.init_requester);
    zmq_close (zmq_data.init_requester2);
    if (*sobol_rank == 0)
    {
        for (i=0; i<zmq_data.local_nb_messages; i++)
        {
            zmq_close (zmq_data.sobol_init_requester[i]);
        }
    }
    for (i=0; i<zmq_data.local_nb_messages; i++)
    {
        zmq_close (zmq_data.sobol_requester[i]);
        zmq_close (zmq_data.sobol_release_requester[i]);
    }
    zmq_ctx_destroy (zmq_data.context);
    if (*sobol_rank == 0)
    {
        free(zmq_data.sobol_init_requester);
    }
    free(zmq_data.sobol_requester);
    free(zmq_data.send_counts);
    free(zmq_data.sdispls);
    free(zmq_data.server_vect_size);
    free(zmq_data.buffer);

#ifdef BUILD_WITH_PROBES
    fprintf (stdout, " --- Simulation comm time: %g s\n",total_comm_time);
    fprintf (stdout, " --- Bytes sent: %d bytes\n",total_bytes_sent);
#endif // BUILD_WITH_PROBES
}
