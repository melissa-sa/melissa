/**
 *
 * @file get_options.c
 * @brief Parse commande line to get stats options.
 * @author Terraz Théophile
 * @date 2016-03-03
 *
 * @defgroup get_options Get options from command line
 *
 **/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stats.h"

static inline void stats_usage ()
{
    fprintf(stderr,
            " Usage:\n"
            " -p <int[]> : array of sizes of parameter sets of the study (mandatory)\n"
            "              example: -p 2:5:4\n"
            "              study with 3 variables parameters:\n"
            "               - the first can take two values,\n"
            "               - the second can take five values,\n"
            "               - the third can take four values.\n"
            " -t <int>   : number of time steps of the study\n"
            " -o <op>    : operations to be performed, separated with semicolons\n"
            "              possibles values :\n"
            "              mean\n"
            "              variance\n"
            "              min\n"
            "              max\n"
            "              threshold_exceedance\n"
            "              sobol_indices\n"
            "              (default: mean:variance)\n"
            " -e <double>: threshold for threshold exceedance computaion\n"
            " -s <int>   : maximum order for sobol indices\n"
            "\n"
            );
}

static inline void str_tolower (char *string)
{
    int i = 0;
    while (string[i] != '\0')
    {
        string[i] = (char)tolower(string[i]);
        i++;
    }
    return;
}

static inline void init_options (stats_options_t *options)
{
    // everything is set to 0
    options->nb_time_steps   = 0;
    options->nb_parameters   = 0;
    options->size_parameters = NULL;
    options->threshold       = 0.0;
    options->mean_op         = 0;
    options->variance_op     = 0;
    options->min_and_max_op  = 0;
    options->threshold_op    = 0;
    options->sobol_op        = 0;
    options->sobol_order     = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_options
 *
 * This function frees the memory in the options structure
 *
 *******************************************************************************
 *
 * @param[in] *options
 * pointer to the structure containing options parsed from command line
 *
 *******************************************************************************/

void free_options (stats_options_t *options)
{
    free (options->size_parameters);
}

static inline void get_size_parameters (char            *name,
                                        stats_options_t *options)
{
    char       *name_ptr;
    const char  s[2] = ":";
    char       *temp_char;
    int         i;

    if (name == NULL || name[0] == '-' || name[0] == ':')
    {
        stats_usage ();
        exit (1);
    }

    options->nb_parameters = 1;

    name_ptr = name;
    for (i=0; i<strlen(name)-1; i++, name_ptr++)
    {
        if (*name_ptr == ':')
        {
            options->nb_parameters += 1;
        }
    }

    options->size_parameters = malloc (options->nb_parameters * sizeof(int));

    /* get the first token */
    temp_char = strtok (name, s);
    i = 0;

    /* walk through other tokens */
    while( temp_char != NULL )
    {
       options->size_parameters[i] = atoi (temp_char);
       i += 1;

       temp_char = strtok (NULL, s);
    }
}

static inline void get_operations (char            *name,
                                   stats_options_t *options)
{
    const char  s[2] = ":";
    char       *temp_char;
    int         i;

    if (name == NULL || name[0] == '-' || name[0] == ':')
    {
        stats_usage ();
        exit (1);
    }

    options->mean_op         = 0;
    options->variance_op     = 0;
    options->min_and_max_op  = 0;
    options->threshold_op    = 0;
    options->sobol_op        = 0;

    /* get the first token */
    temp_char = strtok (name, s);
    i = 0;

    /* walk through other tokens */
    while( temp_char != NULL )
    {
        str_tolower (temp_char);

        if (0 == strcmp(temp_char, "mean"))
        {
            options->mean_op = 1;
        }
        else if (0 == strcmp(temp_char, "variance"))
        {
            options->variance_op = 1;
        }
        else if (0 == strcmp(temp_char, "min") || 0 == strcmp(temp_char, "max"))
        {
            options->min_and_max_op = 1;
        }
        else if (0 == strcmp(temp_char, "threshold") || 0 == strcmp(temp_char, "threshold_exceedance"))
        {
            options->threshold_op = 1;
        }
        else if (0 == strcmp(temp_char, "sobol") || 0 == strcmp(temp_char, "sobol_indices"))
        {
            options->sobol_op = 1;
        }
        else
        {
            stats_usage ();
            exit (1);
        }

       temp_char = strtok (NULL, s);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup get_options
 *
 * This function displays the global parameters on stdout
 *
 *******************************************************************************
 *
 * @param[in] *options
 * pointer to the structure containing the options parsed from command line
 *
 *******************************************************************************/

void print_options (stats_options_t *options)
{
    int i;
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "nb_time_step = %d\n", options->nb_time_steps);
    fprintf(stdout, "nb_parameters = %d\n", options->nb_parameters);
    for (i=0; i<options->nb_parameters; i++)
    {
        fprintf(stdout, "size_parameters[%d] = %d\n", i, options->size_parameters[i]);
    }
    fprintf(stdout, "operations:\n");
    if (options->mean_op != 0)
        fprintf(stdout, "    mean\n");
    if (options->variance_op != 0)
        fprintf(stdout, "    variance\n");
    if (options->min_and_max_op != 0)
    {
        fprintf(stdout, "    min\n");
        fprintf(stdout, "    max\n");
    }
    if (options->threshold_op != 0)
        fprintf(stdout, "    threshold exceedance, with threshold = %g\n", options->threshold);
    if (options->sobol_op != 0)
        fprintf(stdout, "    sobol indices, max order: %d\n", options->sobol_order);

}

/**
 *******************************************************************************
 *
 * @ingroup get_options
 *
 * This function parses command line options and fill the parameter structure
 *
 *******************************************************************************
 *
 * @param[in] argc
 * argc
 *
 * @param[in] **argv
 * argv
 *
 * @param[out] *options
 * pointer to the structure containing options parameters
 *
 *******************************************************************************/

void stats_get_options (int argc, char  **argv,
                        stats_options_t  *options)
{
    int opt;

    if (argc == 1) {
        stats_usage ();
        exit (0);
    }

    init_options (options);

    do
    {
        opt = getopt (argc, argv, "p:t:o:e:s:h:n");

        switch (opt) {
        case 'p':
            get_size_parameters (optarg, options);
            break;
        case 't':
            options->nb_time_steps = atoi (optarg);
            break;
        case 'o':
            get_operations (optarg, options);
            break;
        case 'e':
            options->threshold = atof (optarg);
            break;
        case 's':
            options->sobol_order = atoi (optarg);
            break;
        case 'h':
            stats_usage ();
            exit (0);
        case '?':
            stats_usage ();
            exit (1);
        default:
            break;
        }

    } while (opt != -1);

    return;
}
