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
 * @file melissa/server/fields.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef MELISSA_FIELDS_H
#define MELISSA_FIELDS_H

#include <melissa/server/data.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    char            name[MAX_FIELD_NAME_LEN+1]; /**< name of the field                                       */
    melissa_data_t *stats_data;           /**< stats_data structure                                    */
    int            *client_vect_sizes;    /**< client vector sizes for this field for each client rank */
};

typedef struct melissa_field_s melissa_field_t; /**< type corresponding to field_s */

//#if MELISSA4PY != 1
//void melissa_get_output_lib(char* lib_name,
//                            char* func_name);
//#endif // MELISSA4PY

void melissa_get_fields (int               argc,
                         char            **argv,
                         melissa_field_t   fields[],
                         int               nb_fields);

void add_fields (melissa_field_t *fields,
                 int              data_size,
                 int              nb_fields);

int get_field_id(melissa_field_t fields[],
                 int             nb_fields,
                 char*           field_name);

melissa_data_t* get_data_ptr (melissa_field_t fields[],
                              int             nb_fields,
                              char*           field_name);

void finalize_field_data (melissa_field_t   *fields,
                          comm_data_t       *comm_data,
                          melissa_options_t *options,
                          double *write_time);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_FIELDS_H
