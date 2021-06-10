/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENSE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

#include <melissa/server/data.h>
#include <melissa/server/server.h>
#include <melissa/utils.h>

#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static inline void init_options (melissa_options_t* options)
{
    memset(options, 0, sizeof(*options));

    options->verbose_lvl     = MELISSA_INFO;
    options->check_interval  = 300.0;
    options->timeout_simu    = 300.0;
    options->txt_pull_port   = 5556;
    options->txt_push_port   = 5555;
    options->txt_req_port    = 5554;
    options->data_port       = 2004;
    sprintf(options->restart_dir, ".");
    sprintf(options->launcher_name, "localhost");
}


int melissa_options_get_fields(char* optarg, melissa_server_t* server)
{
    assert(optarg);
    assert(server);
	assert(server->melissa_options.nb_fields == 0);
	assert(!server->fields);

    if(optarg[0] == '\0') {
        fprintf(stderr, "-f option passed without argument\n");
        return -1;
    }

	if(optarg[0] == ':') {
		fprintf(stderr, "first name in -f argument is empty\n");
		return -1;
	}

    size_t num_fields = 1;
	size_t arglen = strlen(optarg);

	if(optarg[arglen-1] == ':') {
		fprintf(stderr, "last name in -f argument is empty\n");
		return -1;
	}

    for(size_t i = 0; i < arglen; ++i) {
        if(optarg[i] == ':') {
            ++num_fields;

            // strtok ignores consecutive delimiters (see below)
            if(i > 0 && optarg[i-1] == ':') {
                fprintf(stderr, "found consecutive delimiters in field names\n");
                return -1;
            }
        }
		else if(!isalnum(optarg[i])) {
			fprintf(
				stderr,
				"field names must contain only alphanumeric charachters, got '%c'\n",
				optarg[i]
			);
			return -1;
		}
    }

    size_t fields_size_bytes = num_fields * sizeof(melissa_field_t);

    server->melissa_options.nb_fields = num_fields;
    server->fields = (melissa_field_t*)melissa_malloc(fields_size_bytes);
    memset(server->fields, 0, fields_size_bytes);

    const char delimiters[] = ":";
    char* strtok_state = NULL;
    size_t index = 0;

    for(const char* name = strtok_r(optarg, delimiters, &strtok_state);
        name && index < num_fields;
        name = strtok_r(NULL, delimiters, &strtok_state), ++index
    )
    {
        if(strlen(name) > MAX_FIELD_NAME_LEN) {
            fprintf(
                stderr, "field name '%s' longer than %u\n",
                name, MAX_FIELD_NAME_LEN
            );

			free(server->fields);
			server->fields = NULL;

            return -1;
        }

        strncpy(server->fields[index].name, name, MAX_FIELD_NAME_LEN);
    }

    assert(index == num_fields);

    return 0;
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
        return;
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
 * This function displays the global parameters on stdout
 *
 * @param[in] *options
 * pointer to the structure containing the options parsed from command line
 *
 */

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
 * This function parses command line options and fill the parameter structure
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
 */

void melissa_get_options(int argc, char **argv, melissa_server_t* server)
{
    assert(server);

    if(argc <= 1) {
        fprintf (stderr, "Error: missing options\n");
        stats_usage();
        exit(EXIT_FAILURE);
    }

    melissa_options_t* options = &server->melissa_options;
    init_options(options);

    struct option longopts[] = {{ "checkintervals",          required_argument, NULL, 'c' },
                                { "treshold",                required_argument, NULL, 'e' },
                                { "tresholds",               required_argument, NULL, 'e' },
                                { "fieldnames",              required_argument, NULL, 'f' },
                                { "help",                    no_argument,       NULL, 'h' },
                                { "learning",                required_argument, NULL, 'l' },
                                { "more",                    required_argument, NULL, 'm' },
                                { "launchername",            required_argument, NULL, 'n' },
                                { "operations",              required_argument, NULL, 'o' },
                                { "parameters",              required_argument, NULL, 'p' },
                                { "quantile",                required_argument, NULL, 'q' },
                                { "quantiles",               required_argument, NULL, 'q' },
                                { "restart",                 required_argument, NULL, 'r' },
                                { "samplingsize",            required_argument, NULL, 's' },
                                { "timesteps",               required_argument, NULL, 't' },
                                { "verbosity",               required_argument, NULL, 'v' },
                                { "verbose",                 required_argument, NULL, 'v' },
                                { "timeout",                 required_argument, NULL, 'w' },
                                { "txt_push_port",           required_argument, NULL, 1000 },
                                { "txt_pull_port",           required_argument, NULL, 1001 },
                                { "data_port",               required_argument, NULL, 1002 },
                                { "txt_req_port",            required_argument, NULL, 1003 },
                                { "horovod",                 no_argument,       NULL, 1004 },
                                { NULL,                      0,                 NULL,  0   }};

    int opt = -1;
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
        case 'n':
            sprintf (options->launcher_name, "%s", optarg);
            break;
        case 'f':
            if(melissa_options_get_fields(optarg, server) < 0) {
                stats_usage();
                exit(1);
            }
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
 * This function validates the option structure
 *
 * @param[out] *options
 * pointer to the structure containing options parameters
 *
 */

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
        melissa_print (VERBOSE_WARNING, "no operation given, set to mean and variance\n");
        options->mean_op = 1;
        options->variance_op = 1;
    }

    if (options->threshold_op != 0 && options->nb_thresholds < 1)
    {
        melissa_print (VERBOSE_ERROR, "you must provide at least 1 threshold value\n");
        stats_usage ();
        exit (1);
    }

    if (options->quantile_op != 0 && options->nb_quantiles < 1)
    {
        melissa_print (VERBOSE_ERROR, "you must provide at least 1 quantile value\n");
        stats_usage ();
        exit (1);
    }

    if (options->sampling_size < 2)
    {
        if (options->sampling_size < 1)
        {
            melissa_print (VERBOSE_ERROR, "study sampling size must be greater than 0\n");
            stats_usage ();
            exit (1);
        }
        else
        {
            melissa_print (VERBOSE_WARNING, "study sampling size must be greater than 1\n");
        }
    }

    if (options->sobol_op != 0)
    {
        if (options->nb_parameters < 2)
        {
            melissa_print (VERBOSE_ERROR, "simulations must have at least 2 variable parameter\n");
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
            melissa_print (VERBOSE_ERROR, "sampling size must be > 1\n");
            stats_usage ();
            exit (1);
        }
        if (options->nb_parameters < 1)
        {
            melissa_print (VERBOSE_ERROR, "simulations must have at least 1 variable parameter\n");
            stats_usage ();
            exit (1);
        }
    }

    if (options->nb_time_steps < 1)
    {
        melissa_print (VERBOSE_ERROR, "simulations must have at least 1 time step\n");
        stats_usage ();
        exit (1);
    }

    if (strlen(options->restart_dir) < 1)
    {
        melissa_print (VERBOSE_WARNING, "options->restart_dir= %s changing to .\n", options->restart_dir);
        sprintf (options->restart_dir, ".");
    }

    if (options->check_interval < 5.0)
    {
        melissa_print (VERBOSE_WARNING, "checkpoint interval too small, changing to 5.0\n");
        options->check_interval = 5.0;
    }

    if (options->timeout_simu < 5.0)
    {
        melissa_print (VERBOSE_WARNING, "time before simulation timeout too small, changing to 5.0\n");
        options->timeout_simu = 5.0;
    }
}
