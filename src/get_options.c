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
            " -p <int>:<int> : number of parameters for the parametric study\n"
            "                  and number of simulation, or simulation groups for Sobol indices\n"
            " -t <int>       : number of time steps of the study\n"
            " -o <op>        : operations separated by semicolons\n"
            "                  possibles values :\n"
            "                  mean\n"
            "                  variance\n"
            "                  min\n"
            "                  max\n"
            "                  threshold_exceedance\n"
            "                  sobol_indices\n"
            "                  (default: mean:variance)\n"
            " -e <double>    : threshold for threshold exceedance computaion\n"
//            " -s <int>     : maximum order for sobol indices\n"
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
    options->nb_groups       = 0;
    options->threshold       = 0.0;
    options->mean_op         = 0;
    options->variance_op     = 0;
    options->min_and_max_op  = 0;
    options->threshold_op    = 0;
    options->sobol_op        = 0;
    options->sobol_order     = 0;
}

static inline void get_parameters (char            *name,
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

    /* get the first token */
    temp_char = strtok (name, s);
    options->nb_parameters = atoi (temp_char);
    temp_char = strtok (NULL, s);
    options->nb_groups = atoi (temp_char);
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
    if (options->sobol_op != 0)
        fprintf(stdout, "nb_groups = %d\n", options->nb_groups);
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
            get_parameters (optarg, options);
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


    if (options->sobol_op != 0)
    {
        options->nb_simu = options->nb_groups * (options->nb_parameters + 2);
    }
    else
    {
        options->nb_simu = options->nb_groups;
    }


    return;
}
