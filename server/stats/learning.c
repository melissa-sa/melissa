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
 * @file mean.c
 * @brief Learning related functions.
 * @author Terraz Théophile
 * @date 2018-28-11
 *
 **/

#if BUILD_WITH_MPI == 0
#undef BUILD_WITH_MPI
#endif // BUILD_WITH_MPI

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#ifdef BUILD_WITH_OPENMP
#include <omp.h>
#endif // BUILD_WITH_OPENMP
#include "learning.h"
#include "melissa_utils.h"

/**
 *******************************************************************************
 *
 * @ingroup learning
 *
 * This function initializes a learning structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *learning
 * the learning structure to initialize
 *
 *******************************************************************************/

void init_learning (learning_t *learning)
{

}
