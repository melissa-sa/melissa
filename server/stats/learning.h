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
 * @file mean.h
 * @author Terraz Théophile
 * @date 2018-28-11
 *
 **/

#ifndef LEARNING_H
#define LEARNING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mpi.h>
#include <stdio.h>

/**
 *******************************************************************************
 *
 * @ingroup learning
 *
 * @struct learning_s
 *
 * Structure containing neural network data
 *
 *******************************************************************************/

struct learning_s
{
};

typedef struct learning_s learning_t; /**< type corresponding to learning_s */

#ifdef __cplusplus
}
#endif

#endif // LEARNING_H
