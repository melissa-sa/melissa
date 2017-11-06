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
 * @file melissa_fields.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef MELISSA_FIELDS_H
#define MELISSA_FIELDS_H

#include "melissa_data.h"

/**
 *******************************************************************************
 *
 * @struct melissa_field_s
 *
 * Field structure
 *
 *******************************************************************************/

struct melissa_field_s /**< Structure for a linked list of output fields */
{
    char            name[MAX_FIELD_NAME]; /**< name of the field                                */
    melissa_data_t *stats_data;           /**< stats_data structure                             */
//    struct field_s *next;                 /**< pointer to the next field structure              */
};

typedef struct melissa_field_s melissa_field_t; /**< type corresponding to field_s */

void melissa_get_fields (int               argc,
                         char            **argv,
                         melissa_field_t   fields[],
                         int               nb_fields);

void add_fields (melissa_field_t *fields,
                 int              data_size,
                 int              nb_fields);

melissa_data_t* get_data_ptr (melissa_field_t fields[],
                              int             nb_fields,
                              char*           field_name);

void finalize_field_data (melissa_field_t   *fields,
                          comm_data_t       *comm_data,
                          melissa_options_t *options,
                          int               *local_vect_sizes
#ifdef BUILD_WITH_PROBES
                          , double *write_time
#endif // BUILD_WITH_PROBES
                          );

#endif // MELISSA_FIELDS_H
