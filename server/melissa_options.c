/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "melissa_options.h"
#include "melissa_data.h"
#include "melissa_utils.h"

static inline void stats_usage ()
{
    fprintf(stderr,
            " Usage:\n"
            " -p <int>       : number of parameters for the parametric study\n"
            " -s <int>       : study initial sampling size\n"
            " -t <int>       : number of time steps of the study\n"
            " -o <char*>     : operations separated by semicolons\n"
            "                  possibles values :\n"
            "                  mean\n"
            "                  variance\n"
            "                  skewness\n"
            "                  kurtosis\n"
            "                  min\n"
            "                  max\n"
            "                  threshold_exceedance\n"
            "                  quantile\n"
            "                  sobol_indices\n"
            "                  (default: mean:variance)\n"
            " -e <double>    : threshold value for threshold exceedance computaion\n"
            " -q <char*>     : quantile values separated by semicolons\n"
            " -n <char*>     : Melissa Launcher node name (default: localhost)\n"
            " -l             : Learning mode\n"
            " -r <char*>     : Melissa restart files directory\n"
            " -c <double>    : Server checkpoints intervals (seconds, default: 300)\n"
            " -v             : Verbosity level\n"
            " -h             : Print this message\n"
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
    options->sampling_size   = 0;
    options->nb_simu         = 0;
    options->nb_fields       = 0;
    options->nb_thresholds   = 0;
    options->mean_op         = 0;
    options->variance_op     = 0;
    options->skewness_op     = 0;
    options->kurtosis_op     = 0;
    options->min_and_max_op  = 0;
    options->threshold_op    = 0;
    options->quantile_op     = 0;
    options->nb_quantiles    = 0;
    options->sobol_op        = 0;
    options->sobol_order     = 0;
    options->learning        = 0;
    options->restart         = 0;
    options->verbose_lvl     = MELISSA_INFO;
    options->check_interval  = 300.0;
    options->timeout_simu    = 300.0;
    options->txt_pull_port   = 5556;
    options->txt_push_port   = 5555;
    options->txt_req_port    = 5554;
    options->data_port       = 2004;
    sprintf (options->restart_dir, ".");
    sprintf (options->launcher_name, "localhost");
}

static inline void get_nb_fields (char               *name,
                                  melissa_options_t  *options)
{
    int i, len;

    if (name == NULL || strncmp(&name[0],"-",1) == 0 || strncmp(&name[0],":",1) == 0)
    {
        stats_usage ();
        exit (1);
    }

    i=0;
    len = strlen(name);
    options->nb_fields = 1;
    for (i = 0; i < len; i++)
    {
        if (strncmp(&name[i],":",1) == 0)
        {
            options->nb_fields += 1;
        }
    }
}

static inline void get_nb_thresholds (char               *name,
                                      melissa_options_t  *options)
{
    int i, len;

    if (name == NULL || strncmp(&name[0],"-",1) == 0 || strncmp(&name[0],":",1) == 0)
    {
        stats_usage ();
        exit (1);
    }

    i=0;
    len = strlen(name);
    options->nb_thresholds = 1;
    for (i = 0; i < len; i++)
    {
        if (strncmp(&name[i],":",1) == 0)
        {
            options->nb_thresholds += 1;
        }
    }
    options->threshold = melissa_calloc (options->nb_thresholds, sizeof(double));
}

static inline void get_nb_quantiles (char               *name,
                                     melissa_options_t  *options)
{
    int i, len;

    if (name == NULL || strncmp(&name[0],"-",1) == 0 || strncmp(&name[0],":",1) == 0)
    {
        stats_usage ();
        exit (1);
    }

    i=0;
    len = strlen(name);
    options->nb_quantiles = 1;
    for (i = 0; i < len; i++)
    {
        if (strncmp(&name[i],":",1) == 0)
        {
            options->nb_quantiles += 1;
        }
    }
    options->quantile_order = melissa_calloc (options->nb_quantiles, sizeof(double));
}

static inline void get_thresholds (char              *name,
                                   melissa_options_t *options)
{
    const char  s[2] = ":";
    char       *temp_char;
    int         i=0;

    if (name == NULL || strcmp(&name[0],"-") == 0 || strcmp(&name[0],":") == 0)
    {
        stats_usage ();
        exit (1);
    }

    /* get the first token */
    temp_char = strtok (name, s);

    /* walk through other tokens */
    while( temp_char != NULL )
    {
        options->threshold[i] = atof(temp_char);
        i++;
        temp_char = strtok (NULL, s);
    }
}

static inline void get_quantile_orders (char              *name,
                                        melissa_options_t *options)
{
    const char  s[2] = ":";
    char       *temp_char;
    int         i=0;

    if (name == NULL || strcmp(&name[0],"-") == 0 || strcmp(&name[0],":") == 0)
    {
        stats_usage ();
        exit (1);
    }

    /* get the first token */
    temp_char = strtok (name, s);

    /* walk through other tokens */
    while( temp_char != NULL )
    {
        options->quantile_order[i] = atof(temp_char);
        i++;
        temp_char = strtok (NULL, s);
    }
}

static inline void get_operations (char              *name,
                                   melissa_options_t *options)
{
    const char  s[2] = ":";
    char       *temp_char;

    if (name == NULL || strcmp(&name[0],"-") == 0 || strcmp(&name[0],":") == 0)
    {
        stats_usage ();
        exit (1);
    }

    /* just to be sure */
    options->mean_op         = 0;
    options->variance_op     = 0;
    options->skewness_op     = 0;
    options->kurtosis_op     = 0;
    options->min_and_max_op  = 0;
    options->threshold_op    = 0;
    options->quantile_op     = 0;
    options->sobol_op        = 0;
    /* get the first token */
    temp_char = strtok (name, s);

    /* walk through other tokens */
    while( temp_char != NULL )
    {
        str_tolower (temp_char);

        if (0 == strcmp(temp_char, "mean") || 0 == strcmp(temp_char, "means"))
        {
            options->mean_op = 1;
        }
        else if (0 == strcmp(temp_char, "variance") || 0 == strcmp(temp_char, "variances")
                 || 0 == strcmp(temp_char, "var"))
        {
            options->variance_op = 1;
        }
        else if (0 == strcmp(temp_char, "skewness"))
        {
            options->skewness_op = 1;
        }
        else if (0 == strcmp(temp_char, "kurtosis"))
        {
            options->kurtosis_op = 1;
        }
        else if (0 == strcmp(temp_char, "min") || 0 == strcmp(temp_char, "max")
                 || 0 == strcmp(temp_char, "minimum") || 0 == strcmp(temp_char, "maximum"))
        {
            options->min_and_max_op = 1;
        }
        else if (0 == strcmp(temp_char, "threshold") || 0 == strcmp(temp_char, "threshold_exceedance")
                 || 0 == strcmp(temp_char, "thresholds") || 0 == strcmp(temp_char, "threshold_exceedances"))
        {
            options->threshold_op = 1;
        }
        else if (0 == strcmp(temp_char, "quantile") || 0 == strcmp(temp_char, "quantiles"))
        {
            options->quantile_op = 1;
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
    melissa_print(VERBOSE_INFO, "Options:\n");
    melissa_print(VERBOSE_INFO, "nb_time_steps = %d\n", options->nb_time_steps);
    melissa_print(VERBOSE_INFO, "nb_parameters = %d\n", options->nb_parameters);
    melissa_print(VERBOSE_INFO, "sampling_size = %d\n", options->sampling_size);
    melissa_print(VERBOSE_INFO, "nb_simu = %d\n", options->nb_simu);
    melissa_print(VERBOSE_INFO, "nb_fields = %d\n", options->nb_fields);
    melissa_print(VERBOSE_INFO, "operations:\n");
    if (options->mean_op != 0)
        melissa_print(VERBOSE_INFO, "    mean\n");
    if (options->variance_op != 0)
        melissa_print(VERBOSE_INFO, "    variance\n");
    if (options->skewness_op != 0)
        melissa_print(VERBOSE_INFO, "    skewness\n");
    if (options->kurtosis_op != 0)
        melissa_print(VERBOSE_INFO, "    kurtosis\n");
    if (options->min_and_max_op != 0)
    {
        melissa_print(VERBOSE_INFO, "    min\n");
        melissa_print(VERBOSE_INFO, "    max\n");
    }
    if (options->threshold_op != 0)
        melissa_print(VERBOSE_INFO, "    threshold exceedance (%d values)\n", options->nb_thresholds);
    if (options->quantile_op != 0)
        melissa_print(VERBOSE_INFO, "    quantiles (%d values)\n", options->nb_quantiles);
    if (options->sobol_op != 0)
        melissa_print(VERBOSE_INFO, "    sobol indices\n");
    if (options->learning != 0)
        melissa_print(VERBOSE_INFO, "    learning\n");
//    if (options->restart != 0)
//        fprintf(stdout, "using options.save restart file\n");
    melissa_print(VERBOSE_DEBUG, "Melissa launcher node name: %s\n", options->launcher_name);
    melissa_print(VERBOSE_INFO, "Checkpoint every %g seconds\n", options->check_interval);
    melissa_print(VERBOSE_DEBUG, "Wait time for simulation message before timeout: %d seconds\n", options->timeout_simu);
    melissa_print(VERBOSE_INFO, "Melissa verbosity: %d\n", options->verbose_lvl);
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

void melissa_get_options (int                 argc,
                          char              **argv,
                          melissa_options_t  *options)
{
    int opt;

    if (argc == 1) {
        fprintf (stderr, "Error: missing options\n");
        stats_usage ();
        exit (0);
    }

    init_options (options);

    struct option longopts[] = {{ "checkintervals", required_argument, NULL, 'c' },
                                { "treshold",       required_argument, NULL, 'e' },
                                { "tresholds",      required_argument, NULL, 'e' },
                                { "fieldnames",     required_argument, NULL, 'f' },
                                { "help",           no_argument,       NULL, 'h' },
                                { "learning",       required_argument, NULL, 'l' },
                                { "more",           required_argument, NULL, 'm' },
                                { "launchername",   required_argument, NULL, 'n' },
                                { "operations",     required_argument, NULL, 'o' },
                                { "parameters",     required_argument, NULL, 'p' },
                                { "quantile",       required_argument, NULL, 'q' },
                                { "quantiles",      required_argument, NULL, 'q' },
                                { "restart",        required_argument, NULL, 'r' },
                                { "samplingsize",   required_argument, NULL, 's' },
                                { "timesteps",      required_argument, NULL, 't' },
                                { "verbosity",      required_argument, NULL, 'v' },
                                { "verbose",        required_argument, NULL, 'v' },
                                { "timeout",        required_argument, NULL, 'w' },
                                { "txt_push_port",  required_argument, NULL, 1000 },
                                { "txt_pull_port",  required_argument, NULL, 1001 },
                                { "data_port",      required_argument, NULL, 1002 },
                                { "req_port",       required_argument, NULL, 1003 },
                                { "horovod",        required_argument, NULL, 1004 },
                                { NULL,             0,                 NULL,  0  }};

    do
    {
        opt = getopt_long (argc, argv, "c:e:f:hl:m:n:o:p:q:r:s:t:v:w:", longopts, NULL);

        switch (opt) {
        case 'r':
            sprintf (options->restart_dir, "%s", optarg);
            if (strlen(options->restart_dir) < 1)
            {
                sprintf (options->restart_dir, ".");
            }
            options->restart = 1;
            break;
        case 'm':
            printf ("restart_dir : %s", optarg);
            sprintf (options->restart_dir, "%s", optarg);
            if (strlen(options->restart_dir) < 1)
            {
                sprintf (options->restart_dir, ".");
            }
            options->restart = 2;
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
            get_nb_thresholds (optarg, options);
            get_thresholds (optarg, options);
            break;
        case 'q':
            get_nb_quantiles (optarg, options);
            get_quantile_orders (optarg, options);
            break;
        case 's':
            options->sampling_size = atoi (optarg);
            break;
        case 'l':
            if (options->learning < 1)
            {
                options->learning = 1;
            }
            sprintf (options->nn_path, "%s", optarg);
            break;
        case 'n':
            sprintf (options->launcher_name, "%s", optarg);
            break;
        case 'f':
            get_nb_fields (optarg, options);
            break;
        case 'c':
            options->check_interval = atof (optarg);
            break;
        case 'v':
            options->verbose_lvl = atoi (optarg);
            break;
        case 'w':
            options->timeout_simu = atof (optarg);
            break;
        case 1000:
            options->txt_push_port = atoi (optarg);
            break;
        case 1001:
            options->txt_pull_port = atoi (optarg);
            break;
        case 1002:
            options->data_port = atoi (optarg);
            break;
        case 1003:
            options->txt_req_port = atoi (optarg);
            break;
        case 1004:
            options->learning = 2;
            break;
        case 'h':
            stats_usage ();
            exit (0);
        case 0:
            break;
        case '?':
            fprintf (stderr, "Error: unknown option\n");
            stats_usage ();
            exit (1);
        default:
            break;
        }

    } while (opt != -1);

    init_verbose_lvl (options->verbose_lvl);
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
        options->kurtosis_op == 0 &&
        options->skewness_op == 0 &&
        options->min_and_max_op == 0 &&
        options->threshold_op == 0 &&
        options->quantile_op == 0 &&
        options->sobol_op == 0 &&
        options->learning == 0)
    {
        // default values
        fprintf (stderr, "WARNING: no operation given, set to mean and variance\n");
        options->mean_op = 1;
        options->variance_op = 1;
    }

    if (options->threshold_op != 0 && options->nb_thresholds < 1)
    {
        fprintf (stderr, "ERROR: you must provide at least 1 threshold value\n");
        stats_usage ();
        exit (1);
    }

    if (options->quantile_op != 0 && options->nb_quantiles < 1)
    {
        fprintf (stderr, "ERROR: you must provide at least 1 quantile value\n");
        stats_usage ();
        exit (1);
    }

    if (options->sampling_size < 2)
    {
        fprintf (stderr, "ERROR: study sampling size must be greater than 1\n");
        stats_usage ();
        exit (1);
    }

    if (options->sobol_op != 0)
    {
        if (options->nb_parameters < 2)
        {
            fprintf (stderr, "ERROR: simulations must have at least 2 variable parameter\n");
            stats_usage ();
            exit (1);
        }
        options->nb_simu = options->sampling_size * (options->nb_parameters + 2);
    }
    else
    {

        options->nb_simu = options->sampling_size;
        if (options->sampling_size < 2)
        {
            fprintf (stderr, "ERROR: sampling size must be > 1\n");
            stats_usage ();
            exit (1);
        }
        if (options->nb_parameters < 1)
        {
            fprintf (stderr, "ERROR: simulations must have at least 1 variable parameter\n");
            stats_usage ();
            exit (1);
        }
    }

    if (options->nb_time_steps < 1)
    {
        fprintf (stderr, "ERROR: simulations must have at least 1 time step\n");
        stats_usage ();
        exit (1);
    }

    if (options->launcher_name == NULL)
    {
        fprintf (stderr, "Warning: Melissa Launcher node name set to \"localhost\"\n");
        sprintf (options->launcher_name, "localhost");
    }

    if (strlen(options->restart_dir) < 1)
    {
        fprintf (stderr, "options->restart_dir= %s changing to .\n", options->restart_dir);
        sprintf (options->restart_dir, ".");
    }

    if (options->check_interval < 5.0)
    {
        fprintf (stderr, "checkpoint interval too small, changing to 5.0\n");
        options->check_interval = 5.0;
    }

    if (options->timeout_simu < 5.0)
    {
        fprintf (stderr, "time before simulation timeout too small, changing to 5.0\n");
        options->timeout_simu = 5.0;
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

    fwrite(options, sizeof(melissa_options_t), 1, f);
    if (options->threshold_op)
    {
        fwrite(options->threshold, sizeof(double), options->nb_thresholds, f);
    }
    if (options->quantile_op)
    {
        fwrite(options->quantile_order, sizeof(double), options->nb_quantiles, f);
    }

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
    int ret;
    char file_name[256];

    sprintf (file_name, "%s/options.save", options->restart_dir);
    f = fopen(file_name, "rb");

    if (f != NULL)
    {
        fread(options, sizeof(melissa_options_t), 1, f);
        if (options->threshold_op)
        {
            options->threshold = melissa_calloc (options->nb_thresholds, sizeof(double));
            fread(options->threshold, sizeof(double), options->nb_thresholds, f);
        }
        if (options->quantile_op)
        {
            options->quantile_order = melissa_calloc (options->nb_quantiles, sizeof(double));
            fread(options->quantile_order, sizeof(double), options->nb_quantiles, f);
        }
    }
    else
    {
        ret = -1;
    }

    fclose(f);
    return ret;
}
