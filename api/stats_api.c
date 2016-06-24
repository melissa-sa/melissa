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
    void **data_requester;                    /**< send data ZeroMQ ports                                     */
    char   port_name[MPI_MAX_PROCESSOR_NAME]; /**< name of the third ZeroMQ port                              */
    int    rinit_tab[2];                      /**< array used to receive data                                 */
    int    sinit_tab[2];                      /**< array used to send data                                    */
    int    nb_proc_server;                    /**< number of MPI processes of the library                     */
    int   *server_vect_size;                  /**< local vect size for the library                            */
    char  *buffer;                            /**< buffer used to send data to the library                    */
    int    buff_size;                         /**< size of this buffer                                        */
    int   *send_counts;                       /**< number of elements to send to server rank i                */
    int   *sdispls;                           /**< displacement to which data should be sent to server rank i */
};

typedef struct zmq_data_s zmq_data_t; /**< type corresponding to zmq_data_s */

static zmq_data_t zmq_data;

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
    int *server_vect_size = data->server_vect_size;
    int  nb_proc_server = data->nb_proc_server;

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
        }
        if (server_count < server_vect_size[server_rank])
        {
            server_count += 1;
        }
        else
        {
            server_count = 1;
            server_rank += 1;
        }

        if (client_rank == rank)
        {
            data->send_counts[server_rank] += 1;
        }
    }

    data->sdispls[0] = 0;
    for (i=0; i<nb_proc_server-1; i++)
    {
        data->sdispls[i+1] = data->sdispls[i] + data->send_counts[i];
    }
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

void connect_to_stats (const int *nb_parameters,
                       int       *local_vect_size,
                       int       *comm_size,
                       int       *rank,
                       MPI_Comm  *comm)
{
    char *node_names;
    char  server_node_name[MPI_MAX_PROCESSOR_NAME];
    int   server_name_size;
    int   port_no, i;
    FILE* file = NULL;
    int  *my_vect_size;
    int   global_vect_size = 0;

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

        sprintf (zmq_data.port_name, "tcp://%s:21010\n", server_node_name);
        zmq_connect (zmq_data.connexion_requester, zmq_data.port_name);
        zmq_send (zmq_data.connexion_requester, zmq_data.sinit_tab, 2 * sizeof(int), 0);
        zmq_recv (zmq_data.connexion_requester, zmq_data.rinit_tab, 2 * sizeof(int), 0);
    }

    my_vect_size = malloc(*comm_size * sizeof(int));

    if (*comm_size > 1)
    {
        MPI_Bcast(zmq_data.rinit_tab, 2, MPI_INT, 0, *comm);
        MPI_Allgather(local_vect_size, 1, MPI_INT, my_vect_size, 1, MPI_INT, *comm);
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
        sprintf (zmq_data.port_name, "tcp://%s:21011\n", server_node_name);
        zmq_connect (zmq_data.init_requester, zmq_data.port_name);
        zmq_send (zmq_data.init_requester, my_vect_size, *comm_size * sizeof(int), 0);
        zmq_recv (zmq_data.init_requester, node_names, zmq_data.rinit_tab[0] * zmq_data.rinit_tab[1] * sizeof(char), 0);
    }

    if (*comm_size > 1)
    {
        MPI_Bcast (node_names, zmq_data.nb_proc_server * server_name_size, MPI_CHAR, 0, *comm);
    }

    zmq_data.data_requester = malloc (zmq_data.nb_proc_server * sizeof(void*));
    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_data.data_requester[i] = zmq_socket (zmq_data.context, ZMQ_REQ);
        port_no = 21012 + i;
        if (zmq_data.send_counts[i] > 0)
        {
            sprintf (zmq_data.port_name, "tcp://%s:%d\n", &node_names[server_name_size * i], port_no);
            zmq_connect (zmq_data.data_requester[i], zmq_data.port_name);
        }
    }

    zmq_data.buff_size = (*nb_parameters+2) * sizeof(int) + MAX_FIELD_NAME * sizeof(char) + zmq_data.server_vect_size[0] * sizeof(double);
    zmq_data.buffer   = malloc (zmq_data.buff_size);

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
 * @param[in] *nb_parameters
 * number of variable parameters of the parametric study
 *
 * @param[in] *local_vect_size
 * sise of the local data vector to send to the library
 *
 * @param[in] *global_vect_size
 * sise of the global data vector to send to the library
 *
 *******************************************************************************/

void connect_to_stats_no_mpi (const int *nb_parameters,
                              int       *vect_size)
{
    char *node_names;
    char  server_node_name[MPI_MAX_PROCESSOR_NAME];
    int   server_name_size;
    int   port_no, i;
    FILE* file = NULL;
    int   my_vect_size[1];
    int   global_vect_size = 0;

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

    sprintf (zmq_data.port_name, "tcp://%s:21010\n", server_node_name);
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

    sprintf (zmq_data.port_name, "tcp://%s:21011\n", server_node_name);
    zmq_connect (zmq_data.init_requester, zmq_data.port_name);
    zmq_send (zmq_data.init_requester, my_vect_size, sizeof(int), 0);
    zmq_recv (zmq_data.init_requester, node_names, zmq_data.rinit_tab[0] * zmq_data.rinit_tab[1] * sizeof(char), 0);

    zmq_data.data_requester = malloc (zmq_data.nb_proc_server * sizeof(void*));
    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_data.data_requester[i] = zmq_socket (zmq_data.context, ZMQ_REQ);
        port_no = 21012 + i;
        if (zmq_data.send_counts[i] > 0)
        {
            sprintf (zmq_data.port_name, "tcp://%s:%d\n", &node_names[server_name_size * i], port_no);
            zmq_connect (zmq_data.data_requester[i], zmq_data.port_name);
        }
    }

    zmq_data.buff_size = (*nb_parameters+2) * sizeof(int) + zmq_data.server_vect_size[0] * sizeof(double);
    zmq_data.buffer   = malloc (zmq_data.buff_size);

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

void connect_from_fortran (const int *nb_parameters,
                           int       *local_vect_size,
                           int       *comm_size,
                           int       *rank,
                           MPI_Fint  *comm_fortran)
{
    MPI_Comm comm = MPI_Comm_f2c(*comm_fortran);
    connect_to_stats(nb_parameters, local_vect_size, comm_size, rank, &comm);

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
                    const int  *parameters,
                    const int  *nb_parameters,
                    const char *field_name,
                    double     *send_vect,
                    const int  *rank)
{
    int   i, ret;
    char *buff_ptr;
    buff_ptr = zmq_data.buffer;
    memcpy(buff_ptr, time_step, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy(buff_ptr, rank, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy(buff_ptr, parameters, *nb_parameters * sizeof(int));
    buff_ptr += *nb_parameters * sizeof(int);
    memcpy (buff_ptr, field_name, MAX_FIELD_NAME);
    buff_ptr += MAX_FIELD_NAME * sizeof(char);

    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        if (zmq_data.send_counts[i] > 0)
        {
            memcpy(buff_ptr, &send_vect[zmq_data.sdispls[i]], zmq_data.send_counts[i] * sizeof(double));
            zmq_send (zmq_data.data_requester[i], zmq_data.buffer, zmq_data.buff_size, 0);
        }
    }

    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        if (zmq_data.send_counts[i] > 0)
        {
            zmq_recv (zmq_data.data_requester[i], &ret, sizeof(int), 0);
            if (ret != 0)
            {
                fprintf(stderr,"ERROR, bad server answer\n");
                error(1);
            }
        }
    }
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
                           const int  *parameters,
                           const int  *nb_parameters,
                           const char *field_name,
                           double     *send_vect)
{
    int rank = 0;
    send_to_stats (time_step,
                   parameters,
                   nb_parameters,
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
    for (i=0; i<zmq_data.nb_proc_server; i++)
    {
        zmq_close (zmq_data.data_requester[i]);
    }
    zmq_ctx_destroy (zmq_data.context);
    free(zmq_data.data_requester);
    free(zmq_data.send_counts);
    free(zmq_data.sdispls);
    free(zmq_data.server_vect_size);
    free(zmq_data.buffer);
}
