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


#include <flowvr/module.h>
#include <iostream>
#include <unistd.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
extern "C" {
#include "stats.h"
}

unsigned int flowvr_instance_count;
unsigned int flowvr_instance_rank;

class StatsOutputPort : public flowvr::OutputPort
{
public:
  StatsOutputPort(const char* name="stats", int size=1)
    : OutputPort(name),
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

static inline void gen_data (stats_data_t     *data,
                             stats_options_t  *options,
                             double           *out_vect,
                             int              *parameters,
                             int              *time_step,
                             int               rank,
                             int               my_vect_size)
{
    static int t = 0;
    int i = 0;

    while (parameters[i] >= options->size_parameters[i]-1)
    {
        parameters[i] = 0;
        i += 1;
        if (i==options->nb_parameters)
            break;
    }
    if (i<options->nb_parameters)
    {
        parameters[i] += 1;
    }
    else
    {
        ++t;
    }
    *time_step = t;

    for (i=0; i<my_vect_size; i++)
    {
        out_vect[i] = rank;
    }


}

int main(int argc, char** argv)
{
    stats_data_t     stats_data;
    stats_options_t  stats_options;
    int              time_step;
    double          *out_vect;
    int             *parameters;
    int              my_vect_size;
    int              comm_size, rank;
    int              i, opt;
    int              iteration, nb_iterations = 1;

#ifndef BUILD_WITH_MPI

    rank = 0;
    comm_size = 1;

#else // BUILD_WITH_MPI

    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

#endif // BUILD_WITH_MPI

    if (getenv("FLOWVR_NBPROC") == 0)
        flowvr::Parallel::init(rank, comm_size);
    else
        flowvr::Parallel::init(true);

    flowvr_instance_count = flowvr::Parallel::getNbProc();
    flowvr_instance_rank = flowvr::Parallel::getRank();

    stats_get_options (argc, argv, &stats_options);
    do
    {
        opt = getopt (argc, argv, "n");
        switch (opt) {
        case 'n':
            stats_data.vect_size = atoi (optarg);
        default:
            break;
        }

    } while (opt != -1);
    stats_data.vect_size = 17;
    init_data (&stats_data, &stats_options, stats_data.vect_size);
    my_vect_size = stats_data.vect_size / flowvr_instance_count;
    if (flowvr_instance_rank < stats_data.vect_size % flowvr_instance_count)
        my_vect_size += 1;
    out_vect   = (double*)malloc (my_vect_size * sizeof(double));
    parameters =    (int*)malloc (stats_options.nb_parameters * sizeof(int));
    memcpy (parameters, stats_options.size_parameters, stats_options.nb_parameters * sizeof(int));

    for (i=0; i< stats_options.nb_parameters; i++)
    {
        nb_iterations *= (stats_options.size_parameters[i]);
    }
    nb_iterations *= stats_options.nb_time_steps ;

    StatsOutputPort port_vector1("out_vect1", stats_options.nb_parameters);
    StatsOutputPort port_vector2("out_vect2", stats_options.nb_parameters);
    std::vector<flowvr::Port*> ports;
    ports.push_back(&port_vector1);
    ports.push_back(&port_vector2);

    flowvr::ModuleAPI* flowvr = flowvr::initModule(ports);
    if (flowvr == NULL)
    {
      return 1;
    }

    while (flowvr->wait())
    {
      flowvr::MessageWrite vect1;
      flowvr::MessageWrite vect2;

      // Generate data
      gen_data(&stats_data, &stats_options, out_vect, parameters, &time_step, flowvr_instance_rank, my_vect_size);

      // Build data
      vect1.data  = flowvr->alloc (my_vect_size * sizeof(double));
      vect2.data  = flowvr->alloc (my_vect_size * sizeof(double));

      // Stamps
      vect1.stamps.write(port_vector1.StampT,time_step);
      vect1.stamps.write(port_vector1.StampRank,rank);
      for (i=0; i<stats_options.nb_parameters; i++)
          vect1.stamps.write(port_vector1.StampParam[i],parameters[i]);
      vect1.stamps.write(port_vector1.StampName,"field1");
      vect2.stamps.write(port_vector2.StampT,time_step);
      vect2.stamps.write(port_vector1.StampRank,rank);
      for (i=0; i<stats_options.nb_parameters; i++)
          vect2.stamps.write(port_vector2.StampParam[i],parameters[i]);
      vect2.stamps.write(port_vector2.StampName,"field2");

      // Write messages
      memcpy (vect1.data.writeAccess(), out_vect, my_vect_size * sizeof(double));
      for (i=0; i<my_vect_size; i++)
          out_vect[i] += flowvr_instance_count;
      memcpy (vect2.data.writeAccess(), out_vect, my_vect_size * sizeof(double));

      // Send messages
      flowvr->put(&port_vector1,vect1);
      flowvr->put(&port_vector2,vect2);

      iteration++;
      if(time_step > stats_options.nb_time_steps)
      {
        break;
      }
//      if (iteration >= nb_iterations)
//      {
//          break;
//      }
    }

    flowvr->close();

    free (out_vect);
    free (parameters);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
