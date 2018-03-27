
#include "flowvr/module.h"

void FlowVRInit();

void SendToGroup(void* buff,
                 int buff_size);

void RecvFromGroup(void* buff);

void FlowVRClose();

std::vector<flowvr::Port*> ports;
flowvr::ModuleAPI*         module;

extern "C" void flowvr_init()
{
    FlowVRInit();
    return;
}

extern "C" void send_to_group(void* buff,
                              int buff_size)
{
    SendToGroup(buff, buff_size);
    return;
}

extern "C" void recv_from_group(void* buff)
{
    RecvFromGroup(buff);
    return;
}

extern "C" void flowvr_close()
{
    FlowVRClose();
    return;
}

class MelissaInPort : public flowvr::InputPort
{
public:
  MelissaInPort(const char* name="MelissaData")
    : InputPort(name),
      StampSobolRank("SobolRank",flowvr::TypeInt::create())
  {
      stamps->add(&StampSobolRank);
  }
  flowvr::StampInfo StampSobolRank;
};
MelissaInPort InPort("MelissaIn");

class MelissaOutPort : public flowvr::OutputPort
{
public:
  MelissaOutPort(const char* name="MelissaData")
    : OutputPort(name),
      StampSobolRank("SobolRank",flowvr::TypeInt::create())
  {
      stamps->add(&StampSobolRank);
  }
  flowvr::StampInfo StampSobolRank;
};
MelissaOutPort OutPort("MelissaOut");

void FlowVRInit()
{
    module = flowvr::initModule(ports);
    if (module == NULL)
    {
        // do something
        exit (1);
    }
    ports.push_back(&OutPort);
    ports.push_back(&InPort);

    return;
}

void SendToGroup(void* buff,
                 int   buff_size)
{
    flowvr::MessageWrite       m;

    m.data = module->alloc(buff_size);
    memcpy(m.data.writeAccess(),buff,buff_size);
    module->put(&OutPort,m);

    return;
}

void RecvFromGroup(void* buff)
{
    flowvr::Message            m;
    int                        msg_size;

    module->wait();
    module->get(&InPort,m);
    m.stamps.isValid(InPort.StampSobolRank);
//    m.stamps.read(port.StampSobolRank,sobol_rank);
    msg_size = m.data.getSize();
    memcpy (buff, m.data.readAccess(), msg_size);

    return;
}

void FlowVRClose()
{
    module->close();
}
