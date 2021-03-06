## Copyright (c) Regents of the University of Minnesota
## Copyright (c) Michael Gilbert
## Distributed under the terms of the Modified BSD License.

"""
Batch spawners.
This file contains an abstraction layer for batch job queueing systems,
and implements Jupyterhub spawners for Torque, SLURM, and eventually
others.
Common attributes of batch submission / resource manager environments
will include notions of:
  * queue names, resource manager addresses
  * resource limits including runtime, number of processes, memory
  * singleuser child process running on (usually remote) host not known
  until runtime
  * job submission and monitoring via resource manager utilities
  * remote execution via submission of templated scripts
  * job names instead of PIDs
"""


import jupyterhub.spawner, traitlets, async_generator, pwd, jinja2, asyncio, re, tornado, getpass, xml, os, subprocess


def format_template(template, *args, **kwargs):
    """
    Format a template, either using jinja2 or str.format().
    Use jinja2 if the template is a jinja2.Template, or contains '{{' or
    '{%'.  Otherwise, use str.format() for backwards compatability with
    old scripts (but you can't mix them).
    :param jinja2.Template | str template:
    :rtype: jinja2.Template | str
    """
    if isinstance(template, jinja2.Template):
        return template.render(*args, **kwargs)
    elif '{{' in template or '{%' in template:
        return jinja2.Template(template).render(*args, **kwargs)
    else:
        return template.format(*args, **kwargs)


class BatchSpawnerBase(jupyterhub.spawner.Spawner):
    """
    Base class for spawners using resource manager batch job submission
    mechanisms.
    This base class is developed targetting the TorqueSpawner and
    SlurmSpawner, so by default assumes a qsub-like command that reads a
    script from its stdin for starting jobs, a qstat-like command that
    outputs some data that can be parsed to check if the job is running
    and on what remote node, and a qdel-like command to cancel a job. The
    goal is to be sufficiently general that a broad range of systems can
    be supported with minimal overrides.
    At minimum, subclasses should provide reasonable defaults for the
    traits:
        batch_script
        batch_submit_cmd
        batch_query_cmd
        batch_cancel_cmd
    and must provide implementations for the methods:
        state_ispending
        state_isrunning
        state_gethost
    """

    exec_prefix = traitlets.Unicode('',
            help='standard executon prefix (e.g. sudo -E -u {username})'
        ).tag(config=True)
    ## all these req_foo traits will be available as substvars for
    ## templated strings
    req_queue = traitlets.Unicode('',
            help='queue name to submit job to resource manager'
        ).tag(config=True)
    req_host = traitlets.Unicode('',
            help='''host name of batch server to submit job to resource
                    manager'''
        ).tag(config=True)
    req_memory = traitlets.Unicode('',
            help='memory to request from resource manager'
        ).tag(config=True)
    req_nodes = traitlets.Unicode('',
            help='number of nodes allocated to a job'
        ).tag(config=True)
    req_tasks_per_node = traitlets.Unicode('',
            help='number of tasks invoked on each node'
        ).tag(config=True)
    req_nprocs = traitlets.Unicode('',
            help='number of processors to request from resource manager'
        ).tag(config=True)
    req_ngpus = traitlets.Unicode('',
            help='number of GPUs to request from resource manager'
        ).tag(config=True)
    req_runtime = traitlets.Unicode('',
            help='length of time for submitted job to run'
        ).tag(config=True)
    req_partition = traitlets.Unicode('',
            help='partition name to submit job to resource manager'
        ).tag(config=True)
    req_account = traitlets.Unicode('',
            help='account name string to pass to the resource manager'
        ).tag(config=True)
    req_server_output_log = traitlets.Unicode('',
            help='server batch script standard output file'
        ).tag(config=True)
    req_server_error_log = traitlets.Unicode('',
            help='server batch script standard error file'
        ).tag(config=True)
    req_simu_output_log = traitlets.Unicode('',
            help='simulation batch script standard output file'
        ).tag(config=True)
    req_simu_error_log = traitlets.Unicode('',
            help='simulation batch script standard error file'
        ).tag(config=True)
    req_options = traitlets.Unicode('',  ## TODO
            help='other options to include into job submission script'
        ).tag(config=True)
    req_prologue = traitlets.Unicode('',  ## TODO
            help='script to run before single user server starts'
        ).tag(config=True)
    req_epilogue = traitlets.Unicode('',  ## TODO
            help='script to run after single user server ends'
        ).tag(config=True)
    req_username = traitlets.Unicode(getpass.getuser(),
            help='name of a user running a job'
        ).tag(config=True)
    ## useful IF getpwnam on submit host returns correct info for exec host
    req_homedir = traitlets.Unicode(os.path.expanduser('~'),
            help='home directory of a user running a job'
        ).tag(config=True)
    req_keepvars = traitlets.Unicode()
    @traitlets.default('req_keepvars')
    def _req_keepvars_default(self):
        return ','.join(self.get_env().keys())
    req_keepvars_extra = traitlets.Unicode(
            help='''extra environment variables which should be
                    configured, added to the defaults in keepvars, comma
                    separated list''')
    batch_script = traitlets.Unicode('',
            help='''template for job submission script; traits on this
                    class named like req_xyz will be substituted in the
                    template for {xyz} using string.Formatter'''
        ).tag(config=True)
    ## raw output of job submission command unless overridden
    ## initially env var template, substituted later with an actual number
    job_id = traitlets.Unicode()
    ## will get the raw output of the job status command unless overridden
    job_status = traitlets.Unicode()
    batch_submit_cmd = traitlets.Unicode('',
            help='''command to run to submit batch scripts; formatted
                    using req_xyz traits as {xyz}'''
        ).tag(config=True)
    ## override if your batch system needs something more elaborate to
    ## read the job status
    batch_query_cmd = traitlets.Unicode('',
            help='''command to run to view information about jobs located
                    in a Slurm scheduling queue; usually requires
                    providing opt_query_... as well'''
        ).tag(config=True)
    opt_query_state = traitlets.Unicode('',
            help='''batch_query_cmd options; formatted using self.job_id
                    as {job_id}'''
        ).tag(config=True)
    opt_query_cpus = traitlets.Unicode('',
            help='''batch_query_cmd options; formatted using self.job_id
                    as {job_id}'''
        ).tag(config=True)
    opt_query_cpus_name = traitlets.Unicode('',
            help='''batch_query_cmd options; formatted using self.job_id
                    as {job_id}'''
        ).tag(config=True)
    opt_query_time_name = traitlets.Unicode('',
            help='''batch_query_cmd options; formatted using self.job_id
                    as {job_id}'''
        ).tag(config=True)
    opt_query_user = traitlets.Unicode('',
            help='''command to run to read user's jobs status; formatted
                    using req_xyz traits as {xyz} and req_username as
                    {username}; returns lines count excluding header'''
        ).tag(config=True)
    batch_cancel_cmd = traitlets.Unicode('',
            help='''command to stop/cancel a previously submitted job;
                    formatted like batch_query_cmd'''
        ).tag(config=True)

    startup_poll_interval = traitlets.Float(0.5,
            help='''polling interval (seconds) to check job state during
                    startup'''
        ).tag(config=True)

    def get_req_subvars(self):
        """
        Preparing substitution variables for templates using req_xyz
        traits.
        :return: Substitution variables.
        :rtype: dict
        """
        subvars = {}
        for t in [t for t in self.trait_names() if t.startswith('req_')]:
            subvars[t[4:]] = getattr(self, t, '')
        if subvars.get('keepvars_extra'):
            subvars['keepvars'] += ',{}'.format(subvars['keepvars_extra'])
        return subvars

    def parse_job_id(self, output):
        """
        Parsing output of submit command to get job id.
        :param str output: Output of submit command, scheduler-specific.
        :raises NotImplementedError: Must be implemented in a subclass.
        """
        raise NotImplementedError('Subclass must provide implementation')

    async def run_command(self, cmd, cmd_input=None, env=None):
        """
        Running a job scheduler command.
        :param str cmd: Job scheduler command.
        :param str | None cmd_input: Batch script or None.
        :param env: [description], defaults to None
        :type env: [type], optional
        :return: Output of a job scheduler command.
        :rtype: str
        """
        proc = await asyncio.create_subprocess_shell(
            cmd,
            asyncio.subprocess.PIPE,
            asyncio.subprocess.PIPE,
            asyncio.subprocess.PIPE,
            env=env)
        inbytes = None
        if cmd_input:
            inbytes = cmd_input.encode()
        # try:
        out, eout = await proc.communicate(inbytes)
        # except:  ## TODO: except what
        #     self.log.debug(
        #         'exception raised when trying to run command: {}'
        #         .format(inbytes))
        #     proc.kill()
        #     self.log.debug('running command failed done kill')
        #     out, eout = await proc.communicate()
        #     out = out.decode.strip()
        #     eout = eout.decode.strip()
        #     self.log.error('subprocess returned exitcode {}'
        #                     .format(proc.returncode))
        #     self.log.error('stdout:')
        #     self.log.error(out)
        #     self.log.error('stderr:')
        #     self.log.error(eout)
        #     raise RuntimeError('{} exit status {}: {}'
        #                         .format(cmd, proc.returncode, eout))
        # else:
        eout = eout.decode().strip()
        err = proc.returncode
        if err != 0:
            self.log.error('subprocess returned exitcode {}'.format(err))
            self.log.error(eout)
            raise RuntimeError(eout)
        return out.decode().strip()

    async def _get_batch_script(self, **subvars):
        """
        Formating batch script from vars.
        :return: Formatted script.
        :rtype: str
        """
        ## could be overridden by subclasses, but mainly useful for testing
        return format_template(self.batch_script, **subvars)

    async def submit_batch_script(self):
        """
        Submitting a formated script to queueing system.
        :return: Job id.
        :rtype: str
        """
        subvars = self.get_req_subvars()
        ## `cmd` is submitted to the batch system
        cmd = ' '.join((format_template(self.exec_prefix, **subvars),
                        format_template(self.batch_submit_cmd, **subvars)))
        if hasattr(self, 'user_options'):
            subvars.update(self.user_options)
        script = await self._get_batch_script(**subvars)
        self.log.info('Spawner submitting job using ' + cmd)
        self.log.info('Spawner submitted script:\n' + script)
        out = await self.run_command(cmd, script)  #, env=self.get_env())
        # try:
        self.log.info('Job submitted. cmd: ' + cmd + ' output: ' + out)
        self.job_id = self.parse_job_id(out)
        # except:  ## TODO: except what
        #     self.log.error('Job submission failed with exit code ' + out)
        #     self.job_id = ''
        return self.job_id

    async def read_job_state(self, query_cmd):
        """
        Getting job status (running, pending, etc.) based on a query.
        :param traitlets.Unicode query_cmd: Query with parameters.
        :return: Job status.
        :rtype: str
        """
        if self.job_id is None or len(self.job_id) == 0:
            ## job not running
            self.job_status = ''
            return self.job_status
        subvars = self.get_req_subvars()
        subvars['job_id'] = self.job_id
        cmd = ' '.join((format_template(self.exec_prefix, **subvars),
                        format_template(query_cmd, **subvars)))
        self.log.debug('Spawner querying job: ' + cmd)
        # try:
        self.job_status = await self.run_command(cmd)
        # except Exception as e:  ## TODO: except what
        #     self.log.error('Error querying job ' + self.job_id)
        #     self.job_status = ''
        # finally:
        return self.job_status

    async def cancel_batch_job(self):
        """
        Cancelling a job by job_id.
        """
        subvars = self.get_req_subvars()
        subvars['job_id'] = self.job_id
        cmd = ' '.join((format_template(self.exec_prefix, **subvars),
                        format_template(self.batch_cancel_cmd, **subvars)))
        self.log.info('cancelling job {}: {}'.format(self.job_id, cmd))
        await self.run_command(cmd)

    def load_state(self, state):
        """
        Load job_id from state.
        """
        super(BatchSpawnerBase, self).load_state(state)
        self.job_id = state.get('job_id', '')
        self.job_status = state.get('job_status', '')

    def get_state(self):
        """
        Adding job_id to state.
        """
        state = super(BatchSpawnerBase, self).get_state()
        if self.job_id:
            state['job_id'] = self.job_id
        if self.job_status:
            state['job_status'] = self.job_status
        return state

    def clear_state(self):
        """
        Clearing job_id and job status.
        """
        super(BatchSpawnerBase, self).clear_state()
        self.job_id = ''
        self.job_status = ''

    def make_preexec_fn(self, name):
        """
        Making preexec function to change uid (if running as root) before
        job submission.
        """
        return jupyterhub.spawner.set_user_setuid(name)

    def state_ispending(self):
        """
        Returning boolean indicating if job is still waiting to run,
        likely by parsing self.job_status.
        :raises NotImplementedError: Must be implemented in a subclass.
        """
        raise NotImplementedError('Subclass must provide implementation')

    def state_isrunning(self):
        """
        Returning boolean indicating if job is running, likely by parsing
        self.job_status.
        :raises NotImplementedError: Must be implemented in a subclass.
        """
        raise NotImplementedError('Subclass must provide implementation')

    def state_gethost(self):
        """
        Returning string, hostname or addr of running job, likely by
        parsing self.job_status.
        :raises NotImplementedError: Must be implemented in a subclass.
        """
        raise NotImplementedError('Subclass must provide implementation')

    async def poll(self):
        """
        Polling a process by job_id.
        :return: None for running or pending job, 1 otherwise.
        :rtype: str | NoneType
        """
        if self.job_id is not None and len(self.job_id) > 0:
            await self.read_job_state('{} {}'.format(
                self.batch_query_cmd, self.opt_query_state))
            if self.state_isrunning() or self.state_ispending():
                return None
            else:
                self.clear_state()
                return 1
        if not self.job_id:
            ## no job id means it's not running
            self.clear_state()
            return 1

    def getTotalCPUCount(self):
        """
        Getting the number of CPUs allocated to specified jobs.
        :raises NotImplementedError: Must be implemented in a subclass.
        """
        raise NotImplementedError('Subclass must provide implementation')

    def getCPUCountByJob(self):
        """
        Getting the number of CPUs allocated to specified jobs along with
        job names.
        :raises NotImplementedError: Must be implemented in a subclass.
        """
        raise NotImplementedError('Subclass must provide implementation')

    def queryScheduler(self, query):
        """
        Generic query scheme.
        :param str query: Query specific to job scheduler.
        :return: Raw scheduler response.
        :rtype: str
        """
        process = subprocess.Popen(
            self.batch_query_cmd + ' ' + format_template(
                query, **self.get_req_subvars()),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
            universal_newlines=True)
        return process.communicate()[0]

    async def start(self):
        """
        Start-up method.
        Submitting batch script and monitoring the progess.
        :rtype: str, int
        """
        await self.submit_batch_script()
        ## we are called with a timeout, and if the timeout expires this
        ## function will be interrupted at the next yield, and self.stop()
        ## will be called. so this function should not return unless
        ## successful, and if unsuccessful should either raise an
        ## exception or loop forever
        if len(self.job_id) == 0:
            raise RuntimeError(
                'Jupyter batch job submission failure (no jobid in output)'
                )
        while True:
            await self.poll()
            if self.state_isrunning():
                break
            else:
                if self.state_ispending():
                    self.log.debug('Job {} still pending'
                                   .format(self.job_id))
                else:
                    self.log.warning(
                        'job {} neither pending nor running.\n {}'
                        .format(self.job_id, self.job_status))
                    raise RuntimeError(
                        '''the Jupyter batch job has disappeared while
                           pending in the queue or died immediately after
                           starting''')
            await tornado.gen.sleep(self.startup_poll_interval)
        self.log.info('notebook server job {} started'.format(self.job_id))

    async def stop(self, now=False):
        """
        Stopping the singleuser server job.
        Returns immediately after sending job cancellation command if
        now=True, otherwise tries to confirm that job is no longer
        running.
        """
        self.log.info('stopping server job {}'.format(self.job_id))
        await self.cancel_batch_job()
        if now:
            return
        for i in range(10):
            await self.poll()
            if not self.state_isrunning():
                return
            await tornado.gen.sleep(1.0)
        if self.job_id:
            self.log.warning(
                'Notebook server job {} possibly failed to terminate'
                .format(self.job_id))

    @async_generator.async_generator
    async def progress(self):
        """
        Monitoring job status.
        """
        while True:
            if self.state_ispending():
                await async_generator.yield_({
                    'message': 'Pending in queue...',})
            elif self.state_isrunning():
                await async_generator.yield_({
                    'message': 'Cluster job running... waiting to connect',
                })
                return
            else:
                await async_generator.yield_({
                    'message': 'Unknown status...',})
            await tornado.gen.sleep(1)


class BatchSpawnerRegexStates(BatchSpawnerBase):
    """
    Subclass of BatchSpawnerBase that uses config-supplied regular
    expressions to interact with batch submission system state.
    Provides implementations of
    * state_ispending
    * state_isrunning
    * state_gethost
    In their place, the user should supply the following configuration:
    * state_pending_re - regex that matches job_status if job is waiting
    to run
    * state_running_re - regex that matches job_status if job is running
    * state_exechost_re - regex with at least one capture group that
    extracts execution host from job_status
    * state_exechost_exp - if empty, notebook IP will be set to the
    contents of the first capture group. If this variable is set, the
    match object will be expanded using this string to obtain the
    notebook IP. See Python docs: re.match.expand.
    """

    state_pending_re = traitlets.Unicode('',
            help='regex that matches job_status if job is waiting to run'
        ).tag(config=True)
    state_running_re = traitlets.Unicode('',
            help='regex that matches job_status if job is running'
        ).tag(config=True)
    state_exechost_re = traitlets.Unicode('',
            help='''regex with at least one capture group that extracts
                    the execution host from job_status output '''
                ).tag(config=True)
    state_exechost_exp = traitlets.Unicode('',
            help='''if empty, notebook IP will be set to the contents of
                    the first capture group; if this variable is set, the
                    match object will be expanded using this string to
                    obtain the notebook IP; see Python docs:
                    re.match.expand '''
        ).tag(config=True)

    def state_ispending(self):
        """
        Querying for "pending" status.
        :return: True for pending, False otherwise.
        :rtype: bool
        """
        assert self.state_pending_re, 'misconfigured: define state_pending_re'
        if self.job_status and re.search(self.state_pending_re,
                                         self.job_status):
            return True
        else:
            return False

    def state_isrunning(self):
        """
        Querying for "running" status.
        :return: True for running, False otherwise.
        :rtype: bool
        """
        assert self.state_running_re, 'misconfigured: define state_running_re'
        if self.job_status and re.search(self.state_running_re,
                                         self.job_status):
            return True
        else:
            return False

    def state_gethost(self):
        """
        Returning hostname or addr of running job, by parsing
        self.job_status.
        :return: Hostname or addr of running job.
        :rtype: str
        """
        assert self.state_exechost_re, 'misconfigured: define state_exechost_re'
        match = re.search(self.state_exechost_re, self.job_status)
        if not match:
            self.log.error(
                'spawner unable to match host addr in job status: {}'
                .format(self.job_status))
            return
        elif not self.state_exechost_exp:
            return match.groups()[0]
        else:
            return match.expand(self.state_exechost_exp)


class TorqueSpawner(BatchSpawnerRegexStates):

    batch_script = traitlets.Unicode(
'''#!/bin/sh
#PBS -q {queue}@{host}
#PBS -l walltime={runtime}
#PBS -l nodes={nodes}:ppn={nprocs}
#PBS -l mem={memory}
#PBS -N jupyterhub-singleuser
#PBS -v {keepvars}
#PBS {options}

{prologue}
{epilogue}
''').tag(config=True)
    job_id = traitlets.Unicode('$PBS_JOBID'
        ).tag(config=True)
    ## outputs job id string
    batch_submit_cmd = traitlets.Unicode('qsub'
        ).tag(config=True)
    ## outputs job data XML string
    batch_query_cmd = traitlets.Unicode('qstat').tag(config=True)
    opt_query_state = traitlets.Unicode('-x {job_id}').tag(config=True)
    opt_query_user = traitlets.Unicode('-u {username} | wc -l'
        ).tag(config=True)
    batch_cancel_cmd = traitlets.Unicode('qdel {job_id}'
        ).tag(config=True)
    ## search XML string for job_state - [QH] = pending, R = running, [CE] = done
    state_pending_re = traitlets.Unicode(r'<job_state>[QH]</job_state>'
        ).tag(config=True)
    state_running_re = traitlets.Unicode(r'<job_state>R</job_state>'
        ).tag(config=True)
    state_exechost_re = traitlets.Unicode(
            r'<exec_host>((?:[\w_-]+\.?)+)/\d+'
        ).tag(config=True)

    def parse_job_id(self, output):
        """
        Parsing output of submit command to get job id.
        :param str output: Output of submit command, scheduler-specific.
        :return: Job id in queue.
        :rtype: str
        """
        try:
            int(output)  ## make sure jobid is really a number...
            return output  ## ... but string needs to be returned
        except ValueError as e:
            self.log.error('Spawner unable to parse job ID from text: {}'
                           .format(output))
            raise e


class MoabSpawner(TorqueSpawner):

    job_id = traitlets.Unicode('$MOAB_JOBID'
        ).tag(config=True)
    ## outputs job id string
    batch_submit_cmd = traitlets.Unicode('msub'
        ).tag(config=True)
    ## outputs job data XML string
    batch_query_cmd = traitlets.Unicode('mdiag').tag(config=True)
    opt_query_state = traitlets.Unicode(' -j {job_id} --xml'
        ).tag(config=True)
    opt_query_user = traitlets.Unicode('-j -w user={username} | wc -l'
        ).tag(config=True)
    batch_cancel_cmd = traitlets.Unicode('mjobctl -c {job_id}'
        ).tag(config=True)
    state_pending_re = traitlets.Unicode(r'State="Idle"'
        ).tag(config=True)
    state_running_re = traitlets.Unicode(r'State="Running"'
        ).tag(config=True)
    state_exechost_re = traitlets.Unicode(
            r'AllocNodeList="([^\r\n\t\f :"]*)'
        ).tag(config=True)

    def parse_job_id(self, output):
        """
        Parsing output of submit command to get job id.
        :param str output: Output of submit command, scheduler-specific.
        :return: Job id in queue.
        :rtype: str
        """
        try:
            int(output)  ## make sure jobid is really a number...
            return output  ## ... but string needs to be returned
        except ValueError as e:
            self.log.error('Spawner unable to parse job ID from text: {}'
                           .format(output))
            raise e


class PBSSpawner(TorqueSpawner):

    batch_script = traitlets.Unicode(
'''#!/bin/sh
{% if queue or host %}#PBS -q {% if queue  %}{{queue}}{% endif %}\
{% if host %}@{{host}}{% endif %}{% endif %}
#PBS -l walltime={{runtime}}
#PBS -l select={{nodes}}:ncpus={{nprocs}}:mem={{memory}}
#PBS -N jupyterhub-singleuser
#PBS -o {{homedir}}/{{simu_output_log}}
#PBS -e {{homedir}}/{{simu_error_log}}
#PBS -v {{keepvars}}
{% if options %}#PBS {{options}}{% endif %}

{{prologue}}
{{epilogue}}
''').tag(config=True)

    ## outputs job data XML string
    batch_query_cmd = traitlets.Unicode('qstat').tag(config=True)
    opt_query_state = traitlets.Unicode('-fx {job_id}').tag(config=True)
    state_pending_re = traitlets.Unicode(r'job_state = [QH]'
        ).tag(config=True)
    state_running_re = traitlets.Unicode(r'job_state = R'
        ).tag(config=True)
    state_exechost_re = traitlets.Unicode(r'exec_host = ([\w_-]+)/'
        ).tag(config=True)


class UserEnvMixin:
    """
    Mixin class that computes values for USER, SHELL and HOME in the
    environment passed to the job submission subprocess in case the batch
    system needs these for the batch script.
    """

    def user_env(self, env):
        """
        Augmenting environment of spawned process with user specific env
        variables.
        :param env: Environment of spawned process.
        :return: Augmented environment.
        """
        env['USER'] = self.user.name
        home = pwd.getpwnam(self.user.name).pw_dir
        shell = pwd.getpwnam(self.user.name).pw_shell
        if home:
            env['HOME'] = home
        if shell:
            env['SHELL'] = shell
        return env

    def get_env(self):
        """
        Getting user environment variables to be passed to the user's job.
        Everything here should be passed to the user's job as
        environment.  Caution: If these variables are used for
        authentication to the batch system commands as an admin, be
        aware that the user will receive access to these as well.
        """
        return self.user_env(super().get_env())


class SlurmSpawner(UserEnvMixin,BatchSpawnerRegexStates):

    batch_script = traitlets.Unicode(
'''#!/bin/bash
#SBATCH --output={{homedir}}/{{simu_output_log}}
#SBATCH --job-name=spawner-jupyterhub
#SBATCH --workdir={{homedir}}
#SBATCH --export={{keepvars}}
#SBATCH --get-user-env=L
{% if partition  %}#SBATCH --partition={{partition}}
{% endif %}{% if runtime    %}#SBATCH --time={{runtime}}
{% endif %}{% if memory     %}#SBATCH --mem={{memory}}
{% endif %}{% if nprocs     %}#SBATCH --cpus-per-task={{nprocs}}
{% endif %}{% if reservation%}#SBATCH --reservation={{reservation}}
{% endif %}{% if options    %}#SBATCH {{options}}{% endif %}

trap 'echo SIGTERM received' TERM
{{prologue}}
which jupyterhub-singleuser
{% if srun %}{{srun}} {% endif %}
echo 'jupyterhub-singleuser ended gracefully'
{{epilogue}}
''').tag(config=True)
    job_id = traitlets.Unicode('$SLURM_JOB_ID'
        ).tag(config=True)
    ## all these req_foo traits will be available as substvars for
    ## templated strings
    req_cluster = traitlets.Unicode('',
            help='cluster name to submit job to resource manager'
        ).tag(config=True)
    req_qos = traitlets.Unicode('',
            help='QoS name to submit job to resource manager'
        ).tag(config=True)
    req_srun = traitlets.Unicode('srun',
            help='''set req_srun='' to disable running in job step, and
                    note that this affects environment handling; this is
                    effectively a prefix for the singleuser command'''
        ).tag(config=True)
    req_reservation = traitlets.Unicode('',
            help='reservation name to submit to resource manager'
        ).tag(config=True)
    ## outputs line like 'Submitted batch job 209'
    batch_submit_cmd = traitlets.Unicode('sbatch --parsable'
        ).tag(config=True)
    ## outputs status and exec node like 'RUNNING hostname'
    batch_query_cmd = traitlets.Unicode('squeue').tag(config=True)
    opt_query_state = traitlets.Unicode("-h -j {job_id} -o '%T %B'"
        ).tag(config=True)
    opt_query_cpus = traitlets.Unicode('-h -o "%C" -j {} -t RUNNING'
        ).tag(config=True)
    opt_query_cpus_name = traitlets.Unicode(
        '-h -o "%j %C" -j {} -t RUNNING').tag(config=True)
    opt_query_time_name = traitlets.Unicode(
        '-h -o "%j %L" -j {} -t RUNNING').tag(config=True)
    opt_query_user = traitlets.Unicode('-h --user={username} | wc -l',
        ).tag(config=True)
    batch_cancel_cmd = traitlets.Unicode('scancel {job_id}'
        ).tag(config=True)
    ## use long-form states: PENDING, CONFIGURING = pending
    ## RUNNING, COMPLETING = running
    state_pending_re = traitlets.Unicode(r'^(?:PENDING|CONFIGURING)'
        ).tag(config=True)
    state_running_re = traitlets.Unicode(r'^(?:RUNNING|COMPLETING)'
        ).tag(config=True)
    state_exechost_re = traitlets.Unicode(r'\s+((?:[\w_-]+\.?)+)$'
        ).tag(config=True)

    def parse_job_id(self, output):
        """
        Parsing output of submit command to get job id.
        :param str output: Output of submit command, scheduler-specific.
        :return: Job id in queue.
        :rtype: str
        """
        try:
            ## use only last line to circumvent slurm bug
            job_id = output.splitlines()[-1].split(';')[0]
            int(job_id)  ## make sure jobid is really a number...
            return job_id  ## ... but string needs to be returned
        except (ValueError, IndexError) as e:
            self.log.error('Spawner unable to parse job ID from text: {}'
                           .format(output))
            raise e

    def getTotalCPUCount(self, job_ids):
        """
        Getting the number of CPUs allocated to specified jobs.
        :param list[str] job_ids: One or more job ID.
        :return: Total CPU count.
        :rtype: int
        """
        out = self.queryScheduler(
            self.opt_query_cpus.format(','.join(job_ids)))
        return sum([int(x) for x in list(out.splitlines())])

    def getCPUCountByJob(self, job_ids):
        """
        Getting the number of CPUs allocated to specified jobs along with
        job names.
        :param list[str] job_ids: One or more job ID.
        :return: Job names and respective CPU count.
        :rtype: dict[str, str]
        """
        out = self.queryScheduler(
            self.opt_query_cpus_name.format(','.join(job_ids)))
        return dict(map(lambda x: tuple(x.split(' ')), out.splitlines()))

    def getRemainingJobsTime(self, job_ids):
        """
        Getting time left for the job to execute in
        days-hours:minutes:seconds, along with job names.
        :param list[str] job_ids: One or more job ID.
        :return: Job names and respective time left.
        :rtype: dict[str, str]
        """
        out = self.queryScheduler(
            self.opt_query_time_name.format(','.join(job_ids)))
        return dict(map(lambda x: tuple(x.split(' ')), out.splitlines()))


class MultiSlurmSpawner(SlurmSpawner):
    """
    When Slurm has been compiled with --enable-multiple-slurmd, the
    administrator sets the name of the slurmd instance via the slurmd -N
    option. This node name is usually different from the hostname and may
    not be resolvable by JupyterHub. Here we enable the administrator to
    map the node names onto the real hostnames via a traitlet.
    """

    daemon_resolver = traitlets.Dict({},
            help='map node names to hostnames'
        ).tag(config=True)

    def state_gethost(self):
        """
        Returning hostname or addr of running job, by parsing
        self.job_status.
        :return: Hostname or addr of running job.
        :rtype: str
        """
        host = SlurmSpawner.state_gethost(self)
        return self.daemon_resolver.get(host, host)


class GridengineSpawner(BatchSpawnerBase):

    batch_script = traitlets.Unicode('''#!/bin/bash
#$ -j yes
#$ -N spawner-jupyterhub
#$ -o {homedir}/{simu_output_log}
#$ -e {homedir}/{simu_error_log}
#$ -v {keepvars}
#$ {options}

{prologue}
{epilogue}
''').tag(config=True)
    job_id = traitlets.Unicode('$JOB_ID'
        ).tag(config=True)
    ## outputs something like: "Your job 1 ("job.sh") has been submitted"
    batch_submit_cmd = traitlets.Unicode('qsub'
        ).tag(config=True)
    ## outputs job data XML string
    batch_query_cmd = traitlets.Unicode('qstat').tag(config=True)
    opt_query_state = traitlets.Unicode('-xml').tag(config=True)
    opt_query_user = traitlets.Unicode('-u {username} | wc -l'
        ).tag(config=True)
    batch_cancel_cmd = traitlets.Unicode('qdel {job_id}'
        ).tag(config=True)

    def parse_job_id(self, output):
        """
        Parsing output of submit command to get job id.
        :param str output: Output of submit command, scheduler-specific.
        :return: Job id in queue.
        :rtype: str
        """
        try:
            job_id = output.split(' ')[2]
            int(job_id)  ## make sure jobid is really a number...
            return job_id  ## ... but string needs to be returned
        except (ValueError, IndexError) as e:
            self.log.error('Spawner unable to parse job ID from text: {}'
                           .format(output))
            raise e

    def state_ispending(self):
        """
        Querying for "pending" status.
        :return: True for pending, False otherwise.
        :rtype: bool
        """
        if self.job_status:
            job_info = (xml.etree.ElementTree
                                 .fromstring(self.job_status)
                                 .find(".//job_list[JB_job_number='{0}']"
                                       .format(self.job_id)))
            if job_info:
                return job_info.attrib.get('state') == 'pending'
        return False

    def state_isrunning(self):
        """
        Querying for "running" status.
        :return: True for running, False otherwise.
        :rtype: bool
        """
        if self.job_status:
            job_info = (xml.etree.ElementTree
                                 .fromstring(self.job_status)
                                 .find(".//job_list[JB_job_number='{0}']"
                                    .format(self.job_id)))
            if job_info:
                return job_info.attrib.get('state') == 'running'
        return False

    def state_gethost(self):
        """
        Returning hostname or addr of running job, by parsing
        self.job_status.
        :return: Hostname or addr of running job.
        :rtype: str
        """
        if self.job_status:
            queue_name = (xml.etree.ElementTree
                        .fromstring(self.job_status)
                        .find(".//job_list[JB_job_number='{0}']/queue_name"
                        .format(self.job_id)))
            if queue_name and queue_name.text:
                return queue_name.text.split('@')[1]
        self.log.error(
            'spawner unable to match host addr in job {0} with status {1}'
            .format(self.job_id, self.job_status))


class CondorSpawner(UserEnvMixin,BatchSpawnerRegexStates):

    batch_script = traitlets.Unicode(
'''
Executable = /bin/sh
RequestMemory = {memory}
RequestCpus = {nprocs}
Arguments = \'-c 'exec '\'
Remote_Initialdir = {homedir}
Output = {homedir}/{simu_output_log}
Error = {homedir}/{simu_error_log}
ShouldTransferFiles = False
GetEnv = True
{options}
Queue
''').tag(config=True)
    job_id = traitlets.Unicode('$CONDOR_CLUSTER.$CONDOR_PROCESS'
        ).tag(config=True)
    ## outputs job id string
    batch_submit_cmd = traitlets.Unicode('condor_submit'
        ).tag(config=True)
    ## outputs job data XML string
    batch_query_cmd = traitlets.Unicode('condor_q').tag(config=True)
    opt_query_state = traitlets.Unicode(
            '''{job_id} -format "%s, " JobStatus -format "%s"
               RemoteHost -format "\n" True''')
    opt_query_user = traitlets.Unicode('-submitter {username} | wc -l'
        ).tag(config=True)
    batch_cancel_cmd = traitlets.Unicode('condor_rm {job_id}'
        ).tag(config=True)
    ## job status: 1 = pending, 2 = running
    state_pending_re = traitlets.Unicode(r'^1,'
        ).tag(config=True)
    state_running_re = traitlets.Unicode(r'^2,'
        ).tag(config=True)
    state_exechost_re = traitlets.Unicode(r'^\w*, .*@([^ ]*)'
        ).tag(config=True)

    def parse_job_id(self, output):
        """
        Parsing output of submit command to get job id.
        :param str output: Output of submit command, scheduler-specific.
        :return: Job id in queue.
        :rtype: str
        """
        try:
            job_id = (re.search(r'.*submitted to cluster ([0-9]+)', output)
                        .groups()[0])
            int(job_id)  ## make sure jobid is really a number...
            return job_id  ## ... but string needs to be returned
        except (ValueError, IndexError) as e:
            self.log.error('Spawner unable to parse job ID from text: {}'
                           .format(output))
            raise e


class LsfSpawner(BatchSpawnerBase):
    """
    A Spawner that uses IBM's Platform Load Sharing Facility (LSF) to
    launch notebooks.
    """

    batch_script = traitlets.Unicode(
'''#!/bin/sh
#BSUB -R 'select[type==any]'    # Allow spawning on non-uniform hardware
#BSUB -R 'span[hosts=1]'        # Only spawn job on one server
#BSUB -q {queue}
#BSUB -J spawner-jupyterhub
#BSUB -o {homedir}/{simu_output_log}
#BSUB -e {homedir}/{simu_error_log}

{prologue}
{epilogue}
''').tag(config=True)
    job_id = traitlets.Unicode('$LSB_JOBID'
        ).tag(config=True)
    batch_submit_cmd = traitlets.Unicode('bsub'
        ).tag(config=True)
    batch_query_cmd = traitlets.Unicode('bjobs').tag(config=True)
    opt_query_state = traitlets.Unicode(
        '-a -noheader -o "STAT EXEC_HOST" {job_id}').tag(config=True)
    opt_query_user = traitlets.Unicode('-noheader -u {username} | wc -l'
        ).tag(config=True)
    batch_cancel_cmd = traitlets.Unicode('bkill {job_id}'
        ).tag(config=True)

    def get_env(self):
        env = super().get_env()
        ## LSF relies on environment variables to launch local jobs.
        ## ensure that these values are included in the environment used
        ## to run the spawner
        for key in ['LSF_ENVDIR', 'LSF_SERVERDIR', 'LSF_FULL_VERSION',
                    'LSF_LIBDIR', 'LSF_BINDIR']:
            if key in os.environ and key not in env:
                env[key] = os.environ[key]
        return env

    def parse_job_id(self, output):
        """
        Parsing output of submit command to get job id.
        Assuming output in the following form:
        'Job <1815> is submitted to default queue <normal>.'
        :param str output: Output of submit command, scheduler-specific.
        :return: Job id in queue.
        :rtype: str
        """
        try:
            job_id = output.split(' ')[1].strip('<>')
            int(job_id)  ## make sure jobid is really a number...
            return job_id  ## ... but string needs to be returned
        except (ValueError, IndexError) as e:
            self.log.error('Spawner unable to parse job ID from text: {}'
                           .format(output))
            raise e

    def state_ispending(self):
        """
        Querying for "pending" status.
        :return: True for pending, False otherwise.
        :rtype: bool
        """
        ## parse results of batch_query_cmd
        ## output determined by results of self.batch_query_cmd
        if self.job_status:
            return (self.job_status.split(' ')[0].upper()
                    in {'PEND', 'PUSP'})

    def state_isrunning(self):
        """
        Querying for "running" status.
        :return: True for running, False otherwise.
        :rtype: bool
        """
        if self.job_status:
            return self.job_status.split(' ')[0].upper() == 'RUN'

    def state_gethost(self):
        """
        Returning hostname or addr of running job, by parsing
        self.job_status.
        :return: Hostname or addr of running job.
        :rtype: str
        """
        if self.job_status:
            return self.job_status.split(' ')[1].strip()
        self.log.error(
            'spawner unable to match host addr in job {0} with status {1}'
            .format(self.job_id, self.job_status))

# vim: set ai expandtab softtabstop=4:
