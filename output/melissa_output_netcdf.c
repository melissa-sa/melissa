#include "melissa_output.h"
#include <netcdf.h>
#include <unistd.h>

// for debugging:
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

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
 * Format must be:
 * export MELISSA_NETCDF_DIMENSIONS="<dimlen z>;<dimlen y>;<dimlen x>;"
 * export MELISSA_NETCDF_DIMENSIONS="<dimlen y>;<dimlen x>;"
 * ...
 * For one dimensiont MELISSA_NETCDF_DIMENSIONS can left unset.
 */
void getDimsFromEnv(size_t * dims, size_t * n)
{
    // returns the number of dims
    // in n the size of each dimension can be found!
    *dims = 0;
    char dim_str_cp[256];
    char * dim_str = dim_str_cp;
    strcpy(dim_str, getenv("MELISSA_NETCDF_DIMENSIONS"));
    if (dim_str == NULL)
    {
        *dims = 1;
        return;
    }

    char * old = dim_str;
    for (; *dim_str != '\0'; ++dim_str)
    {
        if (*dim_str == ';')
        {
            *dim_str = '\0';
            n[*dims] = atoi(old);
            ++(*dims);
            old = dim_str+1;
        }
    }
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
void getVar(int ncID, const char *name, int ndims, const int dimids[], int *idp, nc_type xtype)
{
    nc_redef(ncID);
    int res = nc_def_var(ncID, name, xtype, ndims, dimids, idp);

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
        printf("Netcdf Error code: %d\n", res);
        ERROR("Could not get variable");
    }
}

void getDimName(char * dest, int i, int dims)
{
    if (dims <= 3) {
        char std_dims [3][2] = {"z","y","x"};
        int j = 3 - dims;  // for 2D for example i=0 needs to map to y and not to z!
        strcpy(dest, std_dims[j+i]);
    }
    else
    {
        sprintf(dest, "x%d", i);
    }
}


/**
 * Opens/ creates file if not already there
 */
int CreateFile(const char* file_name, size_t dims, size_t n[], int ID[], int *ptime_id)
{
    int ncID = 0;

    if (access(file_name, F_OK) == -1)
    {
        // TODO parallel file opening if sequential is too slow?
        // TODO: race condition? who opens it first?
        //#ifdef BUILD_WITH_MPI
        //int status = nc_create_par(file_name, NC_NETCDF4 | NC_MPIIO, comm_data->comm, MPI_INFO_NULL, &ncID);
        //#else
        //int status = nc_create(file_name, NC_NETCDF4, &ncID);
        int status = nc_create(file_name, 0, &ncID);
        //#endif
        if (status != NC_NOERR)
        {
            printf("Error! nc error code: %d\c", status);
            printf("Error creating file!");
            ERROR("Could not create file!");
        }
    }
    else
    {
        // open it
        D("file exists already. Opening it!");
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
    //getDim(ncID, "x", nX, pxID);
    //getDim(ncID, "y", nY, pyID);
    //getDim(ncID, "z", nZ, pzID);
    //getDim(ncID, "time", NC_UNLIMITED, ptime_id);

    int i;
    for (i = 0; i < dims; ++i)
    {
        char dim_name[256];
        getDimName(dim_name, i, dims);
        getDim(ncID, dim_name, n[i], &(ID[i]));
    }

    getDim(ncID, "time", NC_UNLIMITED, ptime_id);

    return ncID;
}

void write_netcdf(const char   *statistics_name,
        const char   *variable_name,
        const int     t,
        const size_t  vec_size,
        const void * vec[],
        const nc_type xtype)
{
    // TODO: keep files open over all timesteps!
#define MAX_DIMS 10

    // nice value when dimensions turn out to be 0.
    size_t n[MAX_DIMS];
    n[0] = vec_size;
    size_t dims;
    getDimsFromEnv(&dims, n);
    // sometimes we get only 2D slizes... handle them nicely!
    printf("\nshape: %d %d %d == %d\n", n[0], n[1], n[2], vec_size);
    if (vec_size == n[1] * n[2])
    {
        printf("2D");
        dims = 2;
        n[0] = n[1];
        n[1] = n[2];
    }
    else
    {
        int prod = 1;
        int i;
        for (i = 0; i < dims; ++i)
        {
            prod *= n[i];
        }
        if (prod != vec_size)
        {
            printf("1D");
            n[0] = vec_size;
            dims=1;
        }
    }

            // just write vector

    //          nz ny nx
    //size_t n[3] = {10, 5, 4};
    //int n[dims] = {11, 6, 1}
    //size_t dims = 3;

    // iterators:
    long int i, j;

    // the following variables are currently just used by rank 0

    int current_file_id;
    int ID[dims];
    int time_id;

    int variable_var_id;
    int time_var_id;

    int nc_status;

    char file_name[256];

    sprintf(file_name, "%s_%s.nc", variable_name, statistics_name);

    current_file_id = CreateFile(file_name, dims, n, ID, &time_id);
    // add variable Time and variable "variable":
    int variable_dims[dims+1];  //={ time_id, zID, yID, xID };
    variable_dims[0] = time_id;
    memcpy(&variable_dims[1], ID, sizeof(size_t) * dims);

    getVar(current_file_id, variable_name, dims+1, variable_dims, &variable_var_id, xtype);

    getVar(current_file_id, "time", 1, &time_id, &time_var_id, NC_DOUBLE);

    // set time only once per file!
    //nc_var_par_access(current_file_id, time_var_id, NC_COLLECTIVE);
    // TODO: check if already something in time variable!
    //size_t len_t;
    //nc_inq_dimlen(current_file_id, time_var_id, &len_t);
    // required for compatibility with netcdf3 :
    nc_enddef(current_file_id);

    size_t start[dims+1];  //= { t, 0, 0, 0 };
    size_t count[dims+1];  //= { 1, nZ, nY, nX};
    start[0] = t;
    count[0] = 1;  // writing one value
    double double_time = (double) t;
    nc_status = nc_put_vara_double(current_file_id, time_var_id, start, count, &double_time);


    // refresh all the values of time step t:
    //                  t  z  y  x
    start[0] = t;
    for (i = 1; i < dims+1; ++i)
    {
        start[i] = 0;
    }
    count[0] = 1;
    memcpy(&count[1], n, sizeof(size_t) * dims);
    printf("%d,%d,%d,%d", count[0], count[1], count[2], count[3]);
    // now do a write to the cdf! (! all the preparations up there are necessary!)
    nc_status = nc_put_vara_double(current_file_id, variable_var_id, start, count, vec);
    //printf("put_state: %d\n", nc_status);
    D("putting doubles");
    nc_close(current_file_id);
    D("wrote %s\n", file_name);
}

void write_output_d_netcdf(const char   *file_name,
        const char   *field,
        const char   *statisics_name,
        const int     t,
        const size_t  vec_size,
        const double  vec[])
{
    write_netcdf(statisics_name, field, t, vec_size, vec, NC_DOUBLE);
}

void write_output_i_netcdf(const char   *file_name,
        const char   *field,
        const char   *statisics_name,
        const int     t,
        const size_t  vec_size,
        const int     vec[])
{
    write_netcdf(statisics_name, field, t, vec_size, vec, NC_INT);
}

