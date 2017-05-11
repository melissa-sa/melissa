
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <zmq.h>
#include <melissa_api_no_mpi.h>


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
              double* );


int main( int argc, char **argv )
{

  int    nx, ny, n, nmax, nb_op;
  double lx, ly, dt, dx, dy, d, t, epsilon, t1, t2, temp = 0;
  double *u = NULL;
  double *f = NULL;
  double a[3];
  int sobol_rank = 0;
  int sobol_group = 0;
  char *field_name = "heat";

  if (argc > 1)
  {
      temp = strtod(argv[1], NULL);
  }
  if (argc > 2)
  {
      temp *= strtod(argv[2], NULL);
  }
  if (argc > 3)
  {
    sobol_rank  = (int)strtod(argv[argc-2], NULL);
    sobol_group = (int)strtod(argv[argc-1], NULL);
  }

  t1=(double)time(NULL);

  read_file(&nx,&ny,&lx,&ly,&d);

  nb_op   = nx*ny;
  dt      = 0.01;
  nmax    = 100;
  dx      = lx/(nx+1);
  dy      = ly/(ny+1);
  epsilon = 0.0001;

  u = malloc(nb_op * sizeof(double));
  f = malloc(nb_op * sizeof(double));
  init(&u[0], &nb_op, &temp);
  filling_A (&d, &dx, &dy, &dt, &a[0]); /* fill A */

  melissa_init_no_mpi(&nb_op, &sobol_rank, &sobol_group);

  for(n=1;n<=nmax;n++)
  {
    t+=dt;
    filling_F (&nx, &ny, &u[0], &d, &dx, &dy, &dt, &t, &f[0], &nb_op, &lx, &ly);
    conjgrad (&a[0], &f[0], &u[0], &nx, &ny, &epsilon);
    melissa_send_no_mpi(&n, field_name, u, &sobol_rank, &sobol_group);
  }

  finalize (&dx, &dy, &nx, &ny, &nb_op, &u[0], &f[0]);

  melissa_finalize ();

  t2=(double)time(NULL);

  fprintf(stdout, "Calcul time: %g sec\n", t2-t1);
  fprintf(stdout, "Final time step: %g\n", t);

  return 0;
}
