#include "melissa_output.h"

void write_output_d(const char   *file_name,
                            const char   *field,
                            const int     t,
                            const size_t  vect_size,
                            const double  vec[])
{
    int     i;
    FILE*   f;
    double  time_value;
    char    c_buffer[81], c_buffer2[81];
    float  *s_buffer;
    int32_t     n = 1;

    time_value = 0.0012 * t;
    s_buffer = melissa_malloc (vect_size * sizeof(float));

    for (i=0; i<vect_size; i++)
    {
        s_buffer[i] = (float)vec[i];
    }

    f = fopen(file_name, "w");
    sprintf(c_buffer, "%s (time values: %d, %g)", file_name, (int)t, time_value);
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    sprintf(c_buffer, "part");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (&n, sizeof(int32_t), 1, f);
    sprintf(c_buffer, "hexa8");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (s_buffer, sizeof(float), vect_size, f);

    fclose(f);
    melissa_free (s_buffer);
}


void write_output_i(const char   *file_name,
                            const char   *field,
                            const int     t,
                            const size_t  vect_size,
                            const int     vec[])
{
    int     i;
    FILE*   f;
    double  time_value;
    char    c_buffer[81], c_buffer2[81];
    float  *s_buffer;
    int32_t     n = 1;

    time_value = 0.0012 * t;
    s_buffer = melissa_malloc (vect_size * sizeof(float));

    for (i=0; i<vect_size; i++)
    {
        s_buffer[i] = (float)vec[i];
    }

    f = fopen(file_name, "w");
    sprintf(c_buffer, "%s (time values: %d, %g)", file_name, (int)t, time_value);
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    sprintf(c_buffer, "part");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (&n, sizeof(int32_t), 1, f);
    sprintf(c_buffer, "hexa8");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (s_buffer, sizeof(float), vect_size, f);

    fclose(f);
    melissa_free (s_buffer);
}
