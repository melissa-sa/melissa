from flowvrapp import *
from filters import *


# ------------- options ------------- #


parameters = "2:3:4"
nb_time_steps = 3
operations = "variance:max:threshold"
vect_size = 17
threshold = 7.6
max_sobol_order = 1
options = " -p " + parameters\
        + " -t " + str(nb_time_steps)\
        + " -o " + operations\
        + " -n " + str(vect_size)\
        + " -e " + str(threshold)\
        + " -s " + str(max_sobol_order)


# ------------- composites ------------- #


class parallel_data_gen(Composite):
    def __init__(self, prefix, hosts, cmdline, MPI):
        Composite.__init__(self)

        hosts_list = hosts.split()
        ninstance = len(hosts_list)

        if MPI:
            hosts = hosts_list[0]
            for i in range(1, ninstance):
                hosts = hosts + "," + hosts_list[i]
            data_run = FlowvrRunOpenMPI(cmdline, hosts = hosts, prefix = prefix)
        else:
            data_run = FlowvrRunSSHMultiple(cmdline, hosts = hosts, prefix = prefix)

        all_beginIts = []
        all_out_vect1 = []
        all_out_vect2 = []

        for i in range(ninstance):
            data_gen = Module(prefix + "/" + str(i), run = data_run)
            data_gen.addPort("out_vect1", direction = "out")
            data_gen.addPort("out_vect2", direction = "out")
            all_beginIts.append(data_gen.getPort("beginIt"))
            all_out_vect1.append(data_gen.getPort("out_vect1"))
            all_out_vect2.append(data_gen.getPort("out_vect2"))

        self.ports["out_vect1"] = list(all_out_vect1)
        self.ports["out_vect2"] = list(all_out_vect2)
        self.ports["beginIt"] = list(all_beginIts)


class interfaces_flowvr(Composite):
    def __init__(self, prefix, hosts, cmdline):
        Composite.__init__(self)

        hosts_list = hosts.split()
        ninstance = len(hosts_list)
        opt_list = cmdline.split()
        for i in range(len(opt_list)):
            if opt_list[i] == "-n":
                local_vect_size = int(opt_list[i+1]) / ninstance + 1
                break
        for j in range(len(opt_list)):
            if opt_list[j] == "-n":
                opt_list[j+1] = str(local_vect_size)
        local_cmdline = ""
        for j in range(len(opt_list)):
            local_cmdline = local_cmdline + " " + opt_list[j]

        all_endIts = []
        all_in_vects = []

        for i in range(ninstance):
            if i == vect_size % ninstance:
                local_vect_size -= 1
                for j in range(len(opt_list)):
                    if opt_list[j] == "-n":
                        opt_list[j+1] = str(local_vect_size)
                        break
                local_cmdline = ""
                for j in range(len(opt_list)):
                    local_cmdline = local_cmdline + " " + opt_list[j]

            interface_flowvr = Module(prefix + "/" + str(i), local_cmdline, hosts_list[i])
            interface_flowvr.addPort("in_vect", direction = "in")
            all_endIts.append(interface_flowvr.getPort("endIt"))
            all_in_vects.append(interface_flowvr.getPort("in_vect"))

        self.ports["in_vect"] = list(all_in_vects)
        self.ports["endIt"] = list(all_endIts)


 # ------------- main ------------- #

example = 2

if example == 1:
    # Merge + Scatter: allows any spatial partitionning. Not communication efficient.
    commande = "./data_generator"
    data_gen = parallel_data_gen(prefix = "data_generator", hosts = "localhost " * 3, cmdline = commande + options, MPI = True)

    commande = "../../src/interface_flowvr"
    interface_flowvr1 = interfaces_flowvr(prefix = "interface1", hosts = "localhost " * 3, cmdline = commande + options)
    interface_flowvr2 = interfaces_flowvr(prefix = "interface2", hosts = "localhost " * 2, cmdline = commande + options)

    scatter1 = FilterScatter("interface1/scatter", 64)
    scatter2 = FilterScatter("interface2/scatter", 64)

    tree_data1 = generateNto1(prefix = "data_tree1", in_ports = data_gen.getPort("out_vect1"))
    tree_data2 = generateNto1(prefix = "data_tree2", in_ports = data_gen.getPort("out_vect2"))

    tree_data1.link(scatter1.getPort("in"))
    tree_data2.link(scatter2.getPort("in"))

    presignal_and = FilterSignalAnd("presignal_and")

    for i in range(len(interface_flowvr1.getPort("in_vect"))):
        scatter1.newOutputPort().link(interface_flowvr1.getPort("in_vect")[i])
        interface_flowvr1.getPort("endIt")[i].link(presignal_and.newInputPort())
    for i in range(len(interface_flowvr2.getPort("in_vect"))):
        scatter2.newOutputPort().link(interface_flowvr2.getPort("in_vect")[i])
        interface_flowvr2.getPort("endIt")[i].link(presignal_and.newInputPort())

    presignal = FilterPreSignal("presignal", nb = 1)
    presignal_and.getPort("out").link(presignal.getPort('in'))
    presignal.getPort('out').link(data_gen.getPort("beginIt"))


if example == 2:
    # N producers and M analyses, with N = k*M.
    M = 2
    k = 4
    N = M*k

    commande = "./data_generator"
    data_gen = parallel_data_gen(prefix = "data_generator", hosts = "localhost " * N, cmdline = commande + options, MPI = False)

    commande = "../../src/interface_flowvr"
    host_list_analyse = "localhost " * M
    hosts_list = host_list_analyse.split()

    data_vect_size = vect_size / N + 1

    presignal = FilterPreSignal("presignal", nb = 1)
    presignal_and = FilterSignalAnd("presignal_and")

    for i in range(M):
        analyse_vect_size = 0

        ports1 = []
        ports2 = []
        for j in range(k):
            if (k * i + j) == vect_size % N:
                data_vect_size -= 1
            ports1.append(data_gen.getPort("out_vect1")[k * i + j])
            ports2.append(data_gen.getPort("out_vect2")[k * i + j])
            analyse_vect_size += data_vect_size

        options = " -p " + parameters\
                + " -t " + str(nb_time_steps)\
                + " -o " + operations\
                + " -n " + str(analyse_vect_size)\
                + " -e " + str(threshold)\
                + " -s " + str(max_sobol_order)

        interface_flowvr1 = Module("interface1/" + str(i), commande + options, hosts_list[i])
        interface_flowvr2 = Module("interface2/" + str(i), commande + options, hosts_list[i])
        interface_flowvr1.addPort("in_vect", direction = "in")
        interface_flowvr2.addPort("in_vect", direction = "in")

        tree_data1 = generateNto1(prefix = "data_tree1/" + str(i), in_ports = ports1)
        tree_data2 = generateNto1(prefix = "data_tree2/" + str(i), in_ports = ports2)

        tree_data1.link(interface_flowvr1.getPort("in_vect"))
        tree_data2.link(interface_flowvr2.getPort("in_vect"))

        local_presignal_and = FilterSignalAnd("presignal_and" + str(i))
        interface_flowvr1.getPort("endIt").link(local_presignal_and.newInputPort())
        interface_flowvr2.getPort("endIt").link(local_presignal_and.newInputPort())
        local_presignal_and.getPort('out').link(presignal_and.newInputPort())

        for j in range(k):
            presignal.getPort('out').link(data_gen.getPort("beginIt")[k * i + j])
    presignal_and.getPort('out').link(presignal.getPort('in'))

app.generate_xml("test_flowvr")
