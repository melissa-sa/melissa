/**
 *
 * @file melissa_api.c
 * @brief API Functions.
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zmq.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#include "melissa_api.h"
#endif // BUILD_WITH_MPI
#include "melissa_api_no_mpi.h"
#include "melissa_utils.h"

#ifndef MPI_MAX_PROCESSOR_NAME
#define MPI_MAX_PROCESSOR_NAME 256 /**< maximum size of processor names */
#endif

#ifndef MAX_FIELD_NAME
#define MAX_FIELD_NAME 128 /**< maximum size of field names */
#endif

#ifndef BUILD_WITH_MPI
typedef int MPI_Comm; /**< Convert MPI_Comm to int when built without MPI */
#endif // BUILD_WITH_MPI

#define ZEROCOPY /**< Use ZMQ zero copy messages */

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * @struct zmq_data_s
 *
 * Structure containing some data needed by zmq
 *
 *******************************************************************************/

struct zmq_data_s
{
    void    *context;                           /**< ZeroMQ context                                             */
    void    *connexion_requester;               /**< connexion ZeroMQ port                                      */
    void    *init_requester;                    /**< initialization ZeroMQ port                                 */
    void   **data_pusher;                       /**< push data ZeroMQ ports                                     */
    void   **sobol_requester;                   /**< data ZeroMQ Sobol port                                     */
    int      rinit_tab[3];                      /**< array used to receive data                                 */
    int      sobol;                             /**< 1 if sobol computation, 0 otherwhise                       */
    int      sobol_rank;                        /**< sobol rank                                                 */
    int      sinit_tab[2];                      /**< array used to send data                                    */
    int      nb_proc_server;                    /**< number of MPI processes of the library                     */
    int      nb_parameters;                     /**< number of parameters of the study                          */
    int     *server_vect_size;                  /**< local vect size for the library                            */
    char    *buffer;                            /**< buffer used to send data to the library                    */
    int      buff_size;                         /**< size of this buffer                                        */
    int      send_buff_size;                    /**< size of send buffer                                        */
    double  *buffer_sobol;                      /**< buffer used to store data on sobol rank 0                  */
    int     *send_counts;                       /**< number of elements to send to server rank i                */
    int     *local_vect_sizes;                  /**< local vector size                                          */
    int     *sdispls;                           /**< displacement to which data should be sent to server rank i */
    int     *pull_rank;                         /**< rank of the pulling process for the message i              */
    int     *push_rank;                         /**< rank of the pushing process for the message i              */
    int     *message_sizes;                     /**< size of the message i                                      */
    int      total_nb_messages;                 /**< total number of messages                                   */
    int      local_nb_messages;                 /**< local number of messages                                   */
    int      coupling;                          /**< coupled simulations or not                                 */
    MPI_Comm comm_sobol;                        /**< inter-groups communicator                                  */
};

typedef struct zmq_data_s zmq_data_t; /**< type corresponding to zmq_data_s */

static zmq_data_t zmq_data;

#ifdef BUILD_WITH_PROBES
static double total_comm_time;
static double start_comm_time;
static double end_comm_time;
static long int total_bytes_sent;
#endif // BUILD_WITH_PROBES

static void my_free (void *data, void *hint)
{
    free (data);
}


static void print_zmq_error(int ret)
{
    if (ret == EAGAIN)
    {
        fprintf(stderr, "Non-blocking mode was requested and the message cannot be sent at the moment.\n");
    }
    else if (ret == ENOTSUP)
    {
        fprintf(stderr, "The zmq_send() operation is not supported by this socket type\n");
    }
    else if (ret == EFSM)
    {
        fprintf(stderr, "The zmq_send() operation cannot be performed on this socket at the moment due to the socket not being in the appropriate state. This error may occur with socket types that switch between several states, such as ZMQ_REP. See the messaging patterns section of zmq_socket(3) for more information\n");
    }
    else if (ret == ETERM)
    {
        fprintf(stderr, "The ZeroMQ context associated with the specified socket was terminated.\n");
    }
    else if (ret == ENOTSOCK)
    {
        fprintf(stderr, "The provided socket was invalid.\n");
    }
    else if (ret == EINTR)
    {
        fprintf(stderr, "The operation was interrupted by delivery of a signal before the message was sent.\n");
    }
    else if (ret == EHOSTUNREACH)
    {
        fprintf(stderr, "The message cannot be routed.\n");
    }
    else
    {
        fprintf(stderr, "Unknown error.\n");
    }
    exit(0);
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
    data->send_counts[data->nb_proc_server-1] = data->server_vect_size[data->nb_proc_server-1];

    data->total_nb_messages = data->nb_proc_server;
    data->local_nb_messages = data->nb_proc_server;
}

static inline void comm_n_to_m_init (zmq_data_t *data,
                                     const int   vect_size,
                                     int        *local_vect_sizes,
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
    int  nb_elem_message = 0;

    data->total_nb_messages = 1;
    data->local_nb_messages = 0;
    data->buff_size = 0;

    if (rank == 0)
    {
        data->local_nb_messages = 1;
    }

    for (i=0; i<vect_size; i++)
    {
        if (client_count < local_vect_sizes[client_rank])
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

    data->push_rank[0] = 0;
    data->pull_rank[0] = 0;
    client_rank  = 0;
    client_count = 0;
    server_rank  = 0;
    server_count = 0;
    for (i=0; i<vect_size; i++)
    {
        if (client_count < local_vect_sizes[client_rank])
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
    data->message_sizes [data->total_nb_messages - 1 ] = nb_elem_message;
    if (nb_elem_message > data->buff_size)
    {
        data->buff_size = nb_elem_message;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * This function initialise connexion with the stats library
 *
 *******************************************************************************
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *comm_size
 * sise of the MPI communicator comm
 *
 * @param[in] *rank
 * MPI rank
 *
 * @param[in] *sobol_rank
 * Sobol indice rank in Sobol group
 *
 * @param[in] *sample_id
 * ID of the parameter set defining the simulation
 *
 * @param[in] *comm
 * MPI communicator
 *
 * @param[in] *coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init (const int *local_vect_size,
                   const int *comm_size,
                   const int *rank,
                   const int *sobol_rank,
                   const int *sample_id,
                   MPI_Comm  *comm,
                   const int *coupling)
{
    char  *node_names = NULL;
    char   server_node_name[MPI_MAX_PROCESSOR_NAME];
    char   port_name[MPI_MAX_PROCESSOR_NAME] = {0};
    int    port_no, i, j;
    FILE*  file = NULL;
    int    global_vect_size = 0;
    int    linger = -1;
    char   master_node_name[MPI_MAX_PROCESSOR_NAME];
    char  *master_node_names = NULL;
    void  *master_requester = NULL;

    zmq_data.context = zmq_ctx_new ();
    zmq_data.connexion_requester = zmq_socket (zmq_data.context, ZMQ_REQ);
    zmq_data.init_requester = zmq_socket (zmq_data.context, ZMQ_REQ);
    zmq_data.sobol_requester = NULL;
    zmq_data.sobol_rank = *sobol_rank;
    zmq_data.coupling = *coupling;
    total_comm_time = 0.0;
    total_bytes_sent = 0;

    // get main server node name
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
        zmq_data.sinit_tab[1] = *sample_id;
        zmq_data.rinit_tab[0] = 0;
        zmq_data.rinit_tab[1] = 1;

        sprintf (port_name, "tcp://%s:2003", server_node_name);
        melissa_connect (zmq_data.connexion_requester, port_name);
        zmq_send (zmq_data.connexion_requester, zmq_data.sinit_tab, 2 * sizeof(int), 0);
        zmq_recv (zmq_data.connexion_requester, zmq_data.rinit_tab, 3 * sizeof(int), 0);
        sprintf (port_name, "tcp://%s:2002", server_node_name);
        melissa_connect (zmq_data.init_requester, port_name);
    }

    // bcast infos from server
    zmq_data.local_vect_sizes = malloc(*comm_size * sizeof(int));

#ifdef BUILD_WITH_MPI
    if (*comm_size > 1)
    {
        MPI_Bcast(zmq_data.rinit_tab, 3, MPI_INT, 0, *comm);
        i = *local_vect_size;
        MPI_Allgather(&i, 1, MPI_INT, zmq_data.local_vect_sizes, 1, MPI_INT, *comm);
        for (i=0; i<*comm_size; i++)
        {
            global_vect_size += zmq_data.local_vect_sizes[i];
        }
    }
    else
#endif // BUILD_WITH_MPI
    {
        zmq_data.local_vect_sizes[0]  = *local_vect_size;
        global_vect_size = *local_vect_size;
    }

    zmq_data.nb_proc_server = zmq_data.rinit_tab[0];
    zmq_data.sobol = zmq_data.rinit_tab[1];
    zmq_data.nb_parameters = zmq_data.rinit_tab[2];

    zmq_data.server_vect_size = calloc (zmq_data.nb_proc_server, sizeof(int));

    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_data.server_vect_size[i] = global_vect_size / zmq_data.nb_proc_server;
        if (i < global_vect_size % zmq_data.nb_proc_server)
            zmq_data.server_vect_size[i] += 1;
    }

    zmq_data.send_counts = calloc (zmq_data.nb_proc_server, sizeof(int));
    zmq_data.sdispls     = calloc (zmq_data.nb_proc_server, sizeof(int));

    comm_n_to_m_init (&zmq_data, global_vect_size, zmq_data.local_vect_sizes, *rank);

    node_names = malloc (zmq_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME * sizeof(char));

    if (*rank == 0)
    {
        zmq_send (zmq_data.init_requester, zmq_data.local_vect_sizes, *comm_size * sizeof(int), 0);
        zmq_recv (zmq_data.init_requester, node_names, zmq_data.rinit_tab[0] * MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
    }

#ifdef BUILD_WITH_MPI
    if (*comm_size > 1)
    {
        MPI_Bcast (node_names, zmq_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, *comm);
    }
#endif // BUILD_WITH_MPI

    // sobol only part //
    if (zmq_data.sobol == 1)
    {
        // split MPI_COMM_WORLD for coupled simulations
        if (zmq_data.coupling != 0)
        {
#ifdef BUILD_WITH_MPI
            MPI_Comm_split(MPI_COMM_WORLD, *rank, *sobol_rank, &zmq_data.comm_sobol);
#endif // BUILD_WITH_MPI
        }
        else
        {
            // get Sobol master node name
            if (*sobol_rank == 0)
            {
                melissa_get_node_name (master_node_name);
                if (*rank == 0)
                {
                    master_node_names = malloc (MPI_MAX_PROCESSOR_NAME * *comm_size * sizeof(char));
                }
#ifdef BUILD_WITH_MPI
                if (*comm_size > 1)
                {
                    MPI_Gather(master_node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, master_node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, *comm);
                }
#else // BUILD_WITH_MPI
                else
                {
                    memcpy (master_node_names, master_node_name, MPI_MAX_PROCESSOR_NAME);
                }
#endif // BUILD_WITH_MPI
            }

            sprintf (port_name, "../../DATA/master_name.txt");
            file = fopen(port_name, "r");

            if (file == NULL)
            {
                sprintf (port_name, "master_name.txt");
                file = fopen(port_name, "r");
            }
            if (file != NULL)
            {
                fgets(master_node_name, MPI_MAX_PROCESSOR_NAME, file);
                fclose(file);
            }
            else
            {
                strcpy (master_node_name, "localhost");
                if (*sobol_rank == 0 && *rank == 0)
                {
                    fprintf(stdout,"WARNING: Group %d master name set to \"localhost\"\n", *sample_id);
                }
            }
            if (*sobol_rank == 0)
            {
                if (*rank == 0)
                {
                    master_requester = zmq_socket (zmq_data.context, ZMQ_REP);
                    if (0 == strcmp(master_node_name, "localhost"))
                    {
                        sprintf (port_name, "tcp://*:%d", 3004+*sample_id);
                    }
                    else
                    {
                        sprintf (port_name, "tcp://*:3004");
                    }
                    melissa_bind (master_requester, port_name);
                }
            }
            else // if *sobol_rank != 0
            {
                master_requester = zmq_socket (zmq_data.context, ZMQ_REQ);
                if (0 == strcmp(master_node_name, "localhost"))
                {
                    sprintf (port_name, "tcp://%s:%d", master_node_name, 3004+*sample_id);
                }
                else
                {
                    sprintf (port_name, "tcp://%s:3004", master_node_name);
                }
                melissa_connect (master_requester, port_name);
            }
        }
    }

    // end sobol only //

    zmq_data.data_pusher = NULL;
    if (*sobol_rank == 0)
    {
        zmq_data.data_pusher = malloc (zmq_data.local_nb_messages * sizeof(void*));

        j = 0;
        for (i=0; i<zmq_data.total_nb_messages; i++)
        {
            if (*rank == zmq_data.push_rank[i])
            {
                zmq_data.data_pusher[j] = zmq_socket (zmq_data.context, ZMQ_PUSH);
                zmq_setsockopt (zmq_data.data_pusher[j], ZMQ_SNDHWM, &zmq_data.local_nb_messages, sizeof(int));
                zmq_setsockopt (zmq_data.data_pusher[j], ZMQ_LINGER, &linger, sizeof(int));
                port_no = 100 + zmq_data.pull_rank[i];
                sprintf (port_name, "tcp://%s:11%d", &node_names[MPI_MAX_PROCESSOR_NAME * zmq_data.pull_rank[i]], port_no);
                melissa_connect (zmq_data.data_pusher[j], port_name);
                j += 1;
            }
        }

        if (j != zmq_data.local_nb_messages)
        {
            fprintf (stderr, "Warning: wrong number of data pusher ports");
        }
        if (zmq_data.coupling == 0)
        {
            if (zmq_data.sobol == 1)
            {
                for (i=0; i<(zmq_data.nb_parameters+1)*(*comm_size); i++)
                {
                    if (*rank == 0)
                    {
                        //
                        // send node name here
                        //
                        zmq_recv (master_requester, &j, sizeof(int), 0);
                        if (0 == strcmp(master_node_name, "localhost"))
                        {
                            zmq_send (master_requester, master_node_name, MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
                        }
                        else
                        {
                            zmq_send (master_requester, &master_node_names[j*MPI_MAX_PROCESSOR_NAME], MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
                        }
                        //
                        //
                    }
                }
                zmq_data.sobol_requester = malloc ((zmq_data.nb_parameters + 1) * sizeof(void*));
                for (i=0; i<zmq_data.nb_parameters + 1; i++)
                {
                    zmq_data.sobol_requester[i] = zmq_socket (zmq_data.context, ZMQ_REP);
                    if (0 == strcmp(master_node_name, "localhost"))
                    {
                        sprintf (port_name, "tcp://*:4%d", 100 + (*sample_id * *comm_size * (zmq_data.nb_parameters+1) + *rank * (zmq_data.nb_parameters+1) + i));
                    }
                    else
                    {
                        sprintf (port_name, "tcp://*:4%d", 100 + *rank * (zmq_data.nb_parameters+1) + i);
                    }
                    melissa_bind (zmq_data.sobol_requester[i], port_name);
                }
            }
        }
    }
    else // if *sobol_rank != 0
    {
        if (zmq_data.coupling == 0)
        {
            //
            // ask master node name here
            //
            zmq_send (master_requester, rank, sizeof(int), 0);
            zmq_recv (master_requester, master_node_name, MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
            //
            //
            zmq_data.sobol_requester = malloc (sizeof(void*));
            zmq_data.sobol_requester[0] = zmq_socket (zmq_data.context, ZMQ_REQ);
            if (0 == strcmp(master_node_name, "localhost"))
            {
                sprintf (port_name, "tcp://%s:4%d", master_node_name, 100 + (*sample_id * *comm_size * (zmq_data.nb_parameters+1) + *rank * (zmq_data.nb_parameters+1) + *sobol_rank - 1));
            }
            else
            {
                sprintf (port_name, "tcp://%s:4%d", master_node_name, 100 + *rank * (zmq_data.nb_parameters+1) + *sobol_rank - 1);
            }
            melissa_connect (zmq_data.sobol_requester[0], port_name);
        }
    }

    zmq_close (zmq_data.init_requester);
    zmq_close (zmq_data.connexion_requester);
    if (zmq_data.coupling == 0)
    {
        zmq_close (master_requester);
    }
    zmq_data.send_buff_size = zmq_data.buff_size * sizeof(double);
    if (zmq_data.sobol)
    {
        zmq_data.send_buff_size *= (zmq_data.nb_parameters+2);
    }
    zmq_data.send_buff_size += 4 * sizeof(int) + MAX_FIELD_NAME * sizeof(char);
#ifndef ZEROCOPY
    zmq_data.buffer = malloc (zmq_data.send_buff_size);
#endif // ZEROCOPY
    if (zmq_data.sobol)
    {
        if (*sobol_rank == 0)
        {
            zmq_data.buffer_sobol = malloc ((zmq_data.nb_parameters+2) * *local_vect_size * sizeof (double*));
        }
    }
    free (node_names);
}

#ifdef BUILD_WITH_MPI
/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * Fortran wrapper for melissa_init (convert MPI communicator)
 *
 *******************************************************************************
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *comm_size
 * sise of the MPI communicator comm
 *
 * @param[in] *rank
 * MPI rank
 *
 * @param[in] *sobol_rank
 * Sobol indice rank in Sobol group
 *
 * @param[in] *sample_id
 * ID of the parameter set defining the simulation
 *
 * @param[in] *comm_fortran
 * Fortran MPI communicator
 *
 * @param[in] *coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init_f (int       *local_vect_size,
                     int       *comm_size,
                     int       *rank,
                     const int *sobol_rank,
                     const int *sample_id,
                     MPI_Fint  *comm_fortran,
                     int       *coupling)
{
    MPI_Comm comm = MPI_Comm_f2c(*comm_fortran);
    melissa_init(local_vect_size, comm_size, rank, sobol_rank, sample_id, &comm, coupling);
}
#endif // BUILD_WITH_MPI

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * This function initialise connexion with Melissa Server for sequential simulations
 *
 *******************************************************************************
 *
 * @param[in] *vect_size
 * sise of the data vector to send to the library
 *
 * @param[in] *sobol_rank
 * Sobol indice rank in Sobol group
 *
 * @param[in] *sample_id
 * ID of the parameter set defining the simulation
 *
 *******************************************************************************/

void melissa_init_no_mpi (const int *vect_size,
                          const int *sobol_rank,
                          const int *sample_id)
{
    int rank = 0;
    int comm_size = 1;
    int coupling = 0;
    MPI_Comm comm = 0;
    melissa_init (vect_size,
                  &comm_size,
                  &rank,
                  sobol_rank,
                  sample_id,
                  &comm,
                  &coupling);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * This function sends data to Melissa Server
 *
 *******************************************************************************
 *
 * @param[in] time_step
 * current time step of the simulation
 *
 * @param[in] *field_name
 * name of the field to send to Melissa Server
 *
 * @param[in] *send_vect
 * local data array to send to the statistic library
 *
 * @param[in] *rank
 * MPI rank
 *
 * @param[in] *sobol_rank
 * Sobol indice rank in Sobol group
 *
 * @param[in] *sample_id
 * ID of the parameter set defining the simulation
 *
 *******************************************************************************/

void melissa_send (const int  *time_step,
                   const char *field_name,
                   double     *send_vect,
                   const int  *rank,
                   const int  *sobol_rank,
                   const int  *sample_id)
{
    int   i=0, j, k, ret;
    int   buff_size;
    char *buff_ptr;
    int local_vect_size = zmq_data.local_vect_sizes[*rank];
//    MPI_Request *request;
//    MPI_Status *status;

#ifdef BUILD_WITH_PROBES
    start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

    if (zmq_data.sobol == 1)
    {
        if (zmq_data.coupling == 0)
        {
            // gather data from other ranks of the sobol group
            if (*sobol_rank == 0)
            {
                zmq_pollitem_t items [zmq_data.nb_parameters + 1];
                for (i=0; i<zmq_data.nb_parameters + 1; i++)
                {
                    items[i].socket = zmq_data.sobol_requester[i];
                    items[i].fd = 0;
                    items[i].events = ZMQ_POLLIN;
                    items[i].revents = 0;
                }
                j=0;
                //recv data from other ranks of the sobol group
                while (j<zmq_data.nb_parameters + 1)
                {
                    zmq_poll (items, zmq_data.nb_parameters + 1, -1);
                    for (i=0; i<zmq_data.nb_parameters + 1; i++)
                    {
                        if (items[i].revents & ZMQ_POLLIN)
                        {
                            zmq_recv (zmq_data.sobol_requester[i], &zmq_data.buffer_sobol[i*local_vect_size], local_vect_size * sizeof(double), 0);
                            j += 1;
                        }
                    }
                }
                for (i=0; i<zmq_data.nb_parameters + 1; i++)
                {
                    zmq_send (zmq_data.sobol_requester[i], &i, sizeof(int), 0);
                }
            }
            else // *sobol_rank != 0
            {
                //send data to rank 0 of the sobol group
                zmq_send (zmq_data.sobol_requester[0], send_vect, local_vect_size * sizeof(double), 0);
                zmq_recv (zmq_data.sobol_requester[0], &j, sizeof(int), 0);
#ifdef BUILD_WITH_PROBES
                total_bytes_sent += local_vect_size * sizeof(double);
#endif // BUILD_WITH_PROBES
            }
        }
        else
        {
#ifdef BUILD_WITH_MPI
            MPI_Gather(send_vect, local_vect_size, MPI_DOUBLE, zmq_data.buffer_sobol, local_vect_size, MPI_DOUBLE, 0, zmq_data.comm_sobol);
            //        MPI_Igather(send_vect, local_vect_size, MPI_DOUBLE, zmq_data.buffer_sobol, local_vect_size, MPI_DOUBLE, 0, zmq_data.comm_sobol, request);
            //        MPI_Wait(request, status);
#else // BUILD_WITH_MPI
            memcpy (zmq_data.buffer_sobol, send_vect, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI
            total_bytes_sent += local_vect_size * sizeof(double);
        }
    }


    if (*sobol_rank == 0)
    {
#ifdef ZEROCOPY
        zmq_msg_t msg;
        j = 0;
        for (i=0; i<zmq_data.total_nb_messages; i++)
        {
            if (*rank == zmq_data.push_rank[i] && *sobol_rank == 0)
            {
                buff_size = 4 * sizeof(int) + MAX_FIELD_NAME * sizeof(char) + zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double);
                if (zmq_data.sobol == 1)
                {
                    buff_size += zmq_data.send_counts[zmq_data.pull_rank[i]] * (zmq_data.nb_parameters + 1) * sizeof(double);
                }
                zmq_data.buffer = malloc (buff_size);
                buff_ptr = zmq_data.buffer;
                memcpy(buff_ptr, time_step, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, sobol_rank, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, sample_id, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, rank, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy (buff_ptr, field_name, MAX_FIELD_NAME * sizeof(char));
                buff_ptr += MAX_FIELD_NAME * sizeof(char);
                memcpy (buff_ptr, &send_vect[zmq_data.sdispls[zmq_data.pull_rank[i]]], zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double));
                if (zmq_data.sobol == 1)
                {
                    for (k=1; k<zmq_data.nb_parameters + 2; k++)
                    {
                        buff_ptr += zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double);
                        memcpy (buff_ptr, &zmq_data.buffer_sobol[k*local_vect_size + zmq_data.sdispls[zmq_data.pull_rank[i]]],
                                zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double));
                    }
                }
                zmq_msg_init_data (&msg, zmq_data.buffer, buff_size, my_free, NULL);
                ret = zmq_msg_send (&msg, zmq_data.data_pusher[j], 0);
                if (ret == -1)
                {
                    ret = errno;
                    print_zmq_error(ret);
                }
                j += 1;
#ifdef BUILD_WITH_PROBES
                total_bytes_sent += zmq_data.send_buff_size;
#endif // BUILD_WITH_PROBES
            }
        }

#else // ZEROCOPY
        buff_ptr = zmq_data.buffer;
        memcpy(buff_ptr, time_step, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy(buff_ptr, sobol_rank, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy(buff_ptr, sample_id, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy(buff_ptr, rank, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy (buff_ptr, field_name, MAX_FIELD_NAME * sizeof(char));
        buff_ptr += MAX_FIELD_NAME * sizeof(char);
        j = 0;
        for (i=0; i<zmq_data.total_nb_messages; i++)
        {
            if (*rank == zmq_data.push_rank[i] && *sobol_rank == 0)
            {
                memcpy (buff_ptr, &send_vect[zmq_data.sdispls[zmq_data.pull_rank[i]]], zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double));
                if (zmq_data.sobol == 1)
                {
                    for (k=1; k<zmq_data.nb_parameters + 2; k++)
                    {
                        buff_ptr += zmq_data.send_count[zmq_data.pull_rank[i]] * sizeof(double);
                        memcpy (buff_ptr, &zmq_data.buffer_sobol[k*local_vect_size + zmq_data.sdispls[zmq_data.pull_rank[i]]],
                                zmq_data.send_counts[zmq_data.pull_rank[i]] * sizeof(double));
                    }
                }
                ret = zmq_send (zmq_data.data_pusher[j], zmq_data.buffer, zmq_data.send_buff_size, 0);
                if (ret == -1)
                {
                    ret = errno;
                    print_zmq_error(ret);
                }
                j += 1;
#ifdef BUILD_WITH_PROBES
                total_bytes_sent += zmq_data.send_buff_size;
#endif // BUILD_WITH_PROBES
            }
        }
#endif // ZEROCOPY
    }
#ifdef BUILD_WITH_PROBES
    end_comm_time = melissa_get_time();
    total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * This function sends data to the stats library
 *
 *******************************************************************************
 *
 * @param[in] time_step
 * current time step of the simulation
 *
 * @param[in] *field_name
 * name of the field to send to Melissa Server
 *
 * @param[in] *send_vect
 * local data array to send to the statistic library
 *
 * @param[in] *sobol_rank
 * Sobol indice rank in Sobol group
 *
 * @param[in] *sample_id
 * ID of the parameter set defining the simulation
 *
 *******************************************************************************/

void melissa_send_no_mpi (const int  *time_step,
                          const char *field_name,
                          double     *send_vect,
                          const int  *sobol_rank,
                          const int  *sample_id)
{
    int rank = 0;
    melissa_send (time_step,
                  field_name,
                  send_vect,
                  &rank,
                  sobol_rank,
                  sample_id);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * This function disconects the simulation from the statistic library
 *
 *******************************************************************************/

void melissa_finalize ()
{
    int i;

    if (zmq_data.sobol_rank == 0)
    {
        for (i=0; i<zmq_data.local_nb_messages; i++)
        {
            zmq_close (zmq_data.data_pusher[i]);
        }
#ifndef COUPLING
        if (zmq_data.sobol == 1 && zmq_data.coupling == 0)
        {
            for (i=1; i<zmq_data.nb_parameters+1; i++)
            {
                zmq_close (zmq_data.sobol_requester[i]);
            }
        }
#endif
    }
    if (zmq_data.sobol == 1 && zmq_data.coupling == 0)
    {
        zmq_close (zmq_data.sobol_requester[0]);
    }
    zmq_ctx_term (zmq_data.context);
    if (zmq_data.sobol_rank == 0)
    {
        free(zmq_data.data_pusher);
    }
    free(zmq_data.send_counts);
    free(zmq_data.sdispls);
    free(zmq_data.server_vect_size);
#ifndef ZEROCOPY
    free(zmq_data.buffer);
#endif // ZEROCOPY
    if (zmq_data.sobol == 1 && zmq_data.sobol_rank == 0)
    {
        free(zmq_data.buffer_sobol);
    }
    free(zmq_data.local_vect_sizes);

#ifdef BUILD_WITH_PROBES
    fprintf (stdout, " --- Simulation comm time: %g s\n",total_comm_time);
    fprintf (stdout, " --- Bytes sent: %ld bytes\n",total_bytes_sent);
#endif // BUILD_WITH_PROBES
}
