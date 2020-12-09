// Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (Inria)
//               2017, EDF (https://www.edf.fr/)
//               2020, Institut National de Recherche en Informatique et en Automatique (Inria)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// 
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <melissa/server/output.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netcdf.h>

#ifndef NDEBUG

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
 * For one dimension MELISSA_NETCDF_DIMENSIONS can left unset.
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
    if (res != NC_NOERR)
    {
        printf("Netcdf Error code: %d\n", res);
        ERROR("Could not create dimension");
    }
    return res;
}

/**
 * Creates variables or only returns its id if already existent.
 */
void getVar(const int ncID, const char *name, const int ndims, const int dimids[], int *idp, const nc_type xtype)
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
        ERROR("Could not create variable");
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

#define MAX_DIMS 10
#define MAX_FILE_NAME 512
typedef struct  {
    const char file_name[MAX_FILE_NAME];
    size_t dims;
    size_t n[MAX_DIMS];
    int ncID;


    int variable_var_id;
    int time_var_id;

} CacheEntry;

// I want to write CPP!!
// Allows 4096 different output files...
#define FILE_CACHE_SIZE 4096
static CacheEntry cache[FILE_CACHE_SIZE];
static size_t last_entry = 0;

// Returns NULL if not found.
CacheEntry * findEntry(const char* file_name)
{
    int i;
    for (i = 0; i < FILE_CACHE_SIZE; ++i)
    {
        if (strcmp(cache[i].file_name, file_name) == 0)
            return &cache[i];
    }
    return NULL;
}



/**
 * Opens/ creates file if not already there
 */
CacheEntry * CreateFile(const char* file_name, const char* variable_name, size_t vec_size, nc_type xtype)
{
    CacheEntry * c = &cache[last_entry];
    last_entry++;

    // nice value when dimensions turn out to be 0.
    c->n[0] = vec_size;
    getDimsFromEnv(&c->dims, c->n);
    // sometimes we get only 2D slizes... handle them nicely!
    //printf("\nshape: %d %d %d == %d\n", c->n[0], c->n[1], c->n[2], vec_size);
    if (vec_size == c->n[1] * c->n[2])
    {
        c->dims = 2;
        c->n[0] = c->n[1];
        c->n[1] = c->n[2];
    }
    else
    {
        int prod = 1;
        int i;
        for (i = 0; i < c->dims; ++i)
        {
            prod *= c->n[i];
        }
        if (prod != vec_size)
        {
            c->n[0] = vec_size;
            c->dims=1;
        }
    }
    c->ncID = 0;

    if (access(file_name, F_OK) == -1)
    {
        // TODO parallel file opening if sequential is too slow?
        // TODO: race condition? who opens it first?
        //int status = nc_create(file_name, NC_NETCDF4, &ncID);
        int status = nc_create(file_name, 0, &c->ncID);
        //#endif
        if (status != NC_NOERR)
        {
            printf("Error! nc error code: %d\c", status);
            ERROR("Could not create file!");
        }
    }
    else
    {
        // open it
        // should never get here...
        D("file exists already. Opening it!");
        int status = nc_open(file_name, NC_WRITE, &c->ncID);
        if (status!= NC_NOERR)
            //#endif
        {
            printf("Error! nc error code: %d", status);
            ERROR("Could not open file!");
        }
    }

    // create file. if it exists already, just load it.

    // add/get Dimensions
    //getDim(ncID, "x", nX, pxID);
    //getDim(ncID, "y", nY, pyID);
    //getDim(ncID, "z", nZ, pzID);
    //getDim(ncID, "time", NC_UNLIMITED, ptime_id);

    int ID[c->dims];
    int i;
    for (i = 0; i < c->dims; ++i)
    {
        char dim_name[256];
        getDimName(dim_name, i, c->dims);
        getDim(c->ncID, dim_name, c->n[i], &ID[i]);
    }

    int time_id;

    // TODO: maybe we are more performant if we write out all the time variables at once!
    getDim(c->ncID, "time", NC_UNLIMITED, &time_id);

    int variable_dims[c->dims+1];  //={ time_id, zID, yID, xID };
    variable_dims[0] = time_id;
    memcpy(&variable_dims[1], ID, sizeof(int) * c->dims);

    getVar(c->ncID, variable_name, c->dims+1, variable_dims, &c->variable_var_id, xtype);

    getVar(c->ncID, "time", 1, &time_id, &c->time_var_id, NC_DOUBLE);

    // set time only once per file!
    //nc_var_par_access(current_file_id, time_var_id, NC_COLLECTIVE);
    // TODO: check if already something in time variable!
    //size_t len_t;
    //nc_inq_dimlen(current_file_id, time_var_id, &len_t);
    // required for compatibility with netcdf3 :
    nc_enddef(c->ncID);

    strcpy(c->file_name, file_name);

    return c;
}

void write_netcdf(const char   *statistics_name,
        const char   *variable_name,
        const int     t,
        const size_t  vec_size,
        const void * vec[],
        const nc_type xtype)
{

            // just write vector

    //          nz ny nx
    //size_t n[3] = {10, 5, 4};
    //int n[dims] = {11, 6, 1}
    //size_t dims = 3;

    // iterators:
    long int i, j;

    // the following variables are currently just used by rank 0

    int nc_status;

    char file_name[MAX_FILE_NAME];

    sprintf(file_name, "%s_%s.nc", variable_name, statistics_name);
    if (t == 0)
    {
      printf("Start writing %s\n", file_name);
    }

    CacheEntry *c;
    if (t == 0)
    {
        c = CreateFile(file_name, variable_name, vec_size, xtype);
    }
    else
    {
        c = findEntry(file_name);
    }

    // add variable Time and variable "variable":

    size_t start[c->dims+1];  //= { t, 0, 0, 0 };
    size_t count[c->dims+1];  //= { 1, nZ, nY, nX};
    start[0] = t;
    count[0] = 1;  // writing one value
    double double_time = (double) t;
    nc_status = nc_put_vara_double(c->ncID, c->time_var_id, start, count, &double_time);

    // refresh all the values of time step t:
    //                  t  z  y  x
    start[0] = t;
    for (i = 1; i < c->dims+1; ++i)
    {
        start[i] = 0;
    }
    count[0] = 1;
    memcpy(&count[1], c->n, sizeof(size_t) * c->dims);
    //printf("%d,%d,%d,%d", count[0], count[1], count[2], count[3]);
    // now do a write to the cdf! (! all the preparations up there are necessary!)
    nc_status = nc_put_vara_double(c->ncID, c->variable_var_id, start, count, vec);
    //printf("put_state: %d\n", nc_status);
    D("putting doubles");
    D("wrote %s\n", file_name);

    const char * cnb_timesteps = getenv("MELISSA_NETCDF_NB_TIME_STEPS");
    if (cnb_timesteps == NULL)
    {
        ERROR("Need to set environment variable MELISSA_NETCDF_NB_TIME_STEPS!");
    }

    int nb_timesteps = atoi(cnb_timesteps);
    if (nb_timesteps == 0)
    {
        ERROR("Need meaningful value (e.g. >0) for MELISSA_NETCDF_NB_TIME_STEPS!");
    }
    if (t == nb_timesteps-1)
    {
        nc_close(c->ncID);
        printf("Finished writing %s\n", file_name);
        //D("Closed file!");
    }
}

void write_output_d(const char   *file_name,
        const char   *field,
        const char   *statisics_name,
        const int     t,
        const size_t  vec_size,
        const double  vec[])
{
    write_netcdf(statisics_name, field, t, vec_size, vec, NC_DOUBLE);
}

void write_output_i(const char   *file_name,
        const char   *field,
        const char   *statisics_name,
        const int     t,
        const size_t  vec_size,
        const int     vec[])
{
    write_netcdf(statisics_name, field, t, vec_size, vec, NC_INT);
}

