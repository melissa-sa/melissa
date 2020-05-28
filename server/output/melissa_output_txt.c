#include "melissa_output.h"

void write_output_d(const char   *file_name,
                        const char   *statistics_name,
                        const char   *field,
                        const int     t,
                        const size_t  vec_size,
                        const double  vec[])
{
    int   i;
    FILE* f;

    f = fopen(file_name, "w");
    for (i=0; i<vec_size; i++)
    {
        fprintf (f, "%.*e\n", 12, vec[i]);
    }
    fclose(f);
}

void write_output_i(const char   *file_name,
                        const char   *statistics_name,
                        const char   *field,
                        const int     t,
                        const size_t  vec_size,
                        const int     vec[])
{
    int   i;
    FILE* f;

    f = fopen(file_name, "w");
    for (i=0; i<vec_size; i++)
    {
        fprintf (f, "%d\n", vec[i]);
    }
    fclose(f);
}
