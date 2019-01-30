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

#ifdef BUILD_WITH_MPI
    MPI_Allgather(node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, comm_data->comm);
#else // BUILD_WITH_MPI
    memcpy (node_names, node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

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
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * This function checks the status of a simulation
 *
 *******************************************************************************
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
 *******************************************************************************/

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
                if (fields[j].stats_data[i].vect_size > 0)
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
 *******************************************************************************
 *
 * @ingroup melissa_fault_tolerance
 *
 * This function parses messages from launcher
 *
 *******************************************************************************
 *
 * @param[in] msg message from launcher
 * number of timeouts detected
 *
 * @param[in] *simulations
 * pointer to the simulation vector
 *
 *******************************************************************************/

void process_txt_message (char      msg[255],
                          melissa_server_t *server_ptr,
                          int       nb_param)
{
    vector_t *simulations;

    melissa_simulation_t *simu_ptr;
    int                   simu_id, i;
    const char            s[2] = " ";
    char                 *temp_char;

    simulations = &server_ptr->simulations;
    temp_char = strtok (msg, s);
    if (0 == strcmp(temp_char, "job"))
    {
        temp_char = strtok (NULL, s);
        simu_id = atoi(temp_char);
        if (simu_id > simulations->size)
        {
            for (i=simulations->size; i<simu_id; i++)
            {
                vector_add (simulations, add_simulation());
            }
        }
        simu_ptr = vector_get (simulations, simu_id);
        temp_char = strtok (NULL, s);
        strcpy(simu_ptr->job_id, temp_char);
        simu_ptr->job_status = 0;
        if (simu_ptr->parameters == NULL)
        {
            simu_ptr->parameters = melissa_malloc (nb_param * sizeof(double));
        }
        temp_char = strtok (NULL, s);
        i = 0;
        while( temp_char != NULL )
        {
            simu_ptr->parameters[i] = atof(temp_char);
            i++;
            temp_char = strtok (NULL, s);
        }
    }
    else if (0 == strcmp(temp_char, "drop"))
    {
        temp_char = strtok (NULL, s);
        simu_id = atoi(temp_char);
        simu_ptr = vector_get (simulations, simu_id);
        temp_char = strtok (NULL, s);
        strcpy(simu_ptr->job_id, temp_char);
        simu_ptr->job_status = 1;
        simu_ptr->status = 2;

        if (server_ptr->comm_data.rank != 0)
        {
            simu_ptr->status = 3;
            server_ptr->nb_finished_simulations += 1;
            melissa_print(VERBOSE_INFO, "Drop simulation %d\n", simu_id);
            melissa_print(VERBOSE_INFO, "Finished simulations: %d/%d\n", server_ptr->nb_finished_simulations, server_ptr->simulations.size);

        }
    }
}

int check_last_timestep(melissa_field_t *fields,
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
                if (fields[j].stats_data[i].vect_size > 0)
                {
                    if (test_bit (fields[j].stats_data[i].step_simu.items[group_id], nb_time_steps-1) == 0)
                    {
                        return 1;
                    }
                }
            }
        }
    }
    return 2;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * This function counts the amount of data writen on disk
 *
 *******************************************************************************
 *
 * @param[in] *options
 * Melissa options structure
 *
 *******************************************************************************/

long int count_mbytes_written (melissa_options_t  *options)
{
    long int mbytes_written = 0;
//    if (options->mean_op == 1)
//    {
//        mbytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
//    }
//    if (options->variance_op == 1)
//    {
//        mbytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
//    }
//    if (options->min_and_max_op == 1)
//    {
//        mbytes_written += 2*options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
//    }
//    if (options->threshold_op == 1)
//    {
//        mbytes_written += options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
//    }
//    if (options->sobol_op == 1)
//    {
//        mbytes_written += options->nb_parameters * 2 *options->global_vect_size*sizeof(float)*options->nb_time_steps/1000000;
//    }
    return mbytes_written;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * This function recieves a string from the launcher
 *
 *******************************************************************************
 *
 * @param[in] *socket
 * ZMQ socket
 *
 * @param[in, out] *recv_buff
 * the recieve buffer
 *
 *******************************************************************************/

int string_recv (void  *socket,
                 char  *recv_buff)
{
    char buffer [MELISSA_MESSAGE_LEN];
    int size = zmq_recv (socket, buffer, MELISSA_MESSAGE_LEN-1, 0);
    if (size == -1)
    {
        recv_buff[0] = 0;
        return 0;
    }
    if (size > MELISSA_MESSAGE_LEN-1)
        size = MELISSA_MESSAGE_LEN-1;
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
            data = &(field[f].stats_data[i]);
            if (data->vect_size > 0)
            {
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
