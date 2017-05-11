
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include "melissa_options.h"


int main (int argc, char **argv)
{
    melissa_options_t options;
    int vect_size, i, j, t, offset = 0;
    int m = 100, n = 100;
    int comm_size, rank;
    FILE* f;
    MPI_File mpi_f;
    char file_name[256];
    char field[] = "heat";
    int max_size_time;
    double *out_vect;
    MPI_Status status;
    MPI_Comm comm;

    MPI_Init (&argc, &argv);
    comm= MPI_COMM_WORLD;
    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank (comm, &rank);

    melissa_get_options (argc, argv, &options);

    vect_size = m * n;
    printf("m * n = %d * %d = %d\n",m,n,vect_size);

    out_vect = melissa_malloc (vect_size * sizeof(double));

    max_size_time=floor(log10(options.nb_time_steps))+1;

    if (options.mean_op == 1)
    {
        for (t=0; t<options.nb_time_steps; t++)
        {
            sprintf(file_name, "%s_mean_%.*d", field, max_size_time, t+1);
            MPI_File_open (comm, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_f);
            MPI_File_read_at (mpi_f, offset, out_vect, vect_size, MPI_DOUBLE, &status);
            MPI_File_close (&mpi_f);

            sprintf(file_name, "%s_mean_%.*d.dat", field, max_size_time, t+1);
            f = fopen(file_name, "w");
            for (i=0; i<m; i++)
            {
                for (j=0; j<n; j++)
                {
                    fprintf(f, "%d %d %g\n", i, j, out_vect[i]);
                }
            }
            fclose(f);
        }
    }

    if (options.variance_op == 1)
    {
        for (t=0; t<options.nb_time_steps; t++)
        {
            sprintf(file_name, "%s_variance_%.*d", field, max_size_time, t+1);
            MPI_File_open (comm, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_f);
            MPI_File_read_at (mpi_f, offset, out_vect, vect_size, MPI_DOUBLE, &status);
            MPI_File_close (&mpi_f);

            sprintf(file_name, "%s_variance_%.*d.dat", field, max_size_time, t+1);
            f = fopen(file_name, "w");
            for (i=0; i<m; i++)
            {
                for (j=0; j<n; j++)
                {
                    fprintf(f, "%d %d %g\n", i, j, out_vect[i]);
                }
            }
            fclose(f);
        }
    }

    if (options.min_and_max_op == 1)
    {
        for (t=0; t<options.nb_time_steps; t++)
        {
            sprintf(file_name, "%s_min_%.*d", field, max_size_time, t+1);
            MPI_File_open (comm, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_f);
            MPI_File_read_at (mpi_f, offset, out_vect, vect_size, MPI_DOUBLE, &status);
            MPI_File_close (&mpi_f);

            sprintf(file_name, "%s_min_%.*d.dat", field, max_size_time, t+1);
            f = fopen(file_name, "w");
            for (i=0; i<m; i++)
            {
                for (j=0; j<n; j++)
                {
                    fprintf(f, "%d %d %g\n", i, j, out_vect[i]);
                }
            }
            fclose(f);
        }

        for (t=0; t<options.nb_time_steps; t++)
        {
            sprintf(file_name, "%s_max_%.*d", field, max_size_time, t+1);
            MPI_File_open (comm, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_f);
            MPI_File_read_at (mpi_f, offset, out_vect, vect_size, MPI_DOUBLE, &status);
            MPI_File_close (&mpi_f);

            sprintf(file_name, "%s_max_%.*d.dat", field, max_size_time, t+1);
            f = fopen(file_name, "w");
            for (i=0; i<m; i++)
            {
                for (j=0; j<n; j++)
                {
                    fprintf(f, "%d %d %g\n", i, j, out_vect[i]);
                }
            }
            fclose(f);
        }
    }

    free (out_vect);
    MPI_Finalize();
    return 0;
}
