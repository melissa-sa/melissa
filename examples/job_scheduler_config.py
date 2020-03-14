from examples import batchspawner


spawner = batchspawner.SlurmSpawner()
spawner.req_nodes = '1'  ## NODES_SERVER / SIMU TODO: make separate?
spawner.req_tasks_per_node = '1'  ## PROC_PER_NODE
spawner.req_memory = '0'
spawner.req_runtime = '0:10:00'  # WALLTIME_SERVER / SIMU TODO: make separate?
spawner.req_server_output_log = 'melissa.%j.log'
spawner.req_server_error_log = 'melissa.%j.err'
spawner.req_simu_output_log = 'simu.%j.log'
spawner.req_simu_error_log = 'simu.%j.err'


spawner.server_script = '''#!/bin/bash
#SBATCH -N {nodes}
#SBATCH --ntasks-per-node={tasks_per_node}
#SBATCH --mem={memory}
#SBATCH --time={runtime}
#SBATCH --job-name=Melissa
#SBATCH -o {server_output_log}
#SBATCH -e {server_error_log}
#{options}
'''

spawner.simu_script = '''#!/bin/bash
#SBATCH -N {nodes}
#SBATCH --ntasks-per-node={tasks_per_node}
#SBATCH --time={runtime}
#SBATCH -o {simu_output_log}
#SBATCH -e {simu_error_log}
'''
