/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

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

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * @struct global_data_s
 *
 * Structure containing some global data
 *
 *******************************************************************************/

struct global_data_s
{
    void    *context;             /**< ZeroMQ context                                             */
    void    *connexion_requester; /**< connexion ZeroMQ port                                      */
    void   **sobol_requester;     /**< data ZeroMQ Sobol port                                     */
    int      rinit_tab[3];        /**< array used to receive data                                 */
    int      sobol;               /**< 1 if sobol computation, 0 otherwhise                       */
    int      sobol_rank;          /**< sobol rank                                                 */
    int      sample_id;           /**< parameters sample id                                       */
    int      nb_proc_server;      /**< number of MPI processes of the library                     */
    int      nb_parameters;       /**< number of parameters of the study                          */
    char    *buffer;              /**< buffer used to send data to the library                    */
    double  *buffer_sobol;        /**< buffer used to store data on sobol rank 0                  */
    int      buff_size;           /**< size of sobol buffer                                       */
    int      coupling;            /**< coupled simulations or not                                 */
    MPI_Comm comm_sobol;          /**< inter-groups communicator                                  */
};

typedef struct global_data_s global_data_t; /**< type corresponding to global_data_s */

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * @struct field_data_s
 *
 * Structure containing data for each field
 *
 *******************************************************************************/

struct field_data_s
{
    char                  name[MPI_MAX_PROCESSOR_NAME]; /**< The field name                                             */
    int                   global_vect_size;             /**< global field size                                          */
    int                  *server_vect_size;             /**< local vect size for the library                            */
    int                  *local_vect_sizes;             /**< local vector size                                          */
    int                  *send_counts;                  /**< number of elements to send to server rank i                */
    int                  *sdispls;                      /**< displacement to which data should be sent to server rank i */
    int                  *pull_rank;                    /**< rank of the pulling process for the message i              */
    int                  *push_rank;                    /**< rank of the pushing process for the message i              */
    int                   total_nb_messages;            /**< total number of messages                                   */
    int                   local_nb_messages;            /**< local number of messages                                   */
    void                **data_pusher;                  /**< push data ZeroMQ ports                                     */
    struct field_data_s  *next;                         /**< next field_data_struct                                     */
};

typedef struct field_data_s field_data_t; /**< type corresponding to field_data_s */

static global_data_t global_data;
static field_data_t field_data;
static char *node_names;

#ifdef BUILD_WITH_PROBES
static double total_comm_time;
static double start_comm_time;
static double end_comm_time;
static long int total_bytes_sent;
#endif // BUILD_WITH_PROBES

static field_data_t* get_field_data(field_data_t *data,
                                    const char*   field_name)
{
    if (data != NULL)
    {
        if (strncmp(data->name, field_name, MAX_FIELD_NAME) != 0)
        {
            return get_field_data(data->next, field_name);
        }
        else
        {
            return data;
        }
    }
    return data;
}

static void free_field_data(field_data_t *data)
{
    if (data != NULL)
    {
        if (data->next != NULL)
        {
            free_field_data(data->next);
            melissa_free (data->next);
        }
        melissa_free (data->server_vect_size);
        melissa_free (data->send_counts);
        melissa_free (data->local_vect_sizes);
        melissa_free (data->sdispls);
        melissa_free (data->pull_rank);
        melissa_free (data->push_rank);
        if (global_data.sobol_rank == 0)
        {
            int i;
            for (i=0; i<data->local_nb_messages; i++)
            {
                zmq_close (data->data_pusher[i]);
            }
            free(data->data_pusher);
        }
    }
}

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

static inline void comm_1_to_m_init (field_data_t *data,
                                     int            nb_proc_server)
{
    int  i;

    data->sdispls[0] = 0;
    for (i=0; i<nb_proc_server-1; i++)
    {
        data->send_counts[i] = data->server_vect_size[i];
        data->sdispls[i+1] = data->sdispls[i] + data->send_counts[i];
    }
    data->send_counts[nb_proc_server-1] = data->server_vect_size[nb_proc_server-1];

    data->total_nb_messages = nb_proc_server;
    data->local_nb_messages = nb_proc_server;
}

static inline void comm_n_to_m_init (global_data_t *data_glob,
                                     field_data_t  *data_field,
                                     const int      rank)
{
    int  i;
    int  client_rank  = 0;
    int  client_count = 0;
    int  server_rank  = 0;
    int  server_count = 0;
    int  new_message = 0;
    int *server_vect_size = data_field->server_vect_size;
    int  nb_proc_server = data_glob->nb_proc_server;
    int  nb_messages = 0;
    int  nb_elem_message = 0;

    data_field->total_nb_messages = 1;
    data_field->local_nb_messages = 0;

    if (rank == 0)
    {
        data_field->local_nb_messages = 1;
    }

    for (i=0; i<data_field->global_vect_size; i++)
    {
        if (client_count < data_field->local_vect_sizes[client_rank])
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
            data_field->send_counts[server_rank] += 1;
        }

        if (new_message == 1)
        {
            data_field->total_nb_messages += 1;
            if (client_rank == rank)
            {
                data_field->local_nb_messages += 1;
            }
            new_message = 0;
        }
    }

    data_field->sdispls[0] = 0;
    for (i=0; i<nb_proc_server-1; i++)
    {
        data_field->sdispls[i+1] = data_field->sdispls[i] + data_field->send_counts[i];
    }

    new_message = 0;

    data_field->push_rank = malloc (data_field->total_nb_messages * sizeof(int));
    data_field->pull_rank = malloc (data_field->total_nb_messages * sizeof(int));

    data_field->push_rank[0] = 0;
    data_field->pull_rank[0] = 0;
    client_rank  = 0;
    client_count = 0;
    server_rank  = 0;
    server_count = 0;
    for (i=0; i<data_field->global_vect_size; i++)
    {
        if (client_count < data_field->local_vect_sizes[client_rank])
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
            data_field->push_rank[nb_messages] = client_rank;
            data_field->pull_rank[nb_messages] = server_rank;
            new_message = 0;
            nb_elem_message = 0;
        }
        nb_elem_message += 1;
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
 * @param[in] *field_name
 * name of the field to initialize
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
 * @param[in] *simu_id
 * ID of the calling simulation
 *
 * @param[in] *comm
 * MPI communicator
 *
 * @param[in] *coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init (const char *field_name,
                   const int  *local_vect_size,
                   const int  *comm_size,
                   const int  *rank,
                   const int  *simu_id,
                   MPI_Comm   *comm,
                   const int  *coupling)
{
    char           server_node_name[MPI_MAX_PROCESSOR_NAME];
    char           port_name[MPI_MAX_PROCESSOR_NAME] = {0};
    int            port_no, i, j, ret;
    FILE*          file = NULL;
    int            linger = -1;
    char           master_node_name[MPI_MAX_PROCESSOR_NAME];
    char          *master_node_names = NULL;
    void          *master_requester = NULL;
    static int     first_init = 1;
    field_data_t  *comm_ptr = NULL;
    zmq_msg_t      msg;
    char          *buf_ptr = NULL;

    global_data.context = zmq_ctx_new ();
    global_data.connexion_requester = zmq_socket (global_data.context, ZMQ_REQ);
    global_data.sobol_requester = NULL;
//    global_data.sobol_rank = *sobol_rank;
    global_data.coupling = *coupling;
    total_comm_time = 0.0;
    total_bytes_sent = 0;

    // get main server node name
    if (*rank == 0 && first_init != 0)
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

        global_data.rinit_tab[0] = 0;
        global_data.rinit_tab[1] = 1;
        global_data.buff_size    = 0;
        zmq_msg_init_size (&msg, 2 * sizeof(int));
        buf_ptr = zmq_msg_data (&msg);
        memcpy (buf_ptr, comm_size, sizeof(int));
        buf_ptr += sizeof(int);
        memcpy (buf_ptr, simu_id, sizeof(int));
        sprintf (port_name, "tcp://%s:2003", server_node_name);
        melissa_connect (global_data.connexion_requester, port_name);
        ret = zmq_msg_send (&msg, global_data.connexion_requester, 0);
        if (ret == -1)
        {
            ret = errno;
            print_zmq_error(ret);
        }
        zmq_msg_close (&msg);
        zmq_msg_init (&msg);
        ret = zmq_msg_recv (&msg, global_data.connexion_requester, 0);
        if (ret == -1)
        {
            ret = errno;
            print_zmq_error(ret);
        }
        buf_ptr = zmq_msg_data (&msg);
        memcpy(global_data.rinit_tab, buf_ptr, 3 * sizeof(int));
        buf_ptr += 3 * sizeof(int);
    }

    // bcast infos
    if (first_init != 0)
    {
        memcpy (field_data.name, field_name, MPI_MAX_PROCESSOR_NAME);
        comm_ptr = &field_data;
    }
    else
    {
        comm_ptr = get_field_data(&field_data, field_name);
        if (comm_ptr == NULL)
        {
            comm_ptr = melissa_malloc(sizeof (field_data_t));
            memcpy (comm_ptr->name, field_name, MPI_MAX_PROCESSOR_NAME);
        }
        else
        {
            fprintf (stdout, "Warning: field already initialized (%s)\n", field_name);
            return;
        }
    }
    comm_ptr->global_vect_size = 0;
    comm_ptr->local_vect_sizes = malloc(*comm_size * sizeof(int));

#ifdef BUILD_WITH_MPI
    if (*comm_size > 1)
    {
        if (first_init != 0)
        {
            MPI_Bcast(global_data.rinit_tab, 3, MPI_INT, 0, *comm);
        }
        i = *local_vect_size;
        MPI_Allgather(&i, 1, MPI_INT, comm_ptr->local_vect_sizes, 1, MPI_INT, *comm);
        for (i=0; i<*comm_size; i++)
        {
            comm_ptr->global_vect_size += comm_ptr->local_vect_sizes[i];
        }
    }
    else
#endif // BUILD_WITH_MPI
    {
        comm_ptr->local_vect_sizes[0]  = *local_vect_size;
        comm_ptr->global_vect_size = *local_vect_size;
    }

    if (first_init != 0)
    {
        node_names = NULL;
        global_data.nb_proc_server = global_data.rinit_tab[0];
        global_data.sobol = global_data.rinit_tab[1];
        global_data.nb_parameters = global_data.rinit_tab[2];
        if (global_data.sobol == 1)
        {
            global_data.sobol_rank = *simu_id % (global_data.rinit_tab[2] +2);
            global_data.sample_id = *simu_id / (global_data.rinit_tab[2]+2);
        }
        else
        {
            global_data.sobol_rank = 0;
            global_data.sample_id = *simu_id;
        }
        node_names = malloc (global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME * sizeof(char));
    }

    comm_ptr->server_vect_size = calloc (global_data.nb_proc_server, sizeof(int));

    for (i=0; i<global_data.nb_proc_server; i++)
    {
        comm_ptr->server_vect_size[i] = comm_ptr->global_vect_size / global_data.nb_proc_server;
        if (i < comm_ptr->global_vect_size % global_data.nb_proc_server)
            comm_ptr->server_vect_size[i] += 1;
    }

    comm_ptr->send_counts = calloc (global_data.nb_proc_server, sizeof(int));
    comm_ptr->sdispls     = calloc (global_data.nb_proc_server, sizeof(int));

    comm_n_to_m_init (&global_data,
                      comm_ptr,
                      *rank);

    if (*rank == 0 && first_init != 0)
    {
        memcpy(node_names, buf_ptr, global_data.rinit_tab[0] * MPI_MAX_PROCESSOR_NAME * sizeof(char));
        buf_ptr = NULL;
        zmq_msg_close (&msg);
    }

#ifdef BUILD_WITH_MPI
    if (*comm_size > 1 && first_init != 0)
    {
        MPI_Bcast (node_names, global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, *comm);
    }
#endif // BUILD_WITH_MPI

    // sobol only part //
    if (global_data.sobol == 1 && first_init != 0)
    {
        // split MPI_COMM_WORLD for coupled simulations
        if (global_data.coupling != 0)
        {
#ifdef BUILD_WITH_MPI
            MPI_Comm_split(MPI_COMM_WORLD, *rank, global_data.sobol_rank, &global_data.comm_sobol);
#endif // BUILD_WITH_MPI
        }
        else
        {
            // get Sobol master node name
            if (global_data.sobol_rank == 0)
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
                if (global_data.sobol_rank == 0 && *rank == 0)
                {
                    fprintf(stdout,"WARNING: Group %d master name set to \"localhost\"\n", global_data.sample_id);
                }
            }
            if (global_data.sobol_rank == 0)
            {
                if (*rank == 0)
                {
                    master_requester = zmq_socket (global_data.context, ZMQ_REP);
                    if (0 == strcmp(master_node_name, "localhost"))
                    {
                        sprintf (port_name, "tcp://*:%d", 3004+global_data.sample_id);
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
                master_requester = zmq_socket (global_data.context, ZMQ_REQ);
                if (0 == strcmp(master_node_name, "localhost"))
                {
                    sprintf (port_name, "tcp://%s:%d", master_node_name, 3004+global_data.sample_id);
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

    comm_ptr->data_pusher = NULL;
    if (global_data.sobol_rank == 0)
    {
        comm_ptr->data_pusher = malloc (comm_ptr->local_nb_messages * sizeof(void*));

        j = 0;
        for (i=0; i<comm_ptr->total_nb_messages; i++)
        {
            if (*rank == comm_ptr->push_rank[i])
            {
                comm_ptr->data_pusher[j] = zmq_socket (global_data.context, ZMQ_PUSH);
                zmq_setsockopt (comm_ptr->data_pusher[j], ZMQ_SNDHWM, &comm_ptr->local_nb_messages, sizeof(int));
                zmq_setsockopt (comm_ptr->data_pusher[j], ZMQ_LINGER, &linger, sizeof(int));
                port_no = 100 + comm_ptr->pull_rank[i];
                sprintf (port_name, "tcp://%s:11%d", &node_names[MPI_MAX_PROCESSOR_NAME * comm_ptr->pull_rank[i]], port_no);
                melissa_connect (comm_ptr->data_pusher[j], port_name);
                j += 1;
            }
        }

        if (j != comm_ptr->local_nb_messages)
        {
            fprintf (stderr, "Warning: wrong number of data pusher ports");
        }
        if (global_data.coupling == 0 && first_init != 0)
        {
            if (global_data.sobol == 1)
            {
                for (i=0; i<(global_data.nb_parameters+1)*(*comm_size); i++)
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
                global_data.sobol_requester = malloc ((global_data.nb_parameters + 1) * sizeof(void*));
                for (i=0; i<global_data.nb_parameters + 1; i++)
                {
                    global_data.sobol_requester[i] = zmq_socket (global_data.context, ZMQ_REP);
                    if (0 == strcmp(master_node_name, "localhost"))
                    {
                        sprintf (port_name, "tcp://*:4%d", 100 + (global_data.sample_id * *comm_size * (global_data.nb_parameters+1) + *rank * (global_data.nb_parameters+1) + i));
                    }
                    else
                    {
                        sprintf (port_name, "tcp://*:4%d", 100 + *rank * (global_data.nb_parameters+1) + i);
                    }
                    melissa_bind (global_data.sobol_requester[i], port_name);
                }
            }
        }
    }
    else // if *sobol_rank != 0
    {
        if (global_data.coupling == 0 && first_init != 0)
        {
            //
            // ask master node name here
            //
            zmq_send (master_requester, rank, sizeof(int), 0);
            zmq_recv (master_requester, master_node_name, MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
            //
            //
            global_data.sobol_requester = malloc (sizeof(void*));
            global_data.sobol_requester[0] = zmq_socket (global_data.context, ZMQ_REQ);
            if (0 == strcmp(master_node_name, "localhost"))
            {
                sprintf (port_name, "tcp://%s:4%d", master_node_name, 100 + (global_data.sample_id * *comm_size * (global_data.nb_parameters+1) + *rank * (global_data.nb_parameters+1) + global_data.sobol_rank - 1));
            }
            else
            {
                sprintf (port_name, "tcp://%s:4%d", master_node_name, 100 + *rank * (global_data.nb_parameters+1) + global_data.sobol_rank - 1);
            }
            melissa_connect (global_data.sobol_requester[0], port_name);
        }
    }
    if (first_init != 0)
    {
        zmq_close (global_data.connexion_requester);
        if (global_data.coupling == 0)
        {
            zmq_close (master_requester);
        }
    }
    if (global_data.sobol)
    {
        if (global_data.sobol_rank == 0)
        {
            if (first_init != 0)
            {
                global_data.buffer_sobol = malloc ((global_data.nb_parameters+2) * *local_vect_size * sizeof (double));
            }
            else if ((global_data.nb_parameters+2) * *local_vect_size * sizeof (double) > global_data.buff_size)
            {
                global_data.buff_size = (global_data.nb_parameters+2) * *local_vect_size * sizeof (double);
                global_data.buffer_sobol = realloc (global_data.buffer_sobol, (global_data.nb_parameters+2) * *local_vect_size * sizeof (double));
            }
        }
    }
    first_init = 0;
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
 * @param[in] *field_name
 * name of the field to initialize
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
 * @param[in] *simu_id
 * ID of the calling simulation
 *
 * @param[in] *comm_fortran
 * Fortran MPI communicator
 *
 * @param[in] *coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init_f (const char *field_name,
                     int        *local_vect_size,
                     int        *comm_size,
                     int        *rank,
                     const int  *simu_id,
                     MPI_Fint   *comm_fortran,
                     int        *coupling)
{
    MPI_Comm comm = MPI_Comm_f2c(*comm_fortran);
    melissa_init(field_name, local_vect_size, comm_size, rank, simu_id, &comm, coupling);
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
 * @param[in] *field_name
 * name of the field to initialize
 *
 * @param[in] *vect_size
 * sise of the data vector to send to the library
 *
 * @param[in] *simu_id
 * ID of the calling simulation
 *
 *******************************************************************************/

void melissa_init_no_mpi (const char *field_name,
                          const int  *vect_size,
                          const int  *simu_id)
{
    int rank = 0;
    int comm_size = 1;
    int coupling = 0;
    MPI_Comm comm = 0;
    melissa_init (field_name,
                  vect_size,
                  &comm_size,
                  &rank,
                  simu_id,
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
 * @param[in] *simu_id
 * ID of the calling simulation
 *
 *******************************************************************************/

void melissa_send (const int  *time_step,
                   const char *field_name,
                   double     *send_vect,
                   const int  *rank,
                   const int  *simu_id)
{
    int   i=0, j, k, ret;
    int   buff_size;
    char *buff_ptr;
    int local_vect_size;
    field_data_t  *comm_ptr = NULL;
//    MPI_Request *request;
//    MPI_Status *status;

#ifdef BUILD_WITH_PROBES
    start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

    comm_ptr = get_field_data(&field_data, field_name);
    if (comm_ptr == NULL)
    {
        fprintf (stderr, "ERROR: melissa_send call before melissa_init call (%s)\n", field_name );
        return;
    }
    local_vect_size = comm_ptr->local_vect_sizes[*rank];

    if (global_data.sobol == 1)
    {
        if (global_data.coupling == 0)
        {
            // gather data from other ranks of the sobol group
            if (global_data.sobol_rank == 0)
            {
                zmq_pollitem_t items [global_data.nb_parameters + 1];
                for (i=0; i<global_data.nb_parameters + 1; i++)
                {
                    items[i].socket = global_data.sobol_requester[i];
                    items[i].fd = 0;
                    items[i].events = ZMQ_POLLIN;
                    items[i].revents = 0;
                }
                j=0;
                //recv data from other ranks of the sobol group
                while (j<global_data.nb_parameters + 1)
                {
                    zmq_poll (items, global_data.nb_parameters + 1, -1);
                    for (i=0; i<global_data.nb_parameters + 1; i++)
                    {
                        if (items[i].revents & ZMQ_POLLIN)
                        {
                            zmq_recv (global_data.sobol_requester[i], &global_data.buffer_sobol[i*local_vect_size], local_vect_size * sizeof(double), 0);
                            j += 1;
                        }
                    }
                }
                for (i=0; i<global_data.nb_parameters + 1; i++)
                {
                    zmq_send (global_data.sobol_requester[i], &i, sizeof(int), 0);
                }
            }
            else // *sobol_rank != 0
            {
                //send data to rank 0 of the sobol group
                zmq_send (global_data.sobol_requester[0], send_vect, local_vect_size * sizeof(double), 0);
                zmq_recv (global_data.sobol_requester[0], &j, sizeof(int), 0);
#ifdef BUILD_WITH_PROBES
                total_bytes_sent += local_vect_size * sizeof(double);
#endif // BUILD_WITH_PROBES
            }
        }
        else
        {
#ifdef BUILD_WITH_MPI
            MPI_Gather(send_vect, local_vect_size, MPI_DOUBLE, global_data.buffer_sobol, local_vect_size, MPI_DOUBLE, 0, global_data.comm_sobol);

#else // BUILD_WITH_MPI
            memcpy (global_data.buffer_sobol, send_vect, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI
            total_bytes_sent += local_vect_size * sizeof(double);
        }
    }


    if (global_data.sobol_rank == 0)
    {
        zmq_msg_t msg;
        j = 0;
        for (i=0; i<comm_ptr->total_nb_messages; i++)
        {
            if (*rank == comm_ptr->push_rank[i] && global_data.sobol_rank == 0)
            {
                buff_size = 5 * sizeof(int) + MAX_FIELD_NAME * sizeof(char) + comm_ptr->send_counts[comm_ptr->pull_rank[i]] * sizeof(double);
                if (global_data.sobol == 1)
                {
                    buff_size += comm_ptr->send_counts[comm_ptr->pull_rank[i]] * (global_data.nb_parameters + 1) * sizeof(double);
                }
                global_data.buffer = malloc (buff_size);
                buff_ptr = global_data.buffer;
                memcpy(buff_ptr, time_step, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, &global_data.sobol_rank, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, &global_data.sample_id, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, rank, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, &comm_ptr->send_counts[comm_ptr->pull_rank[i]], sizeof(int));
                buff_ptr += sizeof(int);
                memcpy (buff_ptr, field_name, MAX_FIELD_NAME * sizeof(char));
                buff_ptr += MAX_FIELD_NAME * sizeof(char);
                memcpy (buff_ptr, &send_vect[comm_ptr->sdispls[comm_ptr->pull_rank[i]]], comm_ptr->send_counts[comm_ptr->pull_rank[i]] * sizeof(double));
                if (global_data.sobol == 1)
                {
                    for (k=1; k<global_data.nb_parameters + 2; k++)
                    {
                        buff_ptr += comm_ptr->send_counts[comm_ptr->pull_rank[i]] * sizeof(double);
                        memcpy (buff_ptr, &global_data.buffer_sobol[k*local_vect_size + comm_ptr->sdispls[comm_ptr->pull_rank[i]]],
                                comm_ptr->send_counts[comm_ptr->pull_rank[i]] * sizeof(double));
                    }
                }
                zmq_msg_init_data (&msg, global_data.buffer, buff_size, my_free, NULL);
                ret = zmq_msg_send (&msg, comm_ptr->data_pusher[j], 0);
                if (ret == -1)
                {
                    ret = errno;
                    print_zmq_error(ret);
                }
                j += 1;
#ifdef BUILD_WITH_PROBES
                total_bytes_sent += buff_size;
#endif // BUILD_WITH_PROBES
            }
        }
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
 * @param[in] *simu_id
 * ID of the calling simulation
 *
 *******************************************************************************/

void melissa_send_no_mpi (const int  *time_step,
                          const char *field_name,
                          double     *send_vect,
                          const int  *simu_id)
{
    int rank = 0;
    melissa_send (time_step,
                  field_name,
                  send_vect,
                  &rank,
                  simu_id);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * This function disconects the simulation from the statistic library
 *
 *******************************************************************************/

void melissa_finalize (void)
{
    int i;

#ifndef COUPLING
    if (global_data.sobol_rank == 0)
    {
        if (global_data.sobol == 1 && global_data.coupling == 0)
        {
            for (i=1; i<global_data.nb_parameters+1; i++)
            {
                zmq_close (global_data.sobol_requester[i]);
            }
        }
    }
#endif
    free (node_names);
    free_field_data(&field_data);
    if (global_data.sobol == 1 && global_data.coupling == 0)
    {
        zmq_close (global_data.sobol_requester[0]);
    }
    zmq_ctx_term (global_data.context);
#ifndef ZEROCOPY
//    free(global_data.buffer);
#endif // ZEROCOPY
    if (global_data.sobol == 1 && global_data.sobol_rank == 0)
    {
        free(global_data.buffer_sobol);
    }

#ifdef BUILD_WITH_PROBES
    fprintf (stdout, " --- Simulation comm time: %g s\n",total_comm_time);
    fprintf (stdout, " --- Bytes sent: %ld bytes\n",total_bytes_sent);
#endif // BUILD_WITH_PROBES
}