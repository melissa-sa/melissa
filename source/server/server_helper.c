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
