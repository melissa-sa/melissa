
#include "flowvr/module.h"
#include <iostream>
#include <unistd.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
extern "C" {
#include "stats.h"
}

#ifndef MAX_FIELD_NAME
#define MAX_FIELD_NAME 128
#endif

class StatsInputPort : public flowvr::InputPort
{
public:
  StatsInputPort(const char* name="stats", int size=1)
    : InputPort(name),
      StampT("T",flowvr::TypeInt::create()),
      StampRank("Rank",flowvr::TypeInt::create()),
      StampParam("Param",flowvr::TypeArray::create(size,flowvr::TypeInt::create())),
      StampName("Name",flowvr::TypeString::create())
  {
    stamps->add(&StampT); // add the stamp T to the stamps list for this port
    stamps->add(&StampRank); // add the stamp Rank to the stamps list for this port
    stamps->add(&StampParam); // add the stamp Param to the stamps list for this port
    stamps->add(&StampName); // add the stamp Name to the stamps list for this port
  }
  flowvr::StampInfo StampT;
  flowvr::StampInfo StampRank;
  flowvr::StampInfo StampParam;
  flowvr::StampInfo StampName;
};

int main(int argc, char** argv)
{
    stats_data_t     stats_data;
    stats_options_t  stats_options;
    int              iteration, nb_iterations = 1;
    int              time_step;
    double          *in_vect;
    int             *parameters;
    int              i;
    int              opt;
    int              client_rank;
    std::string      field_name;

    stats_get_options (argc, argv, &stats_options);
    print_options (&stats_options);
    do
    {
        opt = getopt (argc, argv, "p:t:o:e:s:h:n");
        switch (opt) {
        case 'n':
            stats_data.vect_size = atoi (optarg);
        default:
            break;
        }

    } while (opt != -1);
    stats_data.vect_size = 17;
    init_data (&stats_data, &stats_options, stats_data.vect_size);
    in_vect    = (double*)malloc (stats_data.vect_size * sizeof(double));
    parameters = (int*)   malloc (stats_options.nb_parameters * sizeof(int));

    for (i=0; i< stats_options.nb_parameters; i++)
    {
        nb_iterations *= (stats_options.size_parameters[i]);
    }
    nb_iterations *= stats_options.nb_time_steps ;

//    StatsInputPort port_vector("in_vect",stats_data.nb_parameters);
//    flowvr::InputPort port_vector("in_vect");
//    flowvr::StampInfo StampT("T",flowvr::TypeInt::create());
//    flowvr::StampInfo StampParam("Param",flowvr::TypeArray::create(stats_data.nb_parameters,flowvr::TypeInt::create()));
//    port_vector.stamps->add(&StampT);
//    port_vector.stamps->add(&StampParam);

    StatsInputPort port_vector("in_vect", stats_options.nb_parameters);
    std::vector<flowvr::Port*> ports;
    ports.push_back(&port_vector);

    flowvr::ModuleAPI* flowvr = flowvr::initModule(ports);
    if (flowvr == NULL)
    {
        return 1;
    }

    while (flowvr->wait())
    {
        // Get Messages
        flowvr::Message vect;
        flowvr->get(&port_vector,vect);

        // Read stamps
        vect.stamps.isValid(port_vector.StampT);
        vect.stamps.read(port_vector.StampT,time_step);
//        vect.stamps.read(port_vector.StampRank,client_rank);
        for (i=0; i<stats_options.nb_parameters; i++)
            vect.stamps.read(port_vector.StampParam[i],parameters[i]);
        vect.stamps.read(port_vector.StampName,field_name);

        // Print stamps
        printf("parameters");
        for (i=0; i<stats_options.nb_parameters; i++)
            printf(" %d", parameters[i]);
        printf("\n");
        printf("t = %d\n", time_step);

        // Read data
        memcpy(in_vect, vect.data.readAccess(), stats_data.vect_size * sizeof(double));

        // TODO find an other way to end the loop
//        if(time_step > stats_options.nb_time_steps)
//        {
//            break;
//        }

        printf("t = %d, client rank = %d\n", time_step, client_rank);
        printf(" parameters");
        for (i=0; i<stats_options.nb_parameters; i++)
            printf(" %d", parameters[i]);
        printf("\n");

        // Compute stats
        compute_stats (&stats_data, time_step-1, 1, &in_vect);
        iteration++;
        if (iteration >= nb_iterations)
        {
            break;
        }
    }

    finalize_stats (&stats_data);

    free_data (&stats_data);
    free (in_vect);
    free (parameters);

    flowvr->close();

    return 0;
}
