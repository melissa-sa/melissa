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
#include <melissa_api_no_mpi.h>

// Fortran interfaces:

void read_file(int*   ,
               int*   ,
               double*,
               double*,
               double* );

void init(double*,
          int*   ,
          double* );

void filling_A(double*,
               double*,
               double*,
               double*,
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
               double*,
               double*,
               double* );

void conjgrad(double*,
              double*,
              double*,
              int*   ,
              int*   ,
              double* );

void finalize(double*,
              double*,
              int*   ,
              int*   ,
              int*   ,
              double*,
              double*,
              int*    );


int main( int argc, char **argv )
{

  int    nx, ny, n, nmax, vect_size;
  double lx, ly, dt, dx, dy, d, t, epsilon, t1, t2;
  double *u = NULL;
  double *f = NULL;
  double a[3];
  double param[5];
  struct timeb tp;
  char *field_name = "heat1";

  // The program now takes at least 2 parameter:
  // - The coupling method for Sobol' groups
  // - the initial temperature

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

  // Init timer
  ftime(&tp);
  t1 = (double)tp.time + (double)tp.millitm / 1000;

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
  vect_size = nx*ny; // number of cells in the drid

  // initialization
  u = malloc(vect_size * sizeof(double));
  f = malloc(vect_size * sizeof(double));
  // we will solve Au=F
  init(&u[0], &vect_size, &param[0]);
  // init A (tridiagonal matrix):
  filling_A (&d, &dx, &dy, &dt, &a[0]);

  // melissa_init_no_mpi is the first Melissa function to call, and it is called only once.
  // It mainly contacts the server.
  melissa_init_no_mpi(field_name, vect_size);

  // main loop:
  for(n=1;n<=nmax;n++)
  {
    t+=dt;
    // filling F (RHS) before each iteration:
    filling_F (&nx, &ny, &u[0], &d, &dx, &dy, &dt, &t, &f[0], &vect_size, &lx, &ly, &param[0]);
    // conjugated gradient to solve Au = F.
    conjgrad (&a[0], &f[0], &u[0], &nx, &ny, &epsilon);
    // The result is u
    // melissa_send_no_mpi is called at each iteration to send u to the server.
    melissa_send_no_mpi(field_name, u);
  }

  // melissa_finalize closes the connexion with the server.
  // No Melissa function should be called after melissa_finalize.
  melissa_finalize ();

  // end timer
  ftime(&tp);
  t2 = (double)tp.time + (double)tp.millitm / 1000;

  fprintf(stdout, "Calcul time: %g sec\n", t2-t1);
  fprintf(stdout, "Final time step: %g\n", t);

  free(u);
  free(f);

  return 0;
}
