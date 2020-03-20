import batchspawner


scheduler = batchspawner.SlurmSpawner()
scheduler.req_nodes = '1'
scheduler.req_tasks_per_node = '1'
scheduler.req_memory = '0'
scheduler.req_runtime = '0:10:00'
scheduler.req_server_output_log = 'melissa.%j.log'
scheduler.req_server_error_log = 'melissa.%j.err'
scheduler.req_simu_output_log = 'simu.%j.log'
scheduler.req_simu_error_log = 'simu.%j.err'


scheduler.server_script = '''#!/bin/bash
#SBATCH -N {nodes}
#SBATCH --ntasks-per-node={tasks_per_node}
#SBATCH --mem={memory}
#SBATCH --time={runtime}
#SBATCH --job-name=Melissa
#SBATCH -o {server_output_log}
#SBATCH -e {server_error_log}
#{options}
'''

scheduler.simu_script = '''#!/bin/bash
#SBATCH -N {nodes}
#SBATCH --ntasks-per-node={tasks_per_node}
#SBATCH --time={runtime}
#SBATCH -o {simu_output_log}
#SBATCH -e {simu_error_log}
'''
