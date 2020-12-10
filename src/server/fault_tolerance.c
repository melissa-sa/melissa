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
 * @file fault_tolerance.c
 * @brief Routines related to the melissa fault tolerance.
 * @author Terraz Théophile
 * @date 2017-30-06
 *
 * @defgroup melissa_fault_tolerance Melissa fault tolerance
 *
 **/

#include <melissa/server/fault_tolerance.h>
#include <melissa/messages.h>

#include <string.h>

/**
 *******************************************************************************
 *
 * @ingroup melissa_fault_tolerance
 *
 * This function allocates and return a pointer to a simulation structure
 *
 *******************************************************************************/

melissa_simulation_t* add_simulation()
{
    melissa_simulation_t *simu;

    simu = melissa_malloc (sizeof(melissa_simulation_t));

    simu->status = 0;
    simu->last_time_step = 0;
    simu->timeout = 0;
    simu->last_message = 0.0;
    sprintf (simu->job_id, "0");
    simu->job_status = -1;
    simu->parameters = NULL;

    return simu;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_fault_tolerance
 *
 * This function frees a simulation vector
 *
 *******************************************************************************
 *
 * @param[in] *v
 * pointer to the simulation vector to free
 *
 *******************************************************************************/

void free_simu_vector(vector_t v)
{
    int i;
    melissa_simulation_t *simu_ptr;

    for (i=0; i<v.size; i++)
    {
        simu_ptr = vector_get (&v, i);
        melissa_free (simu_ptr->parameters);
        simu_ptr = NULL;
        melissa_free (v.items[i]);
    }
    melissa_free (v.items);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_fault_tolerance
 *
 * This function checks if the simulations are in a timeout situation
 *
 *******************************************************************************
 *
 * @param[in] *simulations
 * pointer to the simulation vector to check
 *
 * @param[in] timeout_simu
 * time before timeout
 *
 *******************************************************************************/

int check_timeouts (vector_t *simulations,
                    int       timeout_simu)
{
    int detected_timeouts = 0;
    int i;
    double current_time = melissa_get_time();
    melissa_simulation_t *simu_ptr;

    for (i=0; i<simulations->size; i++)
    {
        simu_ptr = vector_get (simulations, i);
        if (simu_ptr->status == 1 && simu_ptr->job_status > 0)
        {
            if (simu_ptr->last_message + timeout_simu < current_time)
            {
                simu_ptr->timeout = 1;
                detected_timeouts += 1;
                melissa_print (VERBOSE_WARNING, "Timeout detected on simulation group %d\n", i);
                simu_ptr->last_message = 0;
                simu_ptr->status = 0;
            }
        }
    }
    return detected_timeouts;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_fault_tolerance
 *
 * This function sends the timeouts detected by check_timeouts
 *
 *******************************************************************************
 *
 * @param[in] *detected_timeouts
 * number of timeouts detected
 *
 * @param[in] *simulations
 * pointer to the simulation vector
 *
 * @param[in] *python_pusher
 * ZMQ socket to the python launcher
 *
 *******************************************************************************/

void send_timeouts (int       detected_timeouts,
                    vector_t *simulations,
                    void     *python_pusher)
{
    int                   i;
    melissa_simulation_t *simu_ptr;

    if (detected_timeouts < 1)
    {
        send_message_timeout(-1, python_pusher, 0);
    }
    else
    {
        for (i=0; i<simulations->size; i++)
        {
            simu_ptr = vector_get (simulations, i);
            if (simu_ptr->timeout == 1)
            {
                send_message_timeout(i, python_pusher, 0);
                simu_ptr->timeout = 0;
            }
        }
    }
}


/**
 *******************************************************************************
 *
 * @ingroup melissa_fault_tolerance
 *
 * This function counts the number of simulations in a given job status
 *
 *******************************************************************************
 *
 * @param[in] *simulations
 * pointer to the simulation vector
 *
 * @param[in] job_status
 * the job status to count
 *
 *******************************************************************************/

int count_job_status(vector_t *simulations,
                      int       job_status)
{
    melissa_simulation_t *simu_ptr;
    int                   i;
    int                   nb_simu = 0;

    for (i=0; i<simulations->size; i++)
    {
        simu_ptr = vector_get (simulations, i);
        if (simu_ptr->job_status == job_status)
        {
            nb_simu +=1;
        }
    }
    return nb_simu;
}
