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
#include <unistd.h>
#include <sys/timeb.h>
#include <mpi.h>

// Fortran interfaces:

void read_file(int*   ,
               int*   ,
               double*,
               double*,
               double*);

void load(int*,
          int*,
          int*,
          int*,
          int* );

void init(double*,
          int*   ,
          int*   ,
          double*,
          double*,
          int*   ,
          double*,
          double*,
          double* );

void filling_A(double*,
               double*,
               double*,
               double*,
               int*   ,
               int*   ,
               double* );

void filling_F(int*   ,
               int*   ,
               double*,
               double*,
               double*,
               double*,
               double*,
               double*,
               double*,
               int*   ,
               int*   ,
               double*,
               double*,
               double* );

void conjgrad(double*,
              double*,
              double*,
              int*   ,
              int*   ,
              double*,
              int*   ,
              int*   ,
              int*   ,
              int*   ,
              int*   ,
              int*   ,
              int*    );

void finalize(double*,
              double*,
              int*   ,
              int*   ,
              int*   ,
              int*   ,
              double*,
              double*,
              int*   ,
              int*    );

int main( int argc, char **argv )
{

  int    nx, ny, n, nmax, me, np, i1, in, vect_size, next, previous;
  double lx, ly, dt, dx, dy, d, t, epsilon, t1, t2, temp;
  double *u = NULL;
  double *f = NULL;
  double a[3];
  double param[5];
  MPI_Comm comm;
  int fcomm;
  struct timeb tp;

  MPI_Init(&argc, &argv);

  // The program takes at least one parameter: the initial temperature
  if (argc < 2)
  {
      fprintf (stderr, "Missing parameter");
      return -1;
  }
  // The initial temperature is stored in param[0]
  // The four next optional parameters are the boundary temperatures
  for (n=0; n<5; n++)
  {
    param[n] = 0;
    if (argc > n+1)
    {
       param[n] = strtod(argv[n+1], NULL);
    }
  }
  temp = param[0];

  // The new MPI communicator, process rank and communicator size
  MPI_Comm_dup(MPI_COMM_WORLD, &comm);
  MPI_Comm_rank(comm, &me);
  MPI_Comm_size(comm, &np);
  fcomm = MPI_Comm_c2f(comm);

  // Init timer
  ftime(&tp);
  t1 = (double)tp.time + (double)tp.millitm / 1000;

  // Neighbour ranks
  next = me+1;
  previous = me-1;

  if(next == np)     next=MPI_PROC_NULL;
  if(previous == -1) previous=MPI_PROC_NULL;

  nx        = 100; // x axis grid subdivisions
  ny        = 100; // y axis grid subdivisions
  lx        = 10.0; // x length
  ly        = 10.0; // y length
  d         = 1.0; // diffusion coefficient
  dt        = 0.01; // timestep value
  nmax      = 100; // number of timesteps
  dx        = lx/(nx+1); // x axis step
  dy        = ly/(ny+1); // y axis step
  epsilon   = 0.0001;  // conjugated gradient precision
  n         = nx*ny; // number of cells in the drid

  // work repartition over the MPI processes
  // i1 and in: first and last global cell indices atributed to this process
  load(&me, &n, &np, &i1, &in);

  // local number of cells
  vect_size = in-i1+1;

  // initialization
  u = malloc(vect_size * sizeof(double));
  f = malloc(vect_size * sizeof(double));
  // we will solve Au=F
  init(&u[0], &i1, &in, &dx, &dy, &nx, &lx, &ly, &temp);
  // init A (tridiagonal matrix):
  filling_A (&d, &dx, &dy, &dt, &nx, &ny, &a[0]);

  // main loop:
  for(n=1; n<=nmax; n++)
  {
    t+=dt;
    // filling F (RHS) before each iteration:
    filling_F (&nx, &ny, &u[0], &d, &dx, &dy, &dt, &t, &f[0], &i1, &in, &lx, &ly, &param[0]);
    // conjugated gradient to solve Au = F.
    conjgrad (&a[0], &f[0], &u[0], &nx, &ny, &epsilon, &i1, &in, &np, &me, &next, &previous, &fcomm);
    // The result is u
    fprintf(stdout, "Time step: %g (%d)\n", t, n);
  }
  
  n = 0;
  // write results on disk
  finalize (&dx, &dy, &nx, &ny, &i1, &in, &u[0], &f[0], &me, &n);

  // end timer
  ftime(&tp);
  t2 = (double)tp.time + (double)tp.millitm / 1000;

  fprintf(stdout, "Calcul time: %g sec\n", t2-t1);

  MPI_Finalize();

  return 0;
}
