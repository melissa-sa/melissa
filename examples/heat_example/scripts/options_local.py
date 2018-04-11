###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################


"""
    Heat example user defined functions for local execution
"""

EXECUTABLE='heatc'
NODES_SERVER = 3
NODES_GROUP = 2

def launch_server(server):
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory'])):
        os.mkdir(GLOBAL_OPTIONS['working_directory'])
    os.chdir(GLOBAL_OPTIONS['working_directory'])
    print 'mpirun ' + ' -n '+str(NODES_SERVER) + ' ' + server.path + '/melissa_server ' + server.cmd_opt + ' &'
    server.job_id = subprocess.Popen(('mpirun ' +
                                      ' -n '+str(NODES_SERVER) +
                                      ' ' + server.path +
                                      '/melissa_server ' +
                                      server.cmd_opt +
                                      ' &').split()).pid
    os.chdir(GLOBAL_OPTIONS['working_directory'])

def launch_simu(simulation):
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))):
        os.mkdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    copyfile(GLOBAL_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
    if MELISSA_STATS['sobol_indices']:
        command = 'mpirun '
        for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
            command += ' '.join(('-n',
                                 str(NODES_GROUP),
                                 '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                 str(simulation.simu_id[i]), str(simulation.coupling),
                                 ' '.join(str(j) for j in simulation.param_set[i]),
                                 ': '))
        print command[:-2]
#        if BUILD_WITH_FLOWVR == 'ON':
#            args = []
#            for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
#                args.append(str(simulation.simu_id[i])+" "+' '.join(str(j) for j in simulation.param_set[i]))
#            create_flowvr_group('@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
#                                args,
#                                simulation.rank,
#                                int(NODES_GROUP),
#                                STUDY_OPTIONS['nb_parameters'])
#            os.system('python create_group'+str(simulation.rank)+'.py')
#            simulation.job_id = subprocess.Popen('flowvr group'+str(simulation.rank), shell=True).pid
#        else:
        simulation.job_id = subprocess.Popen(command[:-2].split()).pid
    else:
        if BUILD_EXAMPLES_WITH_MPI == 'ON':
            command = ' '.join(('mpirun',
                                 '-n',
                                 str(NODES_GROUP),
                                 '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                 str(simulation.simu_id),
                                 ' '.join(str(i) for i in simulation.param_set)))
        else:
            command = ' '.join(('@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                str(0),
                                str(simulation.rank),
                                ' '.join(str(i) for i in simulation.param_set)))
        print command
        simulation.job_id = subprocess.Popen(command.split()).pid
        os.chdir(GLOBAL_OPTIONS['working_directory'])

def check_job(job):
    state = 0
    try:
        subprocess.check_output(["ps",str(job.job_id)])
        state = 1
    except:
        state = 2
    job.job_status = state

def check_load():
    try:
        subprocess.check_output(["pidof",EXECUTABLE])
        return False
    except:
        return True

def kill_job(job):
    os.system('kill '+str(job.job_id))

def heat_visu():
    os.chdir('@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/STATS')
    fig = list()
    nb_time_steps = str(STUDY_OPTIONS['nb_time_steps'])
    matrix = np.zeros((100,100))

    if (MELISSA_STATS['mean']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat1_mean.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Means')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['variance']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat1_variance.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Variances')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['min']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat1_min.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Minimum')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['max']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat1_max.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Maximum')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['threshold_exceedance']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat1_threshold.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = int(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Threshold exceedance')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['quantile']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat1_quantile.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Quantiles')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['sobol_indices']):
        for param in range(STUDY_OPTIONS['nb_parameters']):
            fig.append(plt.figure(len(fig)))
            file_name = 'results.heat1_sobol'+str(param)+'.'+nb_time_steps
            file=open(file_name)
            value = 0
            for line in file:
                matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
                value += 1
            plt.pcolor(matrix,cmap=cm.coolwarm)
            plt.colorbar().set_label('Sobol\'s indices '+str(param))
            fig[len(fig)-1].show()
            file.close()
        for param in range(STUDY_OPTIONS['nb_parameters']):
            fig.append(plt.figure(len(fig)))
            file_name = 'results.heat1_sobol_tot'+str(param)+'.'+nb_time_steps
            file=open(file_name)
            value = 0
            for line in file:
                matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
                value += 1
            plt.pcolor(matrix,cmap=cm.coolwarm)
            plt.colorbar().set_label('Total order Sobol\'s indices '+str(param))
            fig[len(fig)-1].show()
            file.close()

    raw_input()
