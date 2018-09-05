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
 * @file melissa_fields.c
 * @brief Routines related to the melissa_fields structure.
 * @author Terraz Théophile
 * @date 2017-01-09
 *
 * @defgroup melissa_fields Melissa fields
 *
 **/

#include <getopt.h>
#include <string.h>
#include "melissa_fields.h"
#include "melissa_data.h"
#include "melissa_io.h"
#include "compute_stats.h"

/**
 *******************************************************************************
 *
 * @ingroup melissa_fields
 *
 * This function parses field names from command line
 *
 *******************************************************************************
 *
 * @param[in] argc
 * number of Melissa Server arguments
 *
 * @param[in] **argv
 * Melissa Server arguments
 *
 * @param[in,out] *fields
 * melissa_field_t array
 *
 * @param[in] nb_fields
 * number of fields
 *
 *******************************************************************************/

void melissa_get_fields (int               argc,
                         char            **argv,
                         melissa_field_t   fields[],
                         int               nb_fields)
{
    int         opt, i;
    const char  s[2] = ":";
    char       *temp_char;
    for (i=0; i<nb_fields; i++)
    {
        fields[i].stats_data = NULL;
    }

    optind = 1;
    do
    {
        opt = getopt (argc, argv, ":f:");
        switch (opt) {
        case 'f':
            /* get the first token */
            temp_char = strtok (optarg, s);
            i = 0;
            /* walk through other tokens */
            while (temp_char != NULL)
            {
                memset(fields[i].name, '\0', MAX_FIELD_NAME);
                strncpy (fields[i].name, temp_char, MAX_FIELD_NAME);
                i += 1;

                temp_char = strtok (NULL, s);
            }
            break;
        default:
            break;
        }
    } while (opt != -1);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_fields
 *
 * This function initializes the fields after first contact with a client
 *
 *******************************************************************************
 *
 * @param[in,out] *fields
 * Melissa field array
 *
 * @param[in] data_size
 * size of client MPI communicator
 *
 * @param[in] nb_fields
 * number of fields
 *
 *******************************************************************************/

void add_fields (melissa_field_t *fields,
                int              data_size,
                int              nb_fields)
{
    int i, j;
    for (j=0; j<nb_fields; j++)
    {
        fields[j].stats_data = melissa_malloc (data_size * sizeof(melissa_data_t));
        for (i=0; i<data_size; i++)
        {
            fields[j].stats_data[i].is_valid = 0;
            fields[j].stats_data[i].vect_size = 0;
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_fields
 *
 * This function returns a field id given its name
 *
 *******************************************************************************
 *
 * @param[in] fields[]
 * Melissa field array
 *
 * @param[in] nb_fields
 * number of fields
 *
 * @param[in] *field_name
 * name of the field to find
 *
 * @retval field_id
 * the field id
 *
 *******************************************************************************/

int get_field_id(melissa_field_t fields[],
                 int             nb_fields,
                 char*           field_name)
{
    int field_id;
    if (fields != NULL)
    {
        for (field_id=0; field_id<nb_fields; field_id++)
        {
            if (strncmp(fields[field_id].name, field_name, MAX_FIELD_NAME) == 0)
            {
                return field_id;
            }
        }
    }
    else
    {
        melissa_print (VERBOSE_WARNING, fields[0].stats_data->options->verbose_lvl, "WARNING: wrong field name (get_field_id)\n");
    }
    return -1;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_fields
 *
 * This function returns a pointer to a data structure given its field name
 *
 *******************************************************************************
 *
 * @param[in] fields[]
 * Melissa field array
 *
 * @param[in] nb_fields
 * number of fields
 *
 * @param[in] *field_name
 * name of the field to find
 *
 * @retval stats_data
 * pointer to the corresponding melissa_data_t structure
 *
 *******************************************************************************/

melissa_data_t* get_data_ptr (melissa_field_t fields[],
                              int             nb_fields,
                              char*           field_name)
{
    int i;
    if (fields != NULL)
    {
        for (i=0; i<nb_fields; i++)
        {
            if (strncmp(fields[i].name, field_name, MAX_FIELD_NAME) == 0)
            {
                return fields[i].stats_data;
            }
        }
    }
    else
    {
        return NULL;
    }
    melissa_print (VERBOSE_ERROR, fields[i].stats_data->options->verbose_lvl, "ERROR: Wrong field name (get_data_ptr)\n");
    return NULL;
}

#ifdef BUILD_WITH_PROBES
/**
 *******************************************************************************
 *
 * @ingroup melissa_fields
 *
 * This function writes the data and frees the fields structure
 *
 *******************************************************************************
 *
 * @param[in] *fields
 * Melissa field array
 *
 * @param[in] *comm_data
 * Melissa communication structure
 *
 * @param[in] *options
 * Melissa options
 *
 * @param[in] *total_write_time
 * time counter
 *
 *******************************************************************************/
#else // BUILD_WITH_PROBES
/**
 *******************************************************************************
 *
 * @ingroup melissa_fields
 *
 * This function writes the data and frees the fields structure
 *
 *******************************************************************************
 *
 * @param[in] *fields
 * Melissa field array
 *
 * @param[in] *comm_data
 * Melissa communication structure
 *
 * @param[in] *options
 * Melissa options
 *
 *******************************************************************************/
#endif // BUILD_WITH_PROBES

void finalize_field_data (melissa_field_t   *fields,
                          comm_data_t       *comm_data,
                          melissa_options_t *options
#ifdef BUILD_WITH_PROBES
                          , double *total_write_time
#endif // BUILD_WITH_PROBES
                          )
{
    double start_write_time, end_write_time;
    int i, j;
    if (fields == NULL)
    {
        return;
    }
    else
    {
        for (i=0; i<comm_data->client_comm_size; i++)
        {
            for (j=0; j<options->nb_fields; j++)
            {
                if (fields[j].stats_data[i].vect_size > 0)
                {
                    finalize_stats (&fields[j].stats_data[i]);
                }
            }
        }

#ifdef BUILD_WITH_PROBES
        start_write_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
        for (j=0; j<options->nb_fields; j++)
        {
//            write_stats_bin (&(fields[j].stats_data),
//                             options,
//                             comm_data,
//                             fields[j].name);
            write_stats_txt (&(fields[j].stats_data),
                             options,
                             comm_data,
                             fields[j].name);
//            write_stats_ensight (&(fields[j].stats_data),
//                                 options,
//                                 comm_data,
//                                 fields[j].name);
        }
#ifdef BUILD_WITH_PROBES
        end_write_time = melissa_get_time();
        *total_write_time += end_write_time - start_write_time;
#endif // BUILD_WITH_PROBES


        for (j=0; j<options->nb_fields; j++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (fields[j].stats_data[i].vect_size > 0)
                {
                    melissa_free_data (&fields[j].stats_data[i]);
                }
            }
            melissa_free (fields[j].stats_data);
        }
    }
    return;
}
