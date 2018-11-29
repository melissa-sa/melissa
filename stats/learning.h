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
 * @file mean.h
 * @author Terraz Théophile
 * @date 2018-28-11
 *
 **/

#if BUILD_WITH_MPI == 0
#undef BUILD_WITH_MPI
#endif // BUILD_WITH_MPI

#ifndef LEARNING_H
#define LEARNING_H
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
#include <stdio.h>
//#include <tensorflow/c/c_api.h>

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

#endif // LEARNING_H
