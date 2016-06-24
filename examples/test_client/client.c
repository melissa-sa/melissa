
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#include <stats_api.h>
#endif // BUILD_WITH_MPI
#include <stats_api_no_mpi.h>

static inline void error(const int ret)
{
#ifdef BUILD_WITH_MPI
    MPI_Abort(MPI_COMM_WORLD, ret);
#else // BUILD_WITH_MPI
    exit(ret);
#endif // BUILD_WITH_MPI
}

static inline void gen_data (double       *out_vect,
                             int          *time_step,
                             int           vect_size,
                             int           rank)
{
    int i = 0;
     ++ *time_step;

    for (i=0; i<vect_size; i++)
    {
        out_vect[i] = rank;
    }
}

static inline int get_nb_param (char *name)
{
    char *name_ptr;
    int   nb_parameters = 1;

    name_ptr = name;
    while (*name_ptr != '\0')
    {
        if (*name_ptr == ':')
        {
            nb_parameters += 1;
        }
        name_ptr++;
    }
    return nb_parameters;
}

static inline void get_size_param (char *name,
                                   int  *parameters)
{
    const char  s[2] = ":";
    char       *temp_char;
    int         i;

    /* get the first token */
    temp_char = strtok (name, s);
    i = 0;

    /* walk through other tokens */
    while( temp_char != NULL )
    {
       parameters[i] = atoi (temp_char);
       i += 1;
       temp_char = strtok (NULL, s);
    }
}

int main(int argc, char **argv)
{
    int     time_step = 0, nb_time_steps;
    int    *parameters = NULL;
    double *out_vect;
    int     nb_parameters;
    int     vect_size, my_vect_size;
    int     comm_size, rank;

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size (comm, &comm_size);
    MPI_Comm_rank (comm, &rank);
#else // BUILD_WITH_MPI
    rank = 0;
    comm_size = 1;
#endif // BUILD_WITH_MPI

    if (argc < 4 || argv[1] == NULL || argv[1] == "-" || argv[1] == ":")
    {
       fprintf(stderr,"usage: %s param1:param2:...:paramN vect_size nb_time_steps\n", argv[0]);
       error(0);
    }

    nb_parameters  = get_nb_param (argv[1]);
    vect_size      = atoi (argv[2]);
    nb_time_steps  = atoi (argv[3]);

    parameters = malloc (nb_parameters * sizeof(int));
    get_size_param (argv[1], parameters);

    my_vect_size = vect_size / comm_size;
    if (rank < vect_size % comm_size)
        my_vect_size += 1;

    out_vect = calloc (my_vect_size, sizeof(double));

#ifdef BUILD_WITH_MPI
    connect_to_stats (&nb_parameters,
                      &my_vect_size,
                      &comm_size,
                      &rank,
                      &comm);
#else // BUILD_WITH_MPI
    connect_to_stats_no_mpi (&nb_parameters,
                             &my_vect_size,
                             &comm_size,
                             &rank);
#endif // BUILD_WITH_MPI

    while (time_step < nb_time_steps)
    {
        gen_data(out_vect, &time_step, my_vect_size, rank);
        sleep (1);

        send_to_stats (&time_step,
                       parameters,
                       &nb_parameters,
                       "heat",
                       out_vect,
                       &rank);
    }

    disconnect_from_stats ();

    free(parameters);
    free(out_vect);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
