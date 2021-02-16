/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENSE file for further information.             *
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

#include <melissa/api.h>
#include <melissa/messages.h>
#include <melissa/utils.h>

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mpi.h>
#include <zmq.h>

#define MELISSA_COUPLING_NONE 0    /**< No coupling */
#define MELISSA_COUPLING_DEFAULT 0 /**< Default coupling */
#define MELISSA_COUPLING_ZMQ 0     /**< ZeroMQ coupling */
#define MELISSA_COUPLING_MPI 1     /**< MPI coupling */
#define MELISSA_COUPLING_FLOWVR 2  /**< FlowVR coupling */

#ifndef MPI_MAX_PROCESSOR_NAME
#define MPI_MAX_PROCESSOR_NAME 256 /**< maximum size of processor names */
#endif

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
#ifdef CHECK_SIMU_DECONNECTION
    void    *deconnexion_requester; /**< connexion ZeroMQ port                     */
#endif // CHECK_SIMU_DECONNECTION
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
    double  *buffer_data;           /**< buffer used to store data on sobol rank 0 */
    double **data_ptr;              /**< ptr to the data in buffer_data            */
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
    char                  name[MAX_FIELD_NAME_LEN+1];   /**< The field name                                             */
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
        if (strncmp(data->name, field_name, MAX_FIELD_NAME_LEN) != 0)
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
    if (global_data.learning > 0)
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


static void die(int error)
{
	fprintf(stderr, "ZeroMQ socket: %s\n", zmq_strerror(error));
    exit(EXIT_FAILURE);
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

// this function is used when the simulation has only one process
// it defines the partitioning of the data of one field.
static inline void comm_1_to_m_init (global_data_t *data_glob,
                                     field_data_t  *data_field,
                                     const int      rank)
{
    int  i;
    int  nb_proc_server = data_glob->nb_proc_server; // number of processes of the server

    data_field->push_rank = melissa_malloc (nb_proc_server * sizeof(int)); // for each of the total_nb_messages messages, the rank of the simulation that will send it (here, 0)
    data_field->pull_rank = melissa_malloc (nb_proc_server * sizeof(int)); // for each of the total_nb_messages messages, the rank of the server that will receive it

    for (i=0; i<nb_proc_server; i++)
    {
        // here the messages distribution among the processes is trivial.
        // there will be nb_proc_server mesages, each one sent to its corresponding process, by process 0 of the simulation.
        data_field->push_rank[i] = 0;
        data_field->pull_rank[i] = i;
    }

    data_field->sdispls[0] = 0;
    // rank zero sends everything, evenly partitioned.
    // in the server side, the same partitiçoning is computed.
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
    // the other ranks send nothing
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

// this function compute the data redistribution from N simulation processes to M server processes.
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
    // server_vect_size[] will store the local vect sizes from the server point of view
    int *server_vect_size = data_field->server_vect_size;
    int  nb_proc_server = data_glob->nb_proc_server;
    int  nb_messages = 0;
    int  nb_elem_message = 0;

    data_field->total_nb_messages = 1; // at least one message
    data_field->local_nb_messages = 0; // can stay zero in some extreme cases

    if (rank == 0)
    {
        data_field->local_nb_messages = 1;
    }

    // we count and attribute each data point to a simulation rank and a server rank.
    for (i=0; i<data_field->global_vect_size; i++)
    {
        if (client_count < data_field->local_vect_sizes[client_rank])
        {
            client_count += 1;
        }
        else // if the current point is on the next simulation rank, we update the client rank and request to add a new message
        {
            client_count = 1;
            client_rank += 1;
            new_message = 1;
        }
        if (server_count < server_vect_size[server_rank])
        {
            server_count += 1;
        }
        else // if the current point is on the next server rank, we update the server rank and request to add a new message
        {
            server_count = 1;
            server_rank += 1;
            new_message = 1;
        }

        if (client_rank == rank)
        {
            // increment the send_count of the server_rank corresponding to the data point
            data_field->send_counts[server_rank] += 1;
        }

        if (new_message == 1) // if one of the conditions before requested a new message:
        {
            data_field->total_nb_messages += 1; // add a message (global)
            if (client_rank == rank)
            {
                data_field->local_nb_messages += 1; // add a message (local)
            }
            new_message = 0;
        }
    }

    data_field->sdispls[0] = 0;
    // we compute the sdispls correponding to the send_counts
    for (i=0; i<nb_proc_server-1; i++)
    {
        data_field->sdispls[i+1] = data_field->sdispls[i] + data_field->send_counts[i];
    }

    new_message = 0;

    data_field->push_rank = melissa_malloc (data_field->total_nb_messages * sizeof(int)); // for each of the total_nb_messages messages, the rank of the simulation that will send it
    data_field->pull_rank = melissa_malloc (data_field->total_nb_messages * sizeof(int)); // for each of the total_nb_messages messages, the rank of the server that will receive it

    data_field->push_rank[0] = 0;
    data_field->pull_rank[0] = 0;
    client_rank  = 0;
    client_count = 0;
    server_rank  = 0;
    server_count = 0;
    // We do a last loop over the total number of messages, and we compute the push_rank (client) and the pull_rank (server) of each message.
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
 * @param[in] comm
 * MPI communicator
 *
 *******************************************************************************/

static void melissa_init_internal (const char *field_name,
                            const int  local_vect_size,
                            const int  comm_size,
                            const int  rank,
                            MPI_Comm   comm)
{
    char          *server_node_name;
    // extra bytes for "tcp://", TCP port number ":%d", null byte
    char           port_name[MPI_MAX_PROCESSOR_NAME + 32 + 1] = {0};
    int            i, j, ret;
    int            simu_id = -1;
    FILE*          file = NULL;
    int            linger = -1;
    char          *master_node_names = NULL;
    void          *master_requester = NULL;
    static int     first_init = 1;
    field_data_t  *field_data_ptr = NULL;
    zmq_msg_t      msg;
    char          *buf_ptr = NULL;

//    global_data.sobol_rank = *sobol_rank;

    total_comm_time = 0.0;
    total_bytes_sent = 0;
    // this function is called once per simulation field. However, some actions are
    // required only during the first call (to contact the server for example).
    // this is the meaning of this condition.
    if (first_init != 0)
    {
        global_data.buff_size = 0;
        global_data.context = zmq_ctx_new (); // initialize zmq context
        global_data.connexion_requester = zmq_socket (global_data.context, ZMQ_REQ); // create a REQ socket to send a request to the server
#ifdef CHECK_SIMU_DECONNECTION
        global_data.deconnexion_requester = zmq_socket (global_data.context, ZMQ_REQ);
#endif // CHECK_SIMU_DECONNECTION
        global_data.sobol_requester = NULL;
        global_data.comm_size = comm_size;
        assert(comm);
        MPI_Comm_dup(comm, &global_data.comm);
        const char* simu_id_a = getenv("MELISSA_SIMU_ID"); // get the simu ID from the environment variable
        if (simu_id_a == 0)
        {
            printf("Specify the MELISSA_SIMU_ID environment variable as an int in your launch_group command in options.py! (e.g. cmd = 'mpirun -x MELISSA_SIMU_ID=%%d' %% group.simu_id ");
            exit(1);
        }
        simu_id = atoi(simu_id_a);
    } // end if (first_init != 0)

    // get main server node name
    if (rank == 0 && first_init != 0) // only on first call, and for proc 0
    {
        server_node_name = getenv("MELISSA_SERVER_NODE_NAME"); // get the server node name from the environment variable
        melissa_print (VERBOSE_DEBUG, "Server node name: %s\n", server_node_name);
        if (server_node_name == NULL)
        {
            // here for backward compatibility
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
        // we put the mpi comm_size and the simu_id in the data buffer of the message
        memcpy (buf_ptr, &comm_size, sizeof(int));
        buf_ptr += sizeof(int);
        memcpy (buf_ptr, &simu_id, sizeof(int));
        sprintf (port_name, "tcp://%s:2003", server_node_name);
        // we connect to the rank 0 of the server on port 2003
        melissa_connect (global_data.connexion_requester, port_name);
#ifdef CHECK_SIMU_DECONNECTION
        sprintf (port_name, "tcp://%s:2002", server_node_name);
        melissa_connect (global_data.deconnexion_requester, port_name);
#endif // CHECK_SIMU_DECONNECTION
        // we send the message
        if (zmq_msg_send (&msg, global_data.connexion_requester, 0) < 0)
        {
            die(errno);
        }
        zmq_msg_close (&msg);
        zmq_msg_init (&msg);
        // we wait for the response
        if (zmq_msg_recv (&msg, global_data.connexion_requester, 0) < 0)
        {
			die(errno);
        }
        buf_ptr = zmq_msg_data (&msg);
        // we copy the first 5 int of the response data in the rinit tab. We will need it to init the persistent data structures.
        memcpy(global_data.rinit_tab, buf_ptr, 5 * sizeof(int));
        // The pointer is set to point after the 5 first int. We will comme to it later. That is why we do not close the message yet.
        buf_ptr += 5 * sizeof(int);
    } // endif (rank == 0 && first_init != 0)

    // init data structure
    if (first_init != 0)
    {
        // allocate memory for the first field. The simulation must send at least one field.
        field_data = melissa_malloc(sizeof (field_data_t));
        // The field is identified by its name.
        strncpy(field_data->name, field_name, MAX_FIELD_NAME_LEN);
        field_data->next = NULL;
        field_data->id = 0;
        // set field_data_ptr to point to the first field of the list
        field_data_ptr = field_data;
    }
    else
    {
        field_data_ptr = get_field_data(field_data, field_name);
        if (field_data_ptr == NULL) // then the field does not exist
        {
            field_data_ptr = get_last_field (field_data); // set field_data_ptr to point to the last existing field
            field_data_ptr->next = melissa_malloc(sizeof (field_data_t)); // we allocate the next one
            field_data_ptr->next->id = field_data_ptr->id + 1;
            field_data_ptr = field_data_ptr->next; // move forward
            strncpy(field_data_ptr->name, field_name, MAX_FIELD_NAME_LEN);
            field_data_ptr->next = NULL;
        }
        else // then the user already called the melissa_init function for this field
        {
            fprintf (stdout, "WARNING: field already initialized (%s)\n", field_name);
            return;
        }
    }

    // field_data_ptr now points to the current field.
    field_data_ptr->global_vect_size = 0;
    field_data_ptr->local_vect_sizes = malloc(comm_size * sizeof(int));
    field_data_ptr->data_pusher = NULL;
    field_data_ptr->timestamp = 0;

    if (comm_size > 1)
    {
        // bcast server infos from 0 to all
        if (first_init != 0)
        {
            MPI_Bcast(global_data.rinit_tab, 5, MPI_INT, 0, comm);
            // we will see later the usage of the values in this 5 ints
        }
        i = local_vect_size;
        // we use a allgather to gather all the local_vect_sizes (notice the "s" at the end)
        // local_vect_size si a int, field_data_ptr->local_vect_sizes is an array of int of size comm_size
        MPI_Allgather(&i, 1, MPI_INT, field_data_ptr->local_vect_sizes, 1, MPI_INT, comm);
        // we compute global_vect_size from the local_vect_sizes
        for (i=0; i<comm_size; i++)
        {
            field_data_ptr->global_vect_size += field_data_ptr->local_vect_sizes[i];
        }
    }
    else
    {
        field_data_ptr->local_vect_sizes[0]  = local_vect_size;
        field_data_ptr->global_vect_size = local_vect_size;
    }

	// We must have a master simulation in our group. It will be
	// simulation 0 of the group. Don't confuse this ID with the MPI
	// rank.  We do the same comm patern than with Melissa server. we
	// need the node name of the rank 0 of the simulation 0 of the
	// group.
	char master_node_name[MPI_MAX_PROCESSOR_NAME + 1] = { 0 };

    if (first_init != 0) // only in the first call
    {
        port_names = NULL;
        global_data.rank = rank;
        // this is where we use the 5 int sent by the server to the API.
        global_data.nb_proc_server = global_data.rinit_tab[0]; // the first one is the size of the server.
        global_data.sobol = global_data.rinit_tab[1]; // second one is 1 if we compute Sobol' indices, 0 otherwise.
        global_data.learning = global_data.rinit_tab[2]; // third one is a flag for learning. this one affects the data redistribution in melissa_send (not implemented yet).
        global_data.nb_parameters = global_data.rinit_tab[3]; // the number of varying parameters of the simulations
        // In the case of Sobol' indices computation, a special communication patern must be set between the members of one Sobol' group.
        if (global_data.sobol == 1)
        {
            // the user chose his coupling method with the MELISSA_COUPLING environment variable.
            // if you use mpirun to launch your simulations, simply define the environment variable with -x MELISSA_COUPLING=...
            char* coupling_a = getenv("MELISSA_COUPLING");
            if (coupling_a == 0)
            {
                printf("Specify the MELISSA_COUPLING environment variable as an int in your launch_group command in options.py! (e.g. cmd = 'mpirun -x MELISSA_COUPLING=%%d' %% group.coupling ");
                exit(1);
            }
            global_data.coupling = atoi(coupling_a);
            // coupling method can be ZMQ, MPI or FlowVR

            // The simulation's Sobol' rank is retrieved through its simu ID
            global_data.sobol_rank = simu_id % (global_data.nb_parameters + 2);
            global_data.sample_id = simu_id / (global_data.nb_parameters + 2);
            // Now, each process have 4 different IDs:
            // - rank (or global_data.rank): its MPI rank in the single simulation MPI communicator
            // - simu_id: the unique simulation ID defined by Melissa Launcher. Not used past this point.
            //            It we do not compute Sobol' indices, equivalent to global_data.sample_id
            // - global_data.sample_id: the ID of the Sobol' group
            // - global_data.sobol_rank: the rank of the simulation inside its Sobol' group (from 0 to nb_param+1)

			// get the master node name from the environment variable. Only
			// needed if we use COUPLING_ZMQ
			const char* master_node_name_env =
				getenv("MELISSA_MASTER_NODE_NAME");

            if (master_node_name_env == NULL)
            {
                ret = 0;
            }
			else
			{
				strncpy(master_node_name, master_node_name_env, MPI_MAX_PROCESSOR_NAME);
				ret = strnlen(master_node_name, MPI_MAX_PROCESSOR_NAME);
			}
            // write master node name node name if not found in env. Always prefer to use the environment variable.
            if (rank == 0 && global_data.sobol_rank == 0  && global_data.coupling == MELISSA_COUPLING_ZMQ && ret < 1)
            {
                melissa_get_node_name (master_node_name, MPI_MAX_PROCESSOR_NAME);
                sprintf (port_name, "master_name%d.txt", global_data.sample_id);
                file = fopen(port_name, "w");
                fputs(master_node_name, file);
                fclose(file);
            }
        }
        else // no Sobol' indices
        {
            // sobol_rank is useless
            global_data.sobol_rank = 0;
            // sample_id is simu_id
            global_data.sample_id = simu_id;
            // we don't need coupling
            global_data.coupling = MELISSA_COUPLING_DEFAULT;
        }

        // we still have to get the server node names to open the messages ports to each server process.
        port_names = malloc (global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME * sizeof(char));

        // now we process the end of the server message. It contains all the node names of the server ranks.
        if (rank == 0) // only rank 0 has the message
        {
            memcpy(port_names, buf_ptr, global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME * sizeof(char));
            buf_ptr = NULL;
            zmq_msg_close (&msg);
        }
        // then we broadcast these node names to all the MPI ranks.
        if (comm_size > 1)
        {
            MPI_Bcast (port_names, global_data.nb_proc_server * MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm);
        }
    }


    // we will need to know the local vect sizes of the server. This is computed staticaly. The same partition is computed in the server side.
    field_data_ptr->server_vect_size = calloc (global_data.nb_proc_server, sizeof(int));

    for (i=0; i<global_data.nb_proc_server; i++)
    {
        // Simple data partitioning
        field_data_ptr->server_vect_size[i] = field_data_ptr->global_vect_size / global_data.nb_proc_server;
        if (i < field_data_ptr->global_vect_size % global_data.nb_proc_server)
            field_data_ptr->server_vect_size[i] += 1;
    }

    // here we will define the number of elements from our local data that we will need to sent to each server process.
    field_data_ptr->send_counts = calloc (global_data.nb_proc_server, sizeof(int));
    // and the corresponding stride in the input buffer.
    field_data_ptr->sdispls     = calloc (global_data.nb_proc_server, sizeof(int));

    if (global_data.learning > 0) // learning case: we need to gather all the data on rank 0 befor the send
    {
        comm_1_to_m_init (&global_data,
                          field_data_ptr,
                          rank);
        // The gatherv_init initialize the mpi_gatherv that will gather the data on rank 0
        gatherv_init(field_data_ptr,
                     field_data_ptr->local_vect_sizes,
                     comm_size);
    }
    else // else, we initialize the NxM comm patern
    {
        comm_n_to_m_init (&global_data,
                          field_data_ptr,
                          rank);
    }

    // --------------- //
    // sobol only part //
    // --------------- //

    if (global_data.sobol == 1 && first_init != 0) // in this section of code, we define the communication patern between members of the Sobol' group.
    {
        // Three coupling scenarios: MPI, FlowVR, or ZMQ (default).
        switch (global_data.coupling)
        {
        case MELISSA_COUPLING_MPI:
            // split MPI_COMM_WORLD for coupled simulations.
            // only works if all the simulations of the Sobol' group are launched in asingle  MPI MPMD command.
            // all the simulation's comm communicator of the group must form a partition of the MPI_COMM_WORLD communicator.
            // here we create a communicator that connects all the simulations processes with the same rank in the comm communicator.
            MPI_Comm_split(MPI_COMM_WORLD, rank, global_data.sobol_rank, &global_data.comm_sobol);
            break;

        case MELISSA_COUPLING_FLOWVR:
#ifdef BUILD_WITH_FLOWVR
            // flowvr coupling script is outside the C code
            flowvr_init(&comm_size, &rank);
#else // BUILD_WITH_FLOWVR
            fprintf (stderr, "ERROR: Build with FlowVR to use FlowVR coupling");
            exit(1);
#endif // BUILD_WITH_FLOWVR
            break;

        case MELISSA_COUPLING_ZMQ:
            // in the case of a ZMQ coupling, we do the same thing than with the server but with the master simulation.
            // get Sobol master node name
            if (global_data.sobol_rank == 0) // master simulation
            {
                if (rank == 0)
                {
                    master_node_names = malloc (MPI_MAX_PROCESSOR_NAME * comm_size * sizeof(char));
                }
                if (comm_size > 1)
                {
                    // gather node names
                    MPI_Gather(master_node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, master_node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm);
                }
                else
                {
                    memcpy (master_node_names, master_node_name, MPI_MAX_PROCESSOR_NAME);
                }
            }
            if (global_data.sobol_rank != 0) // not master simulation
            {
                // get master node name from environment variable.
                ret = sprintf(master_node_name, "%s", getenv("MELISSA_MASTER_NODE_NAME"));
                if (strcmp(master_node_name, "(null)") == 0)
                {
                    ret = 0;
                }
                if (ret == 0)
                {
//                    master_node_name = melissa_malloc (MPI_MAX_PROCESSOR_NAME);
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
            if (0 == strcmp(master_node_name, "(null)"))
            {
                sprintf (master_node_name, "localhost");
            }

            if (global_data.sobol_rank == 0) // master simulation
            {
                if (rank == 0) // MPI rank 0
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
                    melissa_bind (master_requester, port_name); // opent req/rep port
                }
            }
            else // not master simulation
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
                melissa_connect (master_requester, port_name); // Connect to master simulation
            }
            break;
        default:
            melissa_print(VERBOSE_ERROR, "Bad coupling parameter");
            exit(1);
        }
    }
    // -------------- //
    // end sobol only //
    // -------------- //

    if (global_data.sobol_rank == 0)
    {
        // allocate the PUSH ports that will send data to the server.
        // if Sobol, only the master simulation needs these ports.
        field_data_ptr->data_pusher = malloc (field_data_ptr->local_nb_messages * sizeof(void*));

        j = 0;
        for (i=0; i<field_data_ptr->total_nb_messages; i++)
        {
            if (rank == field_data_ptr->push_rank[i]) // we only open the ports that actualy needs to send data
            {
                field_data_ptr->data_pusher[j] = zmq_socket (global_data.context, ZMQ_PUSH);
                zmq_setsockopt (field_data_ptr->data_pusher[j], ZMQ_SNDHWM, &field_data_ptr->local_nb_messages, sizeof(int));
                zmq_setsockopt (field_data_ptr->data_pusher[j], ZMQ_LINGER, &linger, sizeof(int));
                melissa_connect (field_data_ptr->data_pusher[j], &port_names[MPI_MAX_PROCESSOR_NAME * field_data_ptr->pull_rank[i]]);
                j += 1;
            }
        }
        if (j != field_data_ptr->local_nb_messages) // should never appen
        {
            melissa_print(VERBOSE_WARNING, "Wrong number of data pusher ports");
        }

        // we still have to connect the simulations inside a group to gather the data on sobol_rank 0 when we use COUPLING_ZMQ
        if (global_data.coupling == MELISSA_COUPLING_ZMQ && first_init != 0 && global_data.sobol == 1)
        {
            for (i=0; i<(global_data.nb_parameters+1)*comm_size; i++)
            {
                if (rank == 0)
                {
                    //
                    // send node name here.
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
        // we don't need these ports anymore
        zmq_close (global_data.connexion_requester);
        if (global_data.coupling == MELISSA_COUPLING_ZMQ)
        {
            zmq_close (master_requester);
        }
    }

    // In the case of learning, we need to allocate memory to store the incoming data before sending it to the server.
    // the buffer must be large enough to store all the data on rank 0 of sobol_rank 0
    if (global_data.sobol)
    {
        if (global_data.learning > 0)
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
        global_data.data_ptr = melissa_malloc ((global_data.nb_parameters+2) * sizeof(double*));
    }
    else
    {
        if (global_data.learning > 0)
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
        global_data.data_ptr = melissa_malloc (sizeof(double*));
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
 *******************************************************************************/

void melissa_init_f (const char *field_name,
                     int        *local_vect_size,
                     MPI_Fint   *comm_fortran)
{
    MPI_Comm comm = MPI_Comm_f2c(*comm_fortran);
    melissa_init(field_name, *local_vect_size, comm);
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
 * MPI communicator. Each rank in it must call melissa_init. Later each rank must
 * send its part of every field to melissa.
 *
 *******************************************************************************/

void melissa_init (const char *field_name,
                   const int  vect_size,
                   MPI_Comm   comm)
{
    int rank;
    int comm_size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &comm_size);

    melissa_init_internal (field_name,
                           vect_size,
                           comm_size,
                           rank,
                           comm);
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
    int     local_vect_size = 0;
    double *send_vect_ptr;
    field_data_t  *field_data_ptr = NULL;
    double start_comm_time = melissa_get_time();

#ifdef BUILD_WITH_PROBES
    double start_comm_time;
    double end_comm_time;
    start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

    // me set the field_data_ptr to the initialized field that corresponds to field_name.
    field_data_ptr = get_field_data(field_data, field_name);
    if (field_data_ptr == NULL)
    {
        // if it does not exist, then it has not be initialized.
        fprintf (stdout, "ERROR: melissa_send call before melissa_init call (%s)\n", field_name );
        raise(SIGINT);
        exit(1);
    }

    local_vect_size = field_data_ptr->local_vect_sizes[global_data.rank];
	// WARNING
	//
	// This cast serves only to silence compiler warnings.
	// The assignment is highly suspicious but not easy to fix, cf. issue #70.
    send_vect_ptr = (double *)send_vect;

    if (global_data.learning > 0)
    {
        // in the case of machine learning, we gather everything on the rank 0
        // void* buffer = melissa_malloc(sizeof(double) * 10);
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
        if (global_data.rank == 0)
        {
            local_vect_size = field_data_ptr->global_vect_size; //l ocal_vect_size is global_vect_size on rank 0
        }
        else
        {
            local_vect_size = 0; // local_vect_size is 0 everywhere else
        }
    }

    if (global_data.sobol == 1)
    {
        melissa_print(VERBOSE_DEBUG, "Group %d gather data (rank %d)\n", global_data.sample_id, global_data.rank);
        // gather data from the Sobol' group to sobol_rank 0
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

        case MELISSA_COUPLING_MPI:
            MPI_Gather(send_vect_ptr, local_vect_size, MPI_DOUBLE, global_data.buffer_data, local_vect_size, MPI_DOUBLE, 0, global_data.comm_sobol);
            break;
        }
        total_bytes_sent += local_vect_size * sizeof(double);
    }

    if (global_data.sobol_rank == 0)
    {
        // Without Sobol, the sobol_rank is always 0.
        // With Sobol, only the sobol_rank 0 sends the data to the server
        melissa_print(VERBOSE_DEBUG, "Group %d send data (timestamp %d)\n", global_data.sample_id, field_data_ptr->timestamp);
        if (global_data.learning < 2) // "classic" usage
        {
            j = 0;
            // loop over the total number of messages
            for (i=0; i<field_data_ptr->total_nb_messages; i++)
            {
                // if we are push_rank , we have to send the corresponding message. Else, we continue.
                if (global_data.rank == field_data_ptr->push_rank[i])
                {
                    // create the message
                    global_data.data_ptr[0] = &send_vect_ptr[field_data_ptr->sdispls[field_data_ptr->pull_rank[i]]];
                    if (global_data.sobol == 1)
                    {
                        // add the nb_param+1 data from the other simulations of the group in the message
                        for (k=1; k<global_data.nb_parameters + 2; k++)
                        {
                            global_data.data_ptr[k] = &global_data.buffer_data[k*local_vect_size + field_data_ptr->sdispls[field_data_ptr->pull_rank[i]]];
                        }
                        ret = send_message_simu_data (field_data_ptr->timestamp,
                                                      global_data.sample_id,
                                                      global_data.rank,
                                                      field_data_ptr->send_counts[field_data_ptr->pull_rank[i]],
                                                      global_data.nb_parameters + 2,
                                                      field_name,
                                                      global_data.data_ptr,
                                                      field_data_ptr->data_pusher[j],
                                                      0);
                        buff_size = 4 * sizeof(int) + MAX_FIELD_NAME_LEN + (global_data.nb_parameters + 2) * field_data_ptr->send_counts[field_data_ptr->pull_rank[i]] * sizeof(double);
                    }
                    else
                    {
                        ret = send_message_simu_data (field_data_ptr->timestamp,
                                                      global_data.sample_id,
                                                      global_data.rank,
                                                      field_data_ptr->send_counts[field_data_ptr->pull_rank[i]],
                                                      1,
                                                      field_name,
                                                      global_data.data_ptr,
                                                      field_data_ptr->data_pusher[j],
                                                      0);
                        buff_size = 4 * sizeof(int) + MAX_FIELD_NAME_LEN + field_data_ptr->send_counts[field_data_ptr->pull_rank[i]] * sizeof(double);
                    }
                    melissa_print(VERBOSE_DEBUG, "Message of size %d byte sent (proc %d)\n", buff_size, field_data_ptr->push_rank[i]);
                    if (ret == -1)
                    {
						die(errno);
                    }
                    j += 1;
                    total_bytes_sent += buff_size;
                }
            }
        }
        else if (global_data.rank == 0) // here, learning >= 2. That means that we d'on' split the data for redistribution, bunt we send everything to one server rank in a round-robin fashion
        {
            // remember that when learning != 0 we gather all the data on rank 0
            // send all the data round-robin from proc 0

            j = (field_data_ptr->timestamp + (global_data.sample_id % global_data.nb_proc_server)) % global_data.nb_proc_server;
            for (i=0; i<global_data.nb_proc_server; i++)
            {
                global_data.data_ptr[0] = &send_vect_ptr[0];
                if (i != j)
                {
                    ret = send_message_simu_data (field_data_ptr->timestamp,
                                                  global_data.sample_id,
                                                  global_data.rank,
                                                  0,
                                                  1,
                                                  field_name,
                                                  global_data.data_ptr,
                                                  field_data_ptr->data_pusher[i],
                                                  0);
                    buff_size = 4 * sizeof(int) + MAX_FIELD_NAME_LEN * sizeof(char);
                }
                else
                {
                    buff_size = 4 * sizeof(int) + MAX_FIELD_NAME_LEN * sizeof(char) + local_vect_size * sizeof(double);
                    if (global_data.sobol == 1)
                    {
                        for (k=1; k<global_data.nb_parameters + 2; k++)
                        {
                            global_data.data_ptr[k] = & send_vect_ptr[k*local_vect_size];
                        }
                        ret = send_message_simu_data (field_data_ptr->timestamp,
                                                      global_data.sample_id,
                                                      global_data.rank,
                                                      field_data_ptr->send_counts[field_data_ptr->pull_rank[i]],
                                                      global_data.nb_parameters + 2,
                                                      field_name,
                                                      global_data.data_ptr,
                                                      field_data_ptr->data_pusher[j],
                                                      0);
                        buff_size += local_vect_size * (global_data.nb_parameters + 1) * sizeof(double);
                    }
                    else if (global_data.learning == 0) // should not exist, we already are in a condition where learning >= 2
                    {
                        ret = send_message_simu_data (field_data_ptr->timestamp,
                                                      global_data.sample_id,
                                                      global_data.rank,
                                                      field_data_ptr->send_counts[field_data_ptr->pull_rank[i]],
                                                      1,
                                                      field_name,
                                                      global_data.data_ptr,
                                                      field_data_ptr->data_pusher[j],
                                                      0);
                    }
                    else
                    {
                        // Send the whole vector when learning > 0
                        ret = send_message_simu_data (field_data_ptr->timestamp,
                                                      global_data.sample_id,
                                                      global_data.rank,
                                                      local_vect_size,
                                                      1,
                                                      field_name,
                                                      global_data.data_ptr,
                                                      field_data_ptr->data_pusher[i],
                                                      0);
                    }
                }
                melissa_print(VERBOSE_DEBUG, "Message of size %d byte sent to %d\n", buff_size, i);
                if (ret == -1)
                {
                    die(errno);
                }
                total_bytes_sent += buff_size;
            }
        }
    }
    field_data_ptr->timestamp += 1;
    double end_comm_time = melissa_get_time();
    if (global_data.rank == 0)
    {
        melissa_print(VERBOSE_DEBUG, "Send time for step %d: %f \n", field_data_ptr->timestamp, end_comm_time - start_comm_time);
    }

    if (global_data.sobol)
    {
        for (k=1; k<global_data.nb_parameters + 2; k++)
        {
            global_data.data_ptr[k] = NULL;
        }
    }
    global_data.data_ptr[0] = NULL;
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
 * This function disconects the simulation from the statistic library
 *
 *******************************************************************************/

void melissa_finalize (void)
{
    int i;

    if (global_data.comm_size > 1)
    {
        // wait every processes here
        MPI_Barrier(global_data.comm);
    }

    field_data_t* field_data_ptr;
    for (field_data_ptr = field_data; field_data_ptr != NULL; field_data_ptr = field_data_ptr->next) {
        // Check that we got at least one timestamp per initialized field.
        assert(field_data_ptr->timestamp > 0);

        // Check that we called melissa_send for the same amount for every field
        assert(field_data_ptr->timestamp == field_data->timestamp);
    }

    // in certain cases, we have to ask the server for the permission to disconect.
#ifdef CHECK_SIMU_DECONNECTION
    if (global_data.rank == 0 && global_data.sobol_rank == 0)
    {
        zmq_msg_t  msg;
//        sleep(2);
        i = 0;
        while (i<2)
        {
            melissa_print (VERBOSE_DEBUG, "Group %d ready to disconnect \n", global_data.sample_id);
            zmq_msg_init_size (&msg, sizeof(int));
            memcpy (zmq_msg_data (&msg), &global_data.sample_id, sizeof(int));
            if (zmq_msg_send (&msg, global_data.deconnexion_requester, 0) < 0)
            {
				die(errno);
            }
            zmq_msg_close (&msg);
            melissa_print (VERBOSE_DEBUG, "Group %d waiting... \n", global_data.sample_id);
            zmq_msg_init (&msg);
            if (zmq_msg_recv (&msg, global_data.deconnexion_requester, 0) < 0)
            {
                die(errno);
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


    if (global_data.comm_size > 1)
    {
        // wait every processes here again
        MPI_Barrier(global_data.comm);
    }
#endif // CHECK_SIMU_DECONNECTION

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
    // free everything !!!
    free_field_data(field_data);
    melissa_print(VERBOSE_DEBUG, "Free ZMQ context...\n");
    zmq_ctx_term (global_data.context);
    melissa_print(VERBOSE_DEBUG, "Free ZMQ context OK\n");
    free (port_names);
    if (global_data.sobol == 1 && global_data.sobol_rank == 0)
    {
        free(global_data.buffer_data);
    }
    free(global_data.data_ptr);
#ifdef BUILD_WITH_PROBES
    melissa_print(VERBOSE_INFO, " --- Simulation comm time: %g s\n",total_comm_time);
#endif
    melissa_print(VERBOSE_INFO, " --- Bytes sent: %ld bytes\n",total_bytes_sent);
}



// The "no MPI" API is called "no MPI" because the user is not expected to call
// `MPI_Init` or to pass a valid MPI communicator. The API still requires MPI
// to be present at both compile and run time.  The name is based on
// preprocessor flags existing in the code before August 2020.
#if MELISSA_ENABLE_NO_MPI_API

void melissa_init_no_mpi(const char* field, int vector_size) {
	// check if MPI was initialized
	// Code_Saturne 6.0 does not initialize MPI when there is only one process
	// (e.g., it was called with `code_saturn run -n 1 --param case1.xml`).
	// `_p` for predicate
	int mpi_initialized_p = -1;
	MPI_Initialized(&mpi_initialized_p);

	if(!mpi_initialized_p) {
		MPI_Init(NULL, NULL);
	}

	int rank = 0;
	int comm_size = 1;
	melissa_init_internal(field, vector_size, comm_size, rank, MPI_COMM_WORLD);
}

void melissa_send_no_mpi(const char* field, const double* data) {
	melissa_send(field, data);
}

#endif
