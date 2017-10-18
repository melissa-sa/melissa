
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <mpi.h>

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

  if (argc < 2)
  {
      fprintf (stderr, "Missing parameter");
      return -1;
  }
  param[0] = strtod(argv[1], NULL);

  for (n=0; n<5; n++)
    param[n+1] = 0;
    if (argc > n+1)
    {
       param[n] = strtod(argv[n+1], NULL);
    }
  temp = param[0];

  MPI_Comm_dup(MPI_COMM_WORLD, &comm);
  MPI_Comm_rank(comm, &me);
  MPI_Comm_size(comm, &np);
  fcomm = MPI_Comm_c2f(comm);


  ftime(&tp);
  t1 = (double)tp.time + (double)tp.millitm / 1000;

  next = me+1;
  previous = me-1;

  if(next == np)     next=MPI_PROC_NULL;
  if(previous == -1) previous=MPI_PROC_NULL;

  nx        = 100;
  ny        = 100;
  lx        = 10.0;
  ly        = 10.0;
  d         = 1.0;
  dt        = 0.01;
  nmax      = 100;
  dx        = lx/(nx+1);
  dy        = ly/(ny+1);
  epsilon   = 0.0001;
  n         = nx*ny;

  load(&me, &n, &np, &i1, &in);

  vect_size = in-i1+1;

  u = malloc(vect_size * sizeof(double));
  f = malloc(vect_size * sizeof(double));
  init(&u[0], &i1, &in, &dx, &dy, &nx, &lx, &ly, &temp);
  filling_A (&d, &dx, &dy, &dt, &nx, &ny, &a[0]); /* fill A */

  for(n=1; n<=nmax; n++)
  {
    t+=dt;
    filling_F (&nx, &ny, &u[0], &d, &dx, &dy, &dt, &t, &f[0], &i1, &in, &lx, &ly, &param[0]);
    conjgrad (&a[0], &f[0], &u[0], &nx, &ny, &epsilon, &i1, &in, &np, &me, &next, &previous, &fcomm);
    fprintf(stdout, "Time step: %g (%d)\n", t, n);
  }
  
  n = 0;
  finalize (&dx, &dy, &nx, &ny, &i1, &in, &u[0], &f[0], &me, &n);

  ftime(&tp);
  t2 = (double)tp.time + (double)tp.millitm / 1000;

  fprintf(stdout, "Calcul time: %g sec\n", t2-t1);

  MPI_Finalize();

  return 0;
}
