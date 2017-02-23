/**
 *
 * @file melissa_options.c
 * @brief Parse commande line to get stats options.
 * @author Terraz Théophile
 * @date 2016-03-03
 *
 * @defgroup melissa_options Get options from command line
 *
 **/

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "melissa_options.h"
#include "melissa_utils.h"

static inline void stats_usage ()
{
    fprintf(stderr,
            " Usage:\n"
            " -p <int>       : number of parameters for the parametric study\n"
            "                : number of simulation\n"
            " -s <int>       : number of simulation groups for Sobol indices\n"
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
            " -e <double>    : threshold value for threshold exceedance computaion\n"
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

static inline void init_options (melissa_options_t *options)
{
    // everything is set to 0
    options->nb_time_steps   = 0;
    options->nb_parameters   = 0;
    options->nb_groups       = 0;
    options->nb_simu         = 0;
    options->threshold       = 0.0;
    options->mean_op         = 0;
    options->variance_op     = 0;
    options->min_and_max_op  = 0;
    options->threshold_op    = 0;
    options->sobol_op        = 0;
    options->sobol_order     = 0;
    options->restart         = 0;
}

static inline void get_operations (char              *name,
                                   melissa_options_t *options)
{
    const char  s[2] = ":";
    char       *temp_char;

    if (name == NULL || name[0] == '-' || name[0] == ':')
    {
        stats_usage ();
        exit (1);
    }

    /* gjuste to be sure */
    options->mean_op         = 0;
    options->variance_op     = 0;
    options->min_and_max_op  = 0;
    options->threshold_op    = 0;
    options->sobol_op        = 0;

    /* get the first token */
    temp_char = strtok (name, s);

    /* walk through other tokens */
    while( temp_char != NULL )
    {
        str_tolower (temp_char);

        if (0 == strcmp(temp_char, "mean"))
        {
            options->mean_op = 1;
        }
        else if (0 == strcmp(temp_char, "variance") || 0 == strcmp(temp_char, "var"))
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
 * @ingroup melissa_options
 *
 * This function displays the global parameters on stdout
 *
 *******************************************************************************
 *
 * @param[in] *options
 * pointer to the structure containing the options parsed from command line
 *
 *******************************************************************************/

void melissa_print_options (melissa_options_t *options)
{
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "nb_time_step = %d\n", options->nb_time_steps);
    fprintf(stdout, "nb_parameters = %d\n", options->nb_parameters);
    if (options->sobol_op != 0)
        fprintf(stdout, "nb_groups = %d\n", options->nb_groups);
    fprintf(stdout, "nb_simu = %d\n", options->nb_simu);
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
    if (options->restart != 0)
        fprintf(stdout, "using options.save restart file\n");
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_options
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

void melissa_get_options (int argc, char    **argv,
                          melissa_options_t  *options)
{
    int opt;

    if (argc == 1) {
        stats_usage ();
        exit (0);
    }

    init_options (options);

    do
    {
        opt = getopt (argc, argv, "p:t:o:e:s:g:h:r");

        switch (opt) {
        case 'r':
            if (0 == melissa_read_options (options))
            {
                options->restart = 1;
                melissa_check_options (options);
                return;
            }
            fprintf (stdout, "WARNING: can not read option.save file\n");
            options->restart = 0;
            break;
        case 'p':
            options->nb_parameters = atoi (optarg);
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
            options->nb_simu = atoi (optarg);
            break;
        case 'g':
            options->nb_groups = atoi (optarg);
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

    melissa_check_options (options);

    return;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_options
 *
 * This function validates the option structure
 *
 *******************************************************************************
 *
 * @param[out] *options
 * pointer to the structure containing options parameters
 *
 *******************************************************************************/

void melissa_check_options (melissa_options_t  *options)
{
    // check consistency
    if (options->mean_op == 0 &&
        options->variance_op == 0 &&
        options->min_and_max_op == 0 &&
        options->threshold_op == 0 &&
        options->sobol_op == 0)
    {
        // default values
        fprintf (stdout, "WARNING: no operation given, set to mean and variance\n");
        options->mean_op = 1;
        options->variance_op = 1;
    }

    if (options->sobol_op != 0)
    {
        if (options->nb_groups < 1)
        {
            fprintf (stderr, "ERROR: number of simulation groups must be > 0 for Sobol indices computation\n");
            stats_usage ();
            exit (1);
        }
        if (options->nb_parameters < 2)
        {
            fprintf (stderr, "ERROR: study must have at least 2 variable parameter\n");
            stats_usage ();
            exit (1);
        }
        if (options->nb_simu != options->nb_groups * (options->nb_parameters + 2))
        {
            fprintf (stderr, "WARNING:  wrong number of simulations, set to %d\n", options->nb_groups * (options->nb_parameters + 2));
            options->nb_simu = options->nb_groups * (options->nb_parameters + 2);
        }
    }
    else
    {
        if (options->nb_groups == 0)
        {
            options->nb_groups = options->nb_simu;
        }
        else if (options->nb_simu == 0)
        {
            options->nb_simu = options->nb_groups;
        }
        if (options->nb_simu < 1 && options->nb_groups < 1)
        {
            fprintf (stderr, "ERROR: wrong number of simulations\n");
            stats_usage ();
            exit (1);
        }
        if (options->nb_parameters < 1)
        {
            fprintf (stderr, "ERROR: study must have at least 1 variable parameter\n");
            stats_usage ();
            exit (1);
        }
        if (options->nb_time_steps < 1)
        {
            fprintf (stderr, "ERROR: study must have at least 1 time step\n");
            stats_usage ();
            exit (1);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_options
 *
 * This function writes the option structure on disc
 *
 *******************************************************************************
 *
 * @param[in] *options
 * pointer to the structure containing global options
 *
 *******************************************************************************/

void melissa_write_options (melissa_options_t *options)
{
    FILE* f;

    f = fopen("options.save", "wb+");

    fwrite(options, sizeof(*options), 1, f);

    fclose(f);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_options
 *
 * This function reads a saved option structure on disc
 *
 *******************************************************************************
 *
 * @param[in,out] *options
 * pointer to the structure containing global options
 *
 *******************************************************************************/

int melissa_read_options (melissa_options_t *options)
{
    FILE* f = NULL;
    int ret = 1;

    f = fopen("options.save", "rb");

    if (f != NULL)
    {
        if (1 == fread(options, sizeof(*options), 1, f));
        {
            ret = 0;
        }
    }

    fclose(f);
    return ret;
}
