
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
#include <stats_api.h>


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
              int*    );

void finalize(double*,
              double*,
              int*   ,
              int*   ,
              int*   ,
              int*   ,
              double*,
              double*,
              int*    );


int main( int argc, char **argv )
{

  int    nx,ny,n,nmax,me,np,i1,in,nb_op,next,previous;
  double lx,ly,dt,dx,dy,d,t,epsilon,t1,t2,temp;
  double *u = NULL;
  double *f = NULL;
  double a[3];
  int sobol_rank = 0;
  int sobol_group = 0;
  int sobol = 0;
  MPI_Comm comm;
  char *field_name = "heat";

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&np);
  comm = MPI_COMM_WORLD;

  temp = strtod(argv[1],NULL);
  sobol_group = temp;
  if (argc > 2)
  {
    temp = strtod(argv[2],NULL);
    sobol_rank = temp;
    sobol = 1;
  }


  t1=MPI_Wtime();

  next=me+1;
  previous=me-1;

  if(next==np)     next=MPI_PROC_NULL;
  if(previous==-1) previous=MPI_PROC_NULL;

  read_file(&nx,&ny,&lx,&ly,&d);

  n = nx*ny;
  load(&me, &n, &np, &i1, &in);

  nb_op   = in-i1+1;
  dt      = 0.01;
  nmax    = 100;
  dx      = lx/(nx+1);
  dy      = ly/(ny+1);
  epsilon = 0.0001;

  u = malloc(nb_op * sizeof(double));
  f = malloc(nb_op * sizeof(double));
  init(&u[0],&i1,&in,&dx,&dy,&nx,&lx,&ly,&temp);
  filling_A (&d,&dx,&dy,&dt,&nx,&ny,&a[0]); /* fill A */

  if (sobol == 1)
  {
      connect_to_stats_sobol (&nb_op, &np, &me, &sobol_rank, &sobol_group, &comm);
  }
  else
  {
      connect_to_stats (&nb_op, &np, &me, &comm);
  }

  for(n=1;n<=nmax;n++)
  {
    t+=dt;
    filling_F (&nx,&ny,&u[0],&d,&dx,&dy,&dt,&t,&f[0],&i1,&in,&lx,&ly);
    conjgrad (&a[0],&f[0],&u[0],&nx,&ny,&epsilon,&i1,&in,&np,&me,&next,&previous);

    if (sobol == 1)
    {
        send_to_stats_sobol (&n, field_name, u, &me, &sobol_rank, &sobol_group);
    }
    else
    {
        send_to_stats (&n, field_name, u, &me);
    }
  }

  finalize (&dx,&dy,&nx,&ny,&i1,&in,&u[0],&f[0],&me);

  if (sobol == 1)
  {
      disconnect_from_stats_sobol ();
  }
  else
  {
      disconnect_from_stats ();
  }

  t2=MPI_Wtime();
  fprintf(stdout, "Calcul time: %g sec\n", t2-t1);
  fprintf(stdout, "Final time step: %g\n", t);

  MPI_Finalize();

  return 0;
}
