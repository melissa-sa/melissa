// TODO: do we need this?#include <netcdf_par.h>
// TODO: rename to melissa_output_parflow_netcdf.c as we get netcdf specific
#include <netcdf.h>
#include <unistd.h>

// for debugging:
#include <signal.h>


#include "melissa_data.h"
#include "melissa_utils.h"

#ifdef __DEBUG  // TODO: check if this is the correct debug symbol
#define D(x ...) printf("++++++++:"); printf(x); printf(" %s:%d\n", __FILE__, __LINE__)
#else
#define D(...)
#endif

void ERROR(const char * message)
{
  printf("ERROR! %s\n", message);
  raise(SIGINT);
  exit(1);
}

/**
 * Creates dimension or only returns its id if already existent.
 */
int getDim(int ncID, const char *name, size_t len, int *idp)
{
  int res = nc_def_dim(ncID, name, len, idp);

  if (res == NC_ENAMEINUSE)
  {
    res = nc_inq_varid(ncID, name, idp);
  }
  return res;
}

/**
 * Creates variables or only returns its id if already existent.
 */
void getVar(int ncID, const char *name, int ndims, const int dimids[], int *idp)
{
  int res = nc_def_var(ncID, name, NC_DOUBLE, ndims, dimids, idp);

  if (res == NC_ENAMEINUSE)
  {
    res = nc_inq_varid(ncID, name, idp);
    if (res != NC_NOERR)
    {
      ERROR("Could not find variable");
    }
  }
  if (res != NC_NOERR)
  {
    printf("Netcdf Error code: %d", res);
    ERROR("Could not get variable");
  }
}

/**
 * Opens/ creates file if not already there
 */
int CreateFile(const char* file_name, size_t nX, size_t nY, size_t nZ, int *pxID, int *pyID, int *pzID, int *ptime_id, comm_data_t *comm_data)
{
  int ncID = 0;

  if (access(file_name, F_OK) == -1)
  {
    // TODO parallel file opening if sequential is to slow?
    // TODO: race condition? who opens it first?
    //#ifdef BUILD_WITH_MPI
    //int status = nc_create_par(file_name, NC_NETCDF4 | NC_MPIIO, comm_data->comm, MPI_INFO_NULL, &ncID);
    //#else
    //int status = nc_create(file_name, NC_NETCDF4, &ncID);
    int status = nc_create(file_name, 0, &ncID);
    //#endif
    if (status != NC_NOERR)
    {
      printf("Error! nc error code: %d", status);
      printf("Error creating file!");
      ERROR("Could not create file!");
    }
  }
  else
  {
    // open it
    D("file exists already. Opening it!");
    // TODO: use status as above to simplify debugging!
    //#ifdef BUILD_WITH_MPI
    //if (nc_open_par(file_name, NC_MPIIO | NC_WRITE, comm_data->comm, MPI_INFO_NULL, &ncID) != NC_NOERR)
    //#else
    int status = nc_open(file_name, NC_WRITE, &ncID);
    if (status!= NC_NOERR)
      //#endif
    {
      printf("Error! nc error code: %d", status);
      printf("Error opening existing file!");
      ERROR("Could not open file!");
    }
  }

  // create file. if it exists already, just load it.

  // add/get Dimensions
  getDim(ncID, "x", nX, pxID);
  getDim(ncID, "y", nY, pyID);
  getDim(ncID, "z", nZ, pzID);
  getDim(ncID, "time", NC_UNLIMITED, ptime_id);


  return ncID;
}

// TODO: reuse those functions!
// Accessor functions:
// maybe function pointers and inline do not work together!
void accessor_mean(double *dest, melissa_data_t *cur, int t)
{
  memcpy(dest, cur->moments[t].m1, cur->vect_size*sizeof(double));
}
void accessor_variance(double *dest, melissa_data_t *cur, int t)
{
  memcpy(dest, cur->moments[t].theta2, cur->vect_size*sizeof(double));
}
void accessor_skewness(double *dest, melissa_data_t *cur, int t)
{
  compute_skewness(&cur->moments[t], dest, cur->vect_size);
}
void accessor_kurtosis(double *dest, melissa_data_t *cur, int t)
{
  compute_kurtosis(&cur->moments[t], dest, cur->vect_size);
}
void accessor_min(double *dest, melissa_data_t *cur, int t)
{
  memcpy(dest, cur->min_max[t].min, cur->vect_size*sizeof(double));
}
void accessor_max(double *dest, melissa_data_t *cur, int t)
{
  memcpy(dest, cur->min_max[t].max, cur->vect_size*sizeof(double));
}
void accessor_threshold(double *dest, melissa_data_t *cur, int t)
{
  memcpy(dest, cur->thresholds[t], cur->vect_size*sizeof(double));
}
void accessor_quantile(double *dest, melissa_data_t *cur, int t)
{
  // TODO: don't use this dirty hack:
  int value = (int) *dest;
  memcpy(dest, cur->quantiles[t][value].quantile, cur->vect_size*sizeof(double));
}
void accessor_sobol(double *dest, melissa_data_t *cur, int t)
{
  // TODO: don't use this dirty hack:
  int value = (int) *dest;
  memcpy(dest, cur->sobol_indices[t].sobol_martinez[value].first_order_values, cur->vect_size*sizeof(double));
}
void accessor_sobol_tot(double *dest, melissa_data_t *cur, int t)
{
  // TODO: don't use this dirty hack:
  int value = (int) *dest;
  memcpy(dest, cur->sobol_indices[t].sobol_martinez[value].total_order_values, cur->vect_size*sizeof(double));
}

inline void writeVar(const char         *variable_name,
    const char         *statistics_name,
    const int           dest_init_value,
    const int           is_first_time,
    void (*data_accessor)(double *dest, melissa_data_t *data, int t),
    double 	           *d_buffer,
    int                *local_vect_sizes,
    melissa_data_t    **data,
    melissa_options_t  *options,
    comm_data_t        *comm_data)
{
#ifdef BUILD_WITH_MPI
  MPI_Status  status;
#endif // BUILD_WITH_MPI
  // TODO: use values from a confiuration file!
  int nX = 4;
  int nY = 5;
  int nZ = 10;

  // iterators:
  long int i, j;
  int t;

  // the following variables are currently just used by rank 0

  int current_file_id;
  int xID, yID, zID, time_id;

  int variable_var_id;
  int time_var_id;

  char file_name[256];

  if (comm_data->rank == 0) {
    sprintf(file_name, "%s_%s.nc", variable_name, statistics_name);

    current_file_id = CreateFile(file_name, nX, nY, nZ, &xID, &yID, &zID, &time_id, comm_data);
    // add variable Time and variable "variable":
    //nc_def_var(current_file_id, "time", NC_DOUBLE, 1, &time_id, &time_var_id);
    size_t time_dims[1];
    time_dims[0] = time_id;
    getVar(current_file_id, "time", 1, &time_id, &time_var_id);
    int variable_dims[4] = { time_id, zID, yID, xID }; // low: why in this order? I guess so it will count in the right order ;)
    getVar(current_file_id, variable_name, 4, variable_dims, &variable_var_id);
    // required for compatibility with netcdf3 :
    nc_enddef(current_file_id);

    // set time only once per file!
    size_t start[1], count[1];
    if (is_first_time) {
      //nc_var_par_access(current_file_id, time_var_id, NC_COLLECTIVE);
      start[0] = 0;
      count[0] = options->nb_time_steps;  // writing one value
      double times[options->nb_time_steps];
      for (t=0; t < options->nb_time_steps; ++t) {
        times[t] = t;
      }
      int nc_status = nc_put_vara_double(current_file_id, time_var_id, start, count, times);
    }

    //nc_var_par_access(current_file_id, variable_var_id, NC_COLLECTIVE);
  }

  for (t=0; t<options->nb_time_steps; t++)
  {
    // refresh all the values of time step t:
    //                  t  z  y  x
    size_t start[4] = { t, 0, 0, 0 };
    size_t count[4] = { 1, nZ, nY, nX};
    long int temp_offset = 0;
    // now do a write to the cdf! (! all the preparations up there are necessary!)
    for (i=0; i<comm_data->client_comm_size; i++)
    {
      melissa_data_t *current_data = &(*data[i]);
      if (current_data->vect_size > 0)
      {
        // Additional parameter to accessor function
        d_buffer[temp_offset] = (double) dest_init_value;
        (*data_accessor)(&d_buffer[temp_offset], current_data, t);
        temp_offset += current_data->vect_size;
      }
    }
    temp_offset = 0;
#ifdef BUILD_WITH_MPI
    if (comm_data->rank == 0)
    {
      for (j=1; j<comm_data->comm_size; j++)
      {
        temp_offset += local_vect_sizes[j-1];
        MPI_Recv (&(d_buffer[temp_offset]), local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
      }
      temp_offset = 0;
    }
    else
    {
      MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
    }
#endif // BUILD_WITH_MPI
    if (comm_data->rank == 0)
    {
      // TODO: parallelize? here just rank 0 is writing at the end!
      int nc_status = nc_put_vara_double(current_file_id, variable_var_id, start, count, d_buffer);
      D("putting doubles");
    }
  }

  if (comm_data->rank == 0)
  {
    nc_close(current_file_id);
    D("wrote %s\n", file_name);
  }
}
/**
 *******************************************************************************
 *
 * @ingroup melissa_output
 *
 * This function writes the computed statistics on files
 *
 *******************************************************************************
 *
 * @param[in] **data
 * pointer to the array of structures containing statistics data
 *
 * @param[in] *options
 * Melissa option structure
 *
 * @param[in] comm_data
 * structure containing communications parameters
 *
 * @param[in] *field
 * name of the statistic field to write
 *
 *******************************************************************************/
// for testing:
void write_stats_txt (melissa_data_t    **data,
    melissa_options_t  *options,
    comm_data_t        *comm_data,
    char               *field);

void write_stats_parflow_netcdf (melissa_data_t    **data,
    melissa_options_t  *options,
    comm_data_t        *comm_data,
    char               *field)
{
  // for testing:
  write_stats_txt(data, options, comm_data, field);
#ifdef BUILD_WITH_MPI
  MPI_Status  status;
#endif // BUILD_WITH_MPI
  long int    i;
  int        *offsets;
  int         p;
  FILE*       f;
  char        file_name[256];
  int         max_size_time;
  int         vect_size = 0;
  int        *local_vect_sizes;
  int         global_vect_size = 0;
  int        *i_buffer;
  double     *d_buffer;

  static int is_first_time = 1;
  //    double temp1, temp2;

  max_size_time=floor(log10(options->nb_time_steps))+1;

  // ================= first test =============== //
  //                                              //
  // Communicate local vect size to every process //
  //                                              //
  // ============================================ //

  local_vect_sizes = melissa_calloc (comm_data->comm_size, sizeof(int));
  offsets = melissa_calloc (comm_data->comm_size, sizeof(int));
  for (i=0; i<comm_data->client_comm_size; i++)
  {
    vect_size += (*data)[i].vect_size;
  }
#ifdef BUILD_WITH_MPI
  MPI_Allgather (&vect_size, 1, MPI_INT, local_vect_sizes, 1, MPI_INT, comm_data->comm);
#endif // BUILD_WITH_MPI

  for (i=0; i<comm_data->comm_size-1; i++)
  {
    offsets[i+1] = offsets[i] + local_vect_sizes[i];
    global_vect_size += local_vect_sizes[i];
  }
  global_vect_size += local_vect_sizes[comm_data->comm_size-1];

  // ============================================ //

  d_buffer = melissa_malloc (global_vect_size * sizeof(double));

  // Refactor: put all those in a list!
  if (options->mean_op == 1)
  {
    writeVar(field, "mean", NULL, is_first_time, accessor_mean, d_buffer, local_vect_sizes, data, options, comm_data);
  }

  if (options->variance_op == 1)
  {
    writeVar(field, "variance", NULL, is_first_time, accessor_variance, d_buffer, local_vect_sizes, data, options, comm_data);
  }

  if (options->skewness_op == 1)
  {
    writeVar(field, "skewness", NULL, is_first_time, accessor_skewness, d_buffer, local_vect_sizes, data, options, comm_data);
  }

  if (options->kurtosis_op == 1)
  {
    writeVar(field, "kurtosis", NULL, is_first_time, accessor_kurtosis, d_buffer, local_vect_sizes, data, options, comm_data);
  }

  if (options->min_and_max_op == 1)
  {
    writeVar(field, "min", NULL, is_first_time, accessor_min, d_buffer, local_vect_sizes, data, options, comm_data);
    writeVar(field, "max", NULL, is_first_time, accessor_max, d_buffer, local_vect_sizes, data, options, comm_data);
  }

  if (options->threshold_op == 1)
  {
    writeVar(field, "threshold", NULL, is_first_time, accessor_threshold, d_buffer, local_vect_sizes, data, options, comm_data);
  }

  if (options->quantile_op == 1)
  {
    for (p = 0; p < options->nb_quantiles; ++p)
    {
      char variable_name[256];
      sprintf(variable_name, "quantile%g", options->quantile_order[p]);
      writeVar(field, variable_name, p, is_first_time, accessor_quantile, d_buffer, local_vect_sizes, data, options, comm_data);
    }
  }

  if (options->sobol_op == 1)
  {
    for (p=0; p<options->nb_parameters; p++)
    {
      char variable_name[256];
      sprintf(variable_name, "sobol%d", p);
      writeVar(field, variable_name, p, is_first_time, accessor_sobol, d_buffer, local_vect_sizes, data, options, comm_data);
      sprintf(variable_name, "sobol_tot%d", p);
      writeVar(field, variable_name, p, is_first_time, accessor_sobol_tot, d_buffer, local_vect_sizes, data, options, comm_data);
    }
  }

  melissa_free (d_buffer);
  melissa_free (offsets);
  melissa_free (local_vect_sizes);
  is_first_time = 0;
}
