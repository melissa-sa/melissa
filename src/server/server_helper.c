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

#include <melissa/messages.h>
#include <melissa/server/compute_stats.h>
#include <melissa/server/data.h>
#include <melissa/server/io.h>
#include <melissa/server/options.h>
#include <melissa/server/server.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zmq.h>

int create_port_number (comm_data_t *comm_data,
                        const char*  node_name,
                        const int    start_number,
                        const int    forbidden_port1,
                        const int    forbidden_port2,
                        const int    forbidden_port3,
                        const int    forbidden_port4,
                        const int    forbidden_port5)
{
    char *node_names;
    int   i;
    int   port;

    port = start_number;

    node_names = melissa_malloc(MPI_MAX_PROCESSOR_NAME * comm_data->comm_size);

    MPI_Allgather(node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, comm_data->comm);

    for (i=0; i<comm_data->rank; i++)
    {
        if (0 == strcmp(&node_names[i * MPI_MAX_PROCESSOR_NAME], node_name))
        {
            port += 1;
            while (port == forbidden_port1
                   || port == forbidden_port2
                   || port == forbidden_port3
                   || port == forbidden_port4
                   || port == forbidden_port5)
            {
                port += 1;
            }
        }
    }

    melissa_free (node_names);

    return port;
}

/**
 * This function checks the status of a simulation
 *
 * @param[in] *fields
 * Melissa fields
 *
 * @param[in] nb_fields
 * number of Melissa fields
 *
 * @param[in] group_id
 * id of the group to check
 *
 * @param[in] nb_time_steps
 * number of time step for this study
 *
 * @param[in] *comm_data
 * Structure containing communication informations
 *
 */

int check_simu_state(melissa_field_t *fields,
                     int              nb_fields,
                     int              group_id,
                     int              nb_time_steps,
                     comm_data_t     *comm_data)
{
    int i, j, t;

    if (fields == NULL)
    {
        return 0;
    }
    else
    {
        for (j=0; j<nb_fields; j++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (fields[j].stats_data[i].steps_init > 0)
                {
                    for (t=0; t<nb_time_steps; t++)
                    {
                        if (test_bit (fields[j].stats_data[i].step_simu.items[group_id], t) == 0)
                        {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 2;
}


/**
 * This function parses messages from launcher.
 *
 * @param[in] msg_data A reference to a serialized launcher message
 * @param[in] server_ptr A reference to the server state
 */
void process_launcher_message (void*             msg_data,
                               melissa_server_t *server_ptr)
{
    vector_t             *simulations;// = &server_ptr->simulations;
    melissa_simulation_t *simu_ptr;
    int                   simu_id, i;
    char                  job_id[256];
    char*                 buff_ptr = (char*)msg_data;

    int message_type = get_message_type(buff_ptr);
    buff_ptr += sizeof(int);

    switch (message_type)
    {
    case JOB:
        simulations = &server_ptr->simulations;
        memcpy (&simu_id, buff_ptr, sizeof(int));
        buff_ptr += sizeof(int);
        if (simu_id > simulations->size)
        {
            for (i=simulations->size; i<simu_id; i++)
            {
                vector_add (simulations, add_simulation());
            }
        }
        simu_ptr = vector_get (simulations, simu_id);
        strcpy (simu_ptr->job_id, buff_ptr);
        buff_ptr += strlen(buff_ptr)+1;
        simu_ptr->job_status = 0;
        if (simu_ptr->parameters == NULL)
        {
            simu_ptr->parameters = melissa_malloc (server_ptr->melissa_options.nb_parameters * sizeof(double));
        }
        for (i=0; i<server_ptr->melissa_options.nb_parameters; i++)
        {
            memcpy (&simu_ptr->parameters[i], buff_ptr, sizeof(double));
            buff_ptr += sizeof(double);
        }
        break;
    case DROP:
        simulations = &server_ptr->simulations;
        read_message_drop (buff_ptr,
                           &simu_id,
                           job_id);
        simu_ptr = vector_get (simulations, simu_id);
        strcpy (simu_ptr->job_id, job_id);
        simu_ptr->job_status = 1;
        simu_ptr->status = 2;

        if (server_ptr->comm_data.rank == 0)
        {
            simu_ptr->status = 3;
            melissa_print(VERBOSE_INFO, "Drop simulation %d\n", simu_id);
            melissa_print(VERBOSE_INFO, "Finished simulations: %d/%d\n", server_ptr->nb_finished_simulations, server_ptr->simulations.size);
        }
        server_ptr->nb_finished_simulations += 1;
        break;
    case HELLO:
        break;
    case ALIVE:
        break;
    case OPTIONS:
        melissa_print(VERBOSE_DEBUG, " === Options (not implemented yet) ===\n");
        break;
    default:
        melissa_print(VERBOSE_WARNING, "Unknown message type: %d\n", message_type);
    }
}
