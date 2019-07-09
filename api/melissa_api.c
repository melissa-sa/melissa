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
#include <assert.h>
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
typedef int MPI_Fint; /**< Convert MPI_Fint to int when built without MPI */
#endif // BUILD_WITH_MPI

#ifdef BUILD_WITH_FLOWVR
void flowvr_init(int *comm_size, int *rank);

void send_to_group(void* buff,
                   int buff_size);

void recv_from_group(void* buff);

void flowvr_close();
#endif // BUILD_WITH_FLOWVR

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
    void    *context;               /**< ZeroMQ context                            */
    void    *connexion_requester;   /**< connexion ZeroMQ port                     */
    void    *deconnexion_requester; /**< connexion ZeroMQ port                     */
    void   **sobol_requester;       /**< data ZeroMQ Sobol port                    */
    int      rinit_tab[5];          /**< array used to receive data                */
    int      sobol;                 /**< 1 if sobol computation, 0 otherwhise      */
    int      learning;              /**< 1 if learning, 0 otherwhise               */
    MPI_Comm comm;                  /**< simulation mpi communicator               */
    int      rank;                  /**< mpi rank in comm                          */
    int      comm_size;             /**< mpi comm size                             */
    int      sobol_rank;            /**< sobol rank                                */
    int      sample_id;             /**< parameters sample id                      */
    int      nb_proc_server;        /**< number of MPI processes of the library    */
    int      nb_parameters;         /**< number of parameters of the study         */
    char    *buffer;                /**< buffer used to send data to the library   */
    double  *buffer_data;          /**< buffer used to store data on sobol rank 0 */
    int      buff_size;             /**< size of sobol buffer                      */
    int      coupling;              /**< coupling method                           */
    MPI_Comm comm_sobol;            /**< inter-groups communicator                 */
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
    int                   id;                           /**< The field id                                               */
    int                   global_vect_size;             /**< global field size                                          */
    int                  *server_vect_size;             /**< local vect size for the library                            */
    int                  *local_vect_sizes;             /**< local vector size                                          */
    int                  *send_counts;                  /**< number of elements to send to server rank i                */
    int                  *sdispls;                      /**< displacement to which data should be sent to server rank i */
    int                  *pull_rank;                    /**< rank of the pulling process for the message i              */
    int                  *push_rank;                    /**< rank of the pushing process for the message i              */
    int                   total_nb_messages;            /**< total number of messages                                   */
    int                   local_nb_messages;            /**< local number of messages                                   */
    int                   timestamp;                    /**< melissa internal timestamp                                 */
    void                **data_pusher;                  /**< push data ZeroMQ ports                                     */
    int                  *gatherv_rcvcnt;
    int                  *gatherv_displs;
    struct field_data_s  *next;                         /**< next field_data_struct                                     */
};

typedef struct field_data_s field_data_t; /**< type corresponding to field_data_s */


static global_data_t global_data;
static field_data_t *field_data;
static char *port_names;

static double total_comm_time;
static long int total_bytes_sent;

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

static field_data_t* get_last_field(field_data_t *data)
{
    if (data->next != NULL)
    {
        return get_last_field(data->next);
    }
    return data;
}

static void free_field_data(field_data_t *data)
{

    if (global_data.learning == 1)
    {
        melissa_free (data->gatherv_rcvcnt);
        melissa_free (data->gatherv_displs);
    }
    if (data != NULL)
    {
        if (data->next != NULL)
        {
            free_field_data(data->next);
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
        melissa_free (data);
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

static inline void gatherv_init(field_data_t  *data_field,
                                int           *vect_sizes,
                                int            comm_size)
{
    int i;
    data_field->gatherv_rcvcnt = melissa_malloc (comm_size * sizeof(int));
    data_field->gatherv_displs = melissa_malloc (comm_size * sizeof(int));
    data_field->gatherv_displs[0] = 0;
    for (i=0; i<comm_size-1; i++)
    {
        data_field->gatherv_rcvcnt[i] = vect_sizes[i];
        data_field->gatherv_displs[i+1] = data_field->gatherv_displs[i] + data_field->gatherv_rcvcnt[i];
    }
    data_field->gatherv_rcvcnt[comm_size-1] = vect_sizes[comm_size-1];
}

static inline void comm_1_to_m_init (global_data_t *data_glob,
                                     field_data_t  *data_field,
                                     const int      rank)
{
    int  i;
    int  nb_proc_server = data_glob->nb_proc_server;

    data_field->push_rank = melissa_malloc (nb_proc_server * sizeof(int));
    data_field->pull_rank = melissa_malloc (nb_proc_server * sizeof(int));

    for (i=0; i<nb_proc_server; i++)
    {
        data_field->push_rank[i] = 0;
        data_field->pull_rank[i] = i;
    }

    data_field->sdispls[0] = 0;
    if (rank == 0)
    {
        for (i=0; i<nb_proc_server-1; i++)
        {
            data_field->send_counts[i] = data_field->server_vect_size[i];
            data_field->sdispls[i+1] = data_field->sdispls[i] + data_field->send_counts[i];
        }
        data_field->send_counts[nb_proc_server-1] = data_field->server_vect_size[nb_proc_server-1];

        data_field->total_nb_messages = nb_proc_server;
        data_field->local_nb_messages = nb_proc_server;
    }
    else
    {
        for (i=0; i<nb_proc_server-1; i++)
        {
            data_field->send_counts[i] = 0;
            data_field->sdispls[i+1] = 0;
        }
        data_field->send_counts[nb_proc_server-1] = 0;

        data_field->total_nb_messages = nb_proc_server;
        data_field->local_nb_messages = 0;
    }

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

    data_field->push_rank = melissa_malloc (data_field->total_nb_messages * sizeof(int));
    data_field->pull_rank = melissa_malloc (data_field->total_nb_messages * sizeof(int));

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
 * This function initialise connexion with melissa server
 *
 *******************************************************************************
 *
 * @param[in] *field_name
 * name of the field to initialize
 *
 * @param[in] local_vect_size
 * size of the local data vector to send to the library
 *
 * @param[in] comm_size
 * size of the MPI communicator comm
 *
 * @param[in] rank
 * MPI rank
 *
 * @param[in] simu_id
 * ID of the calling simulation
 *
 * @param[in] comm
 * MPI communicator
 *
 * @param[in] coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init (const char *field_name,
                   const int  local_vect_size,
                   const int  comm_size,
                   const int  rank,
                   const int  simu_id,
                   MPI_Comm   comm,
                   const int  coupling)
{
    char          *server_node_name;
    char           port_name[MPI_MAX_PROCESSOR_NAME] = {0};
    int            i, j, ret;
    FILE*          file = NULL;
    int            linger = -1;
    char          *master_node_name;
    char          *master_node_names = NULL;
    void          *master_requester = NULL;
    static int     first_init = 1;
    field_data_t  *field_data_ptr = NULL;
    zmq_msg_t      msg;
    char          *buf_ptr = NULL;

//    global_data.sobol_rank = *sobol_rank;
    global_data.coupling = coupling;
    total_comm_time = 0.0;
    total_bytes_sent = 0;
    if (first_init != 0)
    {
        global_data.buff_size = 0;
        global_data.context = zmq_ctx_new ();
        global_data.connexion_requester = zmq_socket (global_data.context, ZMQ_REQ);
        global_data.deconnexion_requester = zmq_socket (global_data.context, ZMQ_REQ);
        global_data.sobol_requester = NULL;
        global_data.comm_size = comm_size;
#ifdef BUILD_WITH_MPI
        if(comm)//comm may be null in case of a call of melissa_init_no_mpi with a libmelissa_api.so compiled with MPI support with a non MPI client. This if is here to avoid a useless MPI call outside MPI context.
          MPI_Comm_dup(comm, &global_data.comm);
#else // BUILD_WITH_MPI
        global_data.comm = comm;
#endif // BUILD_WITH_MPI
    }

    // get main server node name
    if (rank == 0 && first_init != 0)
    {
        server_node_name = getenv("MELISSA_SERVER_NODE_NAME");
        melissa_print (VERBOSE_DEBUG, "Server node name: %s\n", server_node_name);
        if (server_node_name == NULL)
        {
            server_node_name = melissa_malloc (MPI_MAX_PROCESSOR_NAME);
            file = fopen("../../DATA/server_name.txt", "r");

            if (file == NULL)
            {
                file = fopen("server_name.txt", "r");

                if (file == NULL)
                {
                    file = fopen("../server_name.txt", "r");

                    if (file == NULL)
                    {
                        strcpy (server_node_name, "localhost");
                        fprintf(stdout,"WARNING: Server name set to \"localhost\"\n");
                    }
                }
            }

            if (file != NULL)
            {
                fgets(server_node_name, MPI_MAX_PROCESSOR_NAME, file);
                fclose(file);
            }
        }

        global_data.rinit_tab[0] = 0;
        global_data.rinit_tab[1] = 1;
        zmq_msg_init_size (&msg, 2 * sizeof(int));
        buf_ptr = zmq_msg_data (&msg);
        memcpy (buf_ptr, &comm_size, sizeof(int));
        buf_ptr += sizeof(int);
        memcpy (buf_ptr, &simu_id, sizeof(int));
        sprintf (port_name, "tcp://%s:2003", server_node_name);
        melissa_connect (global_data.connexion_requester, port_name);
        sprintf (port_name, "tcp://%s:2002", server_node_name);
        melissa_connect (global_data.deconnexion_requester, port_name);
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
        memcpy(global_data.rinit_tab, buf_ptr, 5 * sizeof(int));
        buf_ptr += 5 * sizeof(int);
    }

    // init data structure
    if (first_init != 0)
    {
        field_data = melissa_malloc(sizeof (field_data_t));
        memcpy (field_data->name, field_name, MPI_MAX_PROCESSOR_NAME);
        field_data->next = NULL;
        field_data->id = 0;
        field_data_ptr = field_data;
    }
    else{
        field_data_ptr = get_field_data(field_data, field_name);
        if (field_data_ptr == NULL)
        {
            field_data_ptr = get_last_field (field_data);
            field_data_ptr->next = melissa_malloc(sizeof (field_data_t));
            field_data_ptr->next->id = field_data_ptr->id + 1;
            field_data_ptr = field_data_ptr->next;
            memcpy (field_data_ptr->name, field_name, MPI_MAX_PROCESSOR_NAME);
            field_data_ptr->next = NULL;
        }
        else
        {
            fprintf (stdout, "WARNING: field already initialized (%s)\n", field_name);
            return;
        }
    }

    field_data_ptr->global_vect_size = 0;
    field_data_ptr->local_vect_sizes = malloc(comm_size * sizeof(int));
    field_data_ptr->data_pusher = NULL;
    field_data_ptr->timestamp = 0;

    // bcast infos
#ifdef BUILD_WITH_MPI
    if (comm_size > 1)
    {
        if (first_init != 0)
        {
            MPI_Bcast(global_data.rinit_tab, 5, MPI_INT, 0, comm);
        }
        i = local_vect_size;
        MPI_Allgather(&i, 1, MPI_INT, field_data_ptr->local_vect_sizes, 1, MPI_INT, comm);
        for (i=0; i<comm_size; i++)
        {
            field_data_ptr->global_vect_size += field_data_ptr->local_vect_sizes[i];
        }
    }
    else
#endif // BUILD_WITH_MPI
    {
        field_data_ptr->local_vect_sizes[0]  = local_vect_size;
        field_data_ptr->global_vect_size = local_vect_size;
    }

    if (first_init != 0)
    {
        port_names = NULL;
        global_data.nb_proc_server = global_data.rinit_tab[0];
        global_data.sobol = global_data.rinit_tab[1];
        global_data.learning = global_data.rinit_tab[2];
        global_data.nb_parameters = global_data.rinit_tab[3];
        init_verbose_lvl (global_data.rinit_tab[4]);
        if (global_data.sobol == 1)
        {
            master_node_name = melissa_malloc (MPI_MAX_PROCESSOR_NAME * sizeof(char));
            global_data.sobol_rank = simu_id % (global_data.nb_parameters + 2);
            global_data.sample_id = simu_id / (global_data.nb_parameters + 2);

            master_node_name = getenv("MELISSA_MASTER_NODE_NAME");
            // write master node name node name if not found in env
            if (rank == 0 && global_data.sobol_rank == 0  && global_data.coupling == MELISSA_COUPLING_ZMQ && master_node_name == NULL)
            {
                melissa_get_node_name (master_node_name);
                sprintf (port_name, "master_name%d.txt", global_data.sample_id);
                file = fopen(port_name, "w");
                fputs(master_node_name, file);
                fclose(file);
            }
        }
        else
        {
            global_data.sobol_rank = 0;
            global_data.sample_id = simu_id;
        }
        global_data.rank = rank;
        port_names = malloc (global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME * sizeof(char));
    }

    field_data_ptr->server_vect_size = calloc (global_data.nb_proc_server, sizeof(int));

    for (i=0; i<global_data.nb_proc_server; i++)
    {
        field_data_ptr->server_vect_size[i] = field_data_ptr->global_vect_size / global_data.nb_proc_server;
        if (i < field_data_ptr->global_vect_size % global_data.nb_proc_server)
            field_data_ptr->server_vect_size[i] += 1;
    }

    field_data_ptr->send_counts = calloc (global_data.nb_proc_server, sizeof(int));
    field_data_ptr->sdispls     = calloc (global_data.nb_proc_server, sizeof(int));

    if (global_data.learning == 1)
    {
        comm_1_to_m_init (&global_data,
                          field_data_ptr,
                          rank);
        gatherv_init(field_data_ptr,
                     field_data_ptr->local_vect_sizes,
                     comm_size);
    }
    else
    {
        comm_n_to_m_init (&global_data,
                          field_data_ptr,
                          rank);
    }

    if (rank == 0 && first_init != 0)
    {
        memcpy(port_names, buf_ptr, global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME * sizeof(char));
        buf_ptr = NULL;
        zmq_msg_close (&msg);
    }

#ifdef BUILD_WITH_MPI
    if (comm_size > 1 && first_init != 0)
    {
        MPI_Bcast (port_names, global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm);
    }
#endif // BUILD_WITH_MPI

    // sobol only part //
    if (global_data.sobol == 1 && first_init != 0)
    {
        switch (global_data.coupling)
        {
        case MELISSA_COUPLING_MPI:
#ifdef BUILD_WITH_MPI
            // split MPI_COMM_WORLD for coupled simulations
            MPI_Comm_split(MPI_COMM_WORLD, rank, global_data.sobol_rank, &global_data.comm_sobol);
#else // BUILD_WITH_MPI
            fprintf (stderr, "ERROR: Build with MPI to use MPI coupling");
            exit;
#endif // BUILD_WITH_MPI
            break;

        case MELISSA_COUPLING_FLOWVR:
#ifdef BUILD_WITH_FLOWVR
            flowvr_init(&comm_size, &rank);
#else // BUILD_WITH_FLOWVR
            fprintf (stderr, "ERROR: Build with FlowVR to use FlowVR coupling");
            exit;
#endif // BUILD_WITH_FLOWVR
            break;

        case MELISSA_COUPLING_ZMQ:
            // get Sobol master node name
            if (global_data.sobol_rank == 0)
            {
                if (rank == 0)
                {
                    master_node_names = malloc (MPI_MAX_PROCESSOR_NAME * comm_size * sizeof(char));
#ifdef BUILD_WITH_MPI
                }
                if (comm_size > 1)
                {
                    MPI_Gather(master_node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, master_node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm);
                }
                else
                {
#endif // BUILD_WITH_MPI
                    memcpy (master_node_names, master_node_name, MPI_MAX_PROCESSOR_NAME);
                }
            }
            if (global_data.sobol_rank != 0)
            {
                master_node_name = getenv("MELISSA_MASTER_NODE_NAME");
                if (master_node_name == NULL)
                {
                    master_node_name = melissa_malloc (MPI_MAX_PROCESSOR_NAME);
                    sprintf (port_name, "master_name%d.txt", global_data.sample_id);
                    file = fopen(port_name, "r");
                    if (file != NULL)
                    {
                        fgets(master_node_name, MPI_MAX_PROCESSOR_NAME, file);
                        fclose(file);
                    }
                    else
                    {
                        sleep(1);
                        file = fopen(port_name, "r");
                        if (file != NULL)
                        {
                            fgets(master_node_name, MPI_MAX_PROCESSOR_NAME, file);
                            fclose(file);
                        }
                        else
                        {
                            strcpy (master_node_name, "localhost");
                            if (global_data.sobol_rank == 0 && rank == 0)
                            {
                                melissa_print(VERBOSE_WARNING, "Group %d master name set to \"localhost\"\n", global_data.sample_id);
                            }
                        }
                    }
                }
            }

            if (global_data.sobol_rank == 0)
            {
                if (rank == 0)
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
            break;
        default:
            melissa_print(VERBOSE_ERROR, "Bad coupling parameter");
            exit;
        }
    }
    // end sobol only //

    if (global_data.sobol_rank == 0)
    {
        field_data_ptr->data_pusher = malloc (field_data_ptr->local_nb_messages * sizeof(void*));

        j = 0;
        for (i=0; i<field_data_ptr->total_nb_messages; i++)
        {
            if (rank == field_data_ptr->push_rank[i])
            {
                field_data_ptr->data_pusher[j] = zmq_socket (global_data.context, ZMQ_PUSH);
                zmq_setsockopt (field_data_ptr->data_pusher[j], ZMQ_SNDHWM, &field_data_ptr->local_nb_messages, sizeof(int));
                zmq_setsockopt (field_data_ptr->data_pusher[j], ZMQ_LINGER, &linger, sizeof(int));
                melissa_connect (field_data_ptr->data_pusher[j], &port_names[MPI_MAX_PROCESSOR_NAME * field_data_ptr->pull_rank[i]]);
                j += 1;
            }
        }

        if (j != field_data_ptr->local_nb_messages)
        {
            melissa_print(VERBOSE_WARNING, "Wrong number of data pusher ports");
        }
        if (global_data.coupling == MELISSA_COUPLING_ZMQ && first_init != 0)
        {
            if (global_data.sobol == 1)
            {
                for (i=0; i<(global_data.nb_parameters+1)*comm_size; i++)
                {
                    if (rank == 0)
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
                    global_data.sobol_requester[i] = zmq_socket (global_data.context, ZMQ_PULL);
                    if (0 == strcmp(master_node_name, "localhost"))
                    {
                        sprintf (port_name, "tcp://*:4%d", 100 + (global_data.sample_id * comm_size * (global_data.nb_parameters+1) + rank * (global_data.nb_parameters+1) + i));
                    }
                    else
                    {
                        sprintf (port_name, "tcp://*:4%d", 100 + rank * (global_data.nb_parameters+1) + i);
                    }
                    melissa_bind (global_data.sobol_requester[i], port_name);
                }
            }
        }
    }
    else // if *sobol_rank != 0
    {
        if (global_data.coupling == MELISSA_COUPLING_ZMQ && first_init != 0)
        {
            //
            // ask master node name here
            //
            zmq_send (master_requester, &rank, sizeof(int), 0);
            zmq_recv (master_requester, master_node_name, MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);
            //
            //
            global_data.sobol_requester = malloc (sizeof(void*));
            global_data.sobol_requester[0] = zmq_socket (global_data.context, ZMQ_PUSH);
            if (0 == strcmp(master_node_name, "localhost"))
            {
                sprintf (port_name, "tcp://%s:4%d", master_node_name, 100 + (global_data.sample_id * comm_size * (global_data.nb_parameters+1) + rank * (global_data.nb_parameters+1) + global_data.sobol_rank - 1));
            }
            else
            {
                sprintf (port_name, "tcp://%s:4%d", master_node_name, 100 + rank * (global_data.nb_parameters+1) + global_data.sobol_rank - 1);
            }
            melissa_connect (global_data.sobol_requester[0], port_name);
        }
    }
    if (first_init != 0)
    {
        zmq_close (global_data.connexion_requester);
        if (global_data.coupling == MELISSA_COUPLING_ZMQ)
        {
            zmq_close (master_requester);
        }
    }
    if (global_data.sobol)
    {
        if (global_data.learning == 1)
        {
            if (rank == 0)
            {
                if (global_data.sobol_rank == 0)
                {
                    if (first_init != 0)
                    {
                        global_data.buff_size = (global_data.nb_parameters+2) * field_data_ptr->global_vect_size * sizeof (double);
                        global_data.buffer_data = malloc ((global_data.nb_parameters+2) * field_data_ptr->global_vect_size * sizeof (double));
                    }
                    else if ((global_data.nb_parameters+2) * field_data_ptr->global_vect_size * sizeof (double) > global_data.buff_size)
                    {
                        global_data.buff_size = (global_data.nb_parameters+2) * field_data_ptr->global_vect_size * sizeof (double);
                        global_data.buffer_data = realloc (global_data.buffer_data, (global_data.nb_parameters+2) * field_data_ptr->global_vect_size * sizeof (double));
                    }
                }
                else
                {
                    if (first_init != 0)
                    {
                        global_data.buff_size = field_data_ptr->global_vect_size * sizeof (double);
                        global_data.buffer_data = malloc (field_data_ptr->global_vect_size * sizeof (double));
                    }
                    else if ((global_data.nb_parameters+2) * field_data_ptr->global_vect_size * sizeof (double) > global_data.buff_size)
                    {
                        global_data.buff_size = field_data_ptr->global_vect_size * sizeof (double);
                        global_data.buffer_data = realloc (global_data.buffer_data, field_data_ptr->global_vect_size * sizeof (double));
                    }
                }
            }
            else
            {
                if (first_init != 0)
                {
                    global_data.buff_size = 0;
                    global_data.buffer_data = malloc (sizeof (double));
                }
            }
        }
        else
        {
            if (global_data.sobol_rank == 0)
            {
                if (first_init != 0)
                {
                    global_data.buff_size = (global_data.nb_parameters+2) * local_vect_size * sizeof (double);
                    global_data.buffer_data = malloc ((global_data.nb_parameters+2) * local_vect_size * sizeof (double));
                }
                else if ((global_data.nb_parameters+2) * local_vect_size * sizeof (double) > global_data.buff_size)
                {
                    global_data.buff_size = (global_data.nb_parameters+2) * local_vect_size * sizeof (double);
                    global_data.buffer_data = realloc (global_data.buffer_data, (global_data.nb_parameters+2) * local_vect_size * sizeof (double));
                }
            }
        }
    }
    else if (global_data.learning == 1)
    {
        if (rank == 0)
        {
            if (first_init != 0)
            {
                global_data.buff_size = field_data_ptr->global_vect_size * sizeof (double);
                global_data.buffer_data = malloc (field_data_ptr->global_vect_size * sizeof (double));
            }
            else if (field_data_ptr->global_vect_size * sizeof (double) > global_data.buff_size)
            {
                global_data.buff_size = field_data_ptr->global_vect_size * sizeof (double);
                global_data.buffer_data = realloc (global_data.buffer_data, field_data_ptr->global_vect_size * sizeof (double));
            }
        }
        else
        {
            if (first_init != 0)
            {
                global_data.buff_size = 0;
                global_data.buffer_data = malloc (0);
            }
        }
    }
    first_init = 0;
}

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
 * size of the local data vector to send to the library
 *
 * @param[in] *comm_fortran
 * Fortran MPI communicator
 *
 * @param[in] *coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init_mpi_f (const char *field_name,
                     int        *local_vect_size,
                     MPI_Fint   *comm_fortran,
                     int        *coupling)
{
#ifdef BUILD_WITH_MPI
    MPI_Comm comm = MPI_Comm_f2c(*comm_fortran);
#else // BUILD_WITH_MPI
    int comm = *comm_fortran;
#endif // BUILD_WITH_MPI
    melissa_init_mpi(field_name, *local_vect_size, comm, *coupling);
}

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
 * size of the local data vector to send to the library
 *
 * @param[in] *comm_size
 * size of the MPI communicator comm
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
#ifdef BUILD_WITH_MPI
    MPI_Comm comm = MPI_Comm_f2c(*comm_fortran);
#else // BUILD_WITH_MPI
    int comm = *comm_fortran;
#endif // BUILD_WITH_MPI
    melissa_init(field_name, *local_vect_size, *comm_size, *rank, *simu_id, comm, *coupling);
}

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
 * size of the data vector to send to the library
 *
 * @param[in] *simu_id
 * ID of the calling simulation
 *
 * @param[in] *coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init_no_mpi (const char *field_name,
                          const int  vect_size,
                          const int  simu_id,
                          const int  coupling)
{
    int rank = 0;
    int comm_size = 1;
    MPI_Comm comm = 0;
    if (coupling == MELISSA_COUPLING_MPI)
    {
        melissa_print(VERBOSE_ERROR, "MPI coupling not available in melissa_init_no_mpi");
        exit;
    }
    melissa_init (field_name,
                  vect_size,
                  comm_size,
                  rank,
                  simu_id,
                  comm,
                  coupling);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_api
 *
 * This function initializes the connection with the Melissa Server for mpi
 * simulations. It gives an easier interface than calling melissa_init directly
 * and thus should be used instead if possible.
 *
 *******************************************************************************
 *
 * @param[in] *field_name
 * name of the field to initialize
 *
 * @param[in] *vect_size
 * size of the data vector to send to the library
 *
 * @param[in] comm
 * MPI communicator. Each rank in it must call melissa_init_mpi. Later each rank must
 * send its part of every field to melissa.
 *
 * @param[in] *coupling
 * 1 if simulation are coupled in the same MPI_COMM_WORLD, 0 otherwhise
 *
 *******************************************************************************/

void melissa_init_mpi (const char *field_name,
                       const int  vect_size,
                       MPI_Comm comm,
                       const int  coupling)
{
#ifdef BUILD_WITH_MPI
    int rank;
    int comm_size;
    int simu_id;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &comm_size);

    char* simu_id_a = getenv("MELISSA_SIMU_ID");
    if (simu_id_a == 0)
    {
        printf("Specify the MELISSA_SIMU_ID environment variable as an int in your launch_group command in options.py! (e.g. cmd = 'mpirun -x MELISSA_SIMU_ID=%%d' %% group.simu_id ");
        exit(1);
    }
    simu_id = atoi(simu_id_a);


    melissa_init (field_name,
                  vect_size,
                  comm_size,
                  rank,
                  simu_id,
                  comm,
                  coupling);
#else
    printf("melissa_init_mpi was called but melissa was compiled without MPI!");
    exit(1);
#endif
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
 * @param[in] *field_name
 * name of the field to send to Melissa Server
 *
 * @param[in] *send_vect
 * local data array to send to the statistic library
 *
 *******************************************************************************/

void melissa_send (const char   *field_name,
                   const double *send_vect)
{
    int     i=0, j=0, k, ret;
    int     buff_size;
    char   *buff_ptr;
    int     local_vect_size = 0;
    double *send_vect_ptr;
    field_data_t  *field_data_ptr = NULL;
//    MPI_Request *request;
//    MPI_Status *status;

#ifdef BUILD_WITH_PROBES
    double start_comm_time;
    double end_comm_time;
    start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

    field_data_ptr = get_field_data(field_data, field_name);
    if (field_data_ptr == NULL)
    {
        fprintf (stdout, "ERROR: melissa_send call before melissa_init call (%s)\n", field_name );
        return;
    }

    local_vect_size = field_data_ptr->local_vect_sizes[global_data.rank];
    send_vect_ptr = send_vect;

    if (global_data.learning == 1)
    {
#ifdef BUILD_WITH_MPI
        MPI_Gatherv(send_vect,
                    local_vect_size,
                    MPI_DOUBLE,
                    global_data.buffer_data,
                    field_data_ptr->gatherv_rcvcnt,
                    field_data_ptr->gatherv_displs,
                    MPI_DOUBLE,
                    0,
                    global_data.comm);
        send_vect_ptr = global_data.buffer_data;
#endif // BUILD_WITH_MPI
        if (global_data.rank == 0)
        {
            local_vect_size = field_data_ptr->global_vect_size;
        }
        else
        {
            local_vect_size = 0;
        }
    }

    if (global_data.sobol == 1)
    {
        melissa_print(VERBOSE_DEBUG, "Group %d gather data (rank %d)\n", global_data.sample_id, global_data.rank);
        switch (global_data.coupling)
        {
#ifdef BUILD_WITH_FLOWVR
        case MELISSA_COUPLING_FLOWVR:
            // gather data from other ranks of the sobol group
            if (global_data.sobol_rank == 0)
            {
                recv_from_group ((void*)&global_data.buffer_data[local_vect_size]);
            }
            else // *sobol_rank != 0
            {
                //send data to rank 0 of the sobol group
                send_to_group (send_vect_ptr, local_vect_size * sizeof(double));
            }
            break;
#endif // BUILD_WITH_FLOWVR

        case MELISSA_COUPLING_ZMQ:
            if (global_data.sobol_rank == 0)
            {
                for (i=0; i<global_data.nb_parameters + 1; i++)
                {
                    zmq_recv (global_data.sobol_requester[i], &global_data.buffer_data[(i+1)*local_vect_size], local_vect_size * sizeof(double), 0);
                }
            }
            else // *sobol_rank != 0
            {
                //send data to rank 0 of the sobol group
                zmq_send (global_data.sobol_requester[0], send_vect_ptr, local_vect_size * sizeof(double), 0);
            }
            break;

#ifdef BUILD_WITH_MPI
        case MELISSA_COUPLING_MPI:
            MPI_Gather(send_vect_ptr, local_vect_size, MPI_DOUBLE, global_data.buffer_data, local_vect_size, MPI_DOUBLE, 0, global_data.comm_sobol);
            break;
#endif // BUILD_WITH_MPI
        }
        total_bytes_sent += local_vect_size * sizeof(double);
    }

    if (global_data.sobol_rank == 0)
    {
        melissa_print(VERBOSE_DEBUG, "Group %d send data (timestamp %d)\n", global_data.sample_id, field_data_ptr->timestamp);
        zmq_msg_t msg;
        j = 0;
        for (i=0; i<field_data_ptr->total_nb_messages; i++)
        {
            if (global_data.rank == field_data_ptr->push_rank[i] && global_data.sobol_rank == 0)
            {
                buff_size = 5 * sizeof(int) + MAX_FIELD_NAME * sizeof(char) + field_data_ptr->send_counts[field_data_ptr->pull_rank[i]] * sizeof(double);
                if (global_data.sobol == 1)
                {
                    buff_size += field_data_ptr->send_counts[field_data_ptr->pull_rank[i]] * (global_data.nb_parameters + 1) * sizeof(double);
                }
                global_data.buffer = malloc (buff_size);
                buff_ptr = global_data.buffer;
                memcpy(buff_ptr, &field_data_ptr->timestamp, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, &global_data.sobol_rank, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, &global_data.sample_id, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, &global_data.rank, sizeof(int));
                buff_ptr += sizeof(int);
                memcpy(buff_ptr, &field_data_ptr->send_counts[field_data_ptr->pull_rank[i]], sizeof(int));
                buff_ptr += sizeof(int);
                memcpy (buff_ptr, field_name, MAX_FIELD_NAME * sizeof(char));
                buff_ptr += MAX_FIELD_NAME * sizeof(char);
                memcpy (buff_ptr, &send_vect_ptr[field_data_ptr->sdispls[field_data_ptr->pull_rank[i]]], field_data_ptr->send_counts[field_data_ptr->pull_rank[i]] * sizeof(double));
                if (global_data.sobol == 1)
                {
                    for (k=1; k<global_data.nb_parameters + 2; k++)
                    {
                        buff_ptr += field_data_ptr->send_counts[field_data_ptr->pull_rank[i]] * sizeof(double);
                        memcpy (buff_ptr, &global_data.buffer_data[k*local_vect_size + field_data_ptr->sdispls[field_data_ptr->pull_rank[i]]],
                                field_data_ptr->send_counts[field_data_ptr->pull_rank[i]] * sizeof(double));
                    }
                }
                zmq_msg_init_data (&msg, global_data.buffer, buff_size, my_free, NULL);
                ret = zmq_msg_send (&msg, field_data_ptr->data_pusher[j], 0);
                melissa_print(VERBOSE_DEBUG, "Message of size %d byte sent (proc %d)\n", buff_size, field_data_ptr->push_rank[i]);
                if (ret == -1)
                {
                    ret = errno;
                    print_zmq_error(ret);
                }
                j += 1;
                total_bytes_sent += buff_size;
            }
        }
    }
    field_data_ptr->timestamp += 1;

#if BUILD_WITH_PROBES
    end_comm_time = melissa_get_time();
    total_comm_time += end_comm_time - start_comm_time;
#endif
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
 * @param[in] *field_name
 * name of the field to send to Melissa Server
 *
 * @param[in] *send_vect
 * local data array to send to the statistic library
 *
 *******************************************************************************/

void melissa_send_no_mpi (const char *field_name,
                          const double *send_vect)
{
    melissa_send (field_name,
                  send_vect);
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
    int        i, temp, ret;
    zmq_msg_t  msg;

#ifdef BUILD_WITH_MPI
    if (global_data.comm_size > 1)
    {
        MPI_Barrier(global_data.comm);
    }
#endif // BUILD_WITH_MPI

    for (field_data_t* field_data_ptr = field_data; field_data_ptr != NULL; field_data_ptr = field_data_ptr->next) {
        // Check that we got at least one timestamp per initialized field.
        assert(field_data_ptr->timestamp > 0);

        // Check that we called melissa_send for the same amount for every field
        assert(field_data_ptr->timestamp == field_data->timestamp);
    }

    if (global_data.rank == 0 && global_data.sobol_rank == 0)
    {
//        sleep(2);
        i = 0;
        while (i<2)
        {
            melissa_print (VERBOSE_DEBUG, "Group %d ready to disconnect \n", global_data.sample_id);
            zmq_msg_init_size (&msg, sizeof(int));
            memcpy (zmq_msg_data (&msg), &global_data.sample_id, sizeof(int));
            ret = zmq_msg_send (&msg, global_data.deconnexion_requester, 0);
            if (ret == -1)
            {
                ret = errno;
                print_zmq_error(ret);
            }
            zmq_msg_close (&msg);
            melissa_print (VERBOSE_DEBUG, "Group %d waiting... \n", global_data.sample_id);
            zmq_msg_init (&msg);
            ret = zmq_msg_recv (&msg, global_data.deconnexion_requester, 0);
            if (ret == -1)
            {
                ret = errno;
                print_zmq_error(ret);
            }
            memcpy(&i, zmq_msg_data (&msg), sizeof(int));
            zmq_msg_close (&msg);
            melissa_print (VERBOSE_DEBUG, "Group status: %d \n", i);
            if (i==1)
            {
                sleep(2);
            }
        }
    }
    zmq_close (global_data.deconnexion_requester);
#ifdef BUILD_WITH_MPI
    if (global_data.comm_size > 1)
    {
        MPI_Barrier(global_data.comm);
    }
#endif // BUILD_WITH_MPI

#ifdef BUILD_WITH_FLOWVR
    if (global_data.sobol == 1 && global_data.coupling == MELISSA_COUPLING_FLOWVR)
    {
        flowvr_close();
    }
#endif // BUILD_WITH_FLOWVR
    if (global_data.sobol == 1 && global_data.coupling == MELISSA_COUPLING_ZMQ)
    {
        if (global_data.sobol_rank == 0)
        {
            for (i=1; i<global_data.nb_parameters+1; i++)
            {
                zmq_close (global_data.sobol_requester[i]);
            }
        }
        zmq_close (global_data.sobol_requester[0]);
    }

    free_field_data(field_data);
    melissa_print(VERBOSE_DEBUG, "Free ZMQ context...\n");
    zmq_ctx_term (global_data.context);
    melissa_print(VERBOSE_DEBUG, "Free ZMQ context OK\n");
    free (port_names);

    if (global_data.sobol == 1 && global_data.sobol_rank == 0)
    {
        free(global_data.buffer_data);
    }

#ifdef BUILD_WITH_PROBES
    melissa_print(VERBOSE_INFO, " --- Simulation comm time: %g s\n",total_comm_time);
#endif
    melissa_print(VERBOSE_INFO, " --- Bytes sent: %ld bytes\n",total_bytes_sent);
}
