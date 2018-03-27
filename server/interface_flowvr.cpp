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


#include "flowvr/module.h"
#include <iostream>
#include <unistd.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
#include <zmq.h>

#ifndef MAX_FIELD_NAME
#define MAX_FIELD_NAME 128
#endif

#ifndef BUILD_WITH_MPI
typedef int MPI_Comm; /**< Convert MPI_Comm to int when built without MPI */
#endif // BUILD_WITH_MPI

class MelissaDataPort : public flowvr::InputPort
{
public:
  MelissaDataPort(const char* name="MelissaData")
    : InputPort(name),
      StampT("T",flowvr::TypeInt::create()),
      StampSobolRank("SobolRank",flowvr::TypeInt::create()),
      StampSampleID("SampleID",flowvr::TypeInt::create()),
      StampSize("Size",flowvr::TypeInt::create()),
      StampName("Name",flowvr::TypeString::create())
  {
      stamps->add(&StampT); // add the stamp T to the stamps list for this port
      stamps->add(&StampSobolRank);
      stamps->add(&StampSampleID);
      stamps->add(&StampSize);
      stamps->add(&StampName);
  }
  flowvr::StampInfo StampT;
  flowvr::StampInfo StampSobolRank;
  flowvr::StampInfo StampSampleID;
  flowvr::StampInfo StampSize;
  flowvr::StampInfo StampName;
};

int main(int argc, char** argv)
{
    int              time_step;
    double          *in_vect;
    int              msg_size;
    int              vect_size;
    int              comm_size;
    int              sobol_rank;
    int              sample_id;
    const int       *coupling;
    int              rank = 0;
    std::string      name;
    char             field_name[MAX_FIELD_NAME];
    zmq_msg_t        msg;
    char            *buff_ptr=NULL;

    MelissaDataPort port("data");
    std::vector<flowvr::Port*> ports;
    ports.push_back(&port);


    flowvr::ModuleAPI* flowvr = flowvr::initModule(ports);
    if (flowvr == NULL)
    {
        return 1;
    }

    while (flowvr->wait())
    {
        // Get Messages
        flowvr::Message m;
        flowvr->get(&port,m);

        // Read stamps
        m.stamps.isValid(port.StampT);
        m.stamps.read(port.StampT,time_step);
        if (time_step > 0)
        {
            m.stamps.read(port.StampSobolRank,sobol_rank);
            m.stamps.read(port.StampSampleID,sample_id);
            m.stamps.read(port.StampSize,vect_size);
            m.stamps.read(port.StampName,name);
            strcpy(field_name, name.c_str());
        }
        msg_size = m.data.getSize();
        zmq_msg_init_size (&msg, msg_size + 4 * sizeof(int) + MAX_FIELD_NAME);
        buff_ptr = (char*)zmq_msg_data (&msg);
        memcpy(buff_ptr, &time_step, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy(buff_ptr, &sobol_rank, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy(buff_ptr, &sample_id, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy(buff_ptr, &rank, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy(buff_ptr, &vect_size, sizeof(int));
        buff_ptr += sizeof(int);
        memcpy (buff_ptr, field_name, MAX_FIELD_NAME);
        buff_ptr += MAX_FIELD_NAME;
        memcpy (buff_ptr, m.data.readAccess(), msg_size);
//        zmq_msg_send (&msg, ?????, 0);
    }

    flowvr->close();

    return 0;
}
