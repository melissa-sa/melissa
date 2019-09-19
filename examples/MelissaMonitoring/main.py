from threading import Thread
from collections import Counter, OrderedDict
import subprocess
import time, datetime
import statistics

from IPython.display import display
import ipywidgets as widgets
import matplotlib

from launcher.study import Study

        
class MelissaMonitoring:

    def __init__(self, study_options, melissa_stats, user_functions):
        self.study = Study(study_options, melissa_stats, user_functions)
        self.jobStates = {-1:'Not submitted', 0:'Waiting', 1:'Running', 2:'Finished', 4:'Timeout'}
        self.timeStart = None
        self.timeStop = None
        self.thread = None
        self.state_checker = None
        self.jobRestartThreshold = 3

        self.coreUsageData = None
        self.sobolConfidenceInterval = None

        self.timeWidget = None
        self.serverStatusWidget = None
        self.failedParametersWidget = None
        self.jobsCPUCountWidget = None

    def startStudyInThread(self):
        """Starts study with options from the constructor
        
        Returns:
            Thread -- Thread object used to control the study
        """

        self.coreUsageData = OrderedDict()
        self.thread = Thread(target=self.study.run)
        self.thread.start()

        self.timeStart = datetime.datetime.now()

        return self.thread

    def waitForInitialization(self):
        """Waits for melissa server to fully initialize
        """

        while self.study.threads.get('state_checker', None) is None:
            time.sleep(0.001)
        else:
            self.state_checker = self.study.threads['state_checker']
        
        while not self.state_checker.is_alive():
            time.sleep(0.001)

    def isStudyRunning(self):
        """Checks if study is still running
        
        Returns:
            Bool -- Is study still running?
        """

        return self.state_checker.running_study if self.state_checker.is_alive() else False

    def getJobStatusData(self):
        """Get dictionary with current number of jobs with particular job status
        
        Returns:
            Dictionary -- Mapped as jobStatus -> numberOfJobs
        """

        data = dict(Counter(map(lambda x: x.job_status, self.study.groups)))
        return {self.jobStates[statusCode]: value for statusCode, value in data.items()}

    def getServerStatusData(self):
        """Get server job status

        Returns:
            string -- Server job status
        """

        return self.jobStates[self.study.server_obj[0].status]

    def getCPUCount(self):
        """Get the number of user's current total CPU usage. Slurm specific

        Returns:
            int -- number of CPU's in usage
        """

        ids = self.getJobsIDs()
        process = subprocess.Popen('squeue -h -o "%C" -j {} -t RUNNING'.format(",".join(ids)),
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)

        out, _ = process.communicate()
        return sum([int(x) for x in list(out.splitlines())])

    def getJobCPUCount(self, ID):
        """Get CPU usage of particular job. Slurm specific

        Arguments:
            ID {str} -- id of the job

        Returns:
            str -- CPU usage of the job
        """

        process = subprocess.Popen('squeue -h -o "%C" -j {} -t RUNNING'.format(ID),
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)
        out, _ = process.communicate()
        return str(out).strip()

    def getJobsCPUCount(self):
        """Get the current CPU usage of your jobs. Slurm specific

        Returns:
            Dict[str,str] -- Mapped as name_of_the_job -> CPU_usage
        """

        ids = self.getJobsIDs()
        process = subprocess.Popen('squeue -h -o "%j %C" -j {} -t RUNNING'.format(",".join(ids)),
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)

        out, _ = process.communicate()
        return dict(map(lambda x: tuple(x.split(' ')), out.splitlines()))

    def getRemainingJobsTime(self):
        """Get the current remaining time of your jobs. Slurm specific

        Returns:
            Dict[str,str] -- Mapped as name_of_the_job -> remaining_time
        """

        ids = self.getJobsIDs()
        process = subprocess.Popen('squeue -h -o "%j %L" -j {} -t RUNNING'.format(",".join(ids)),
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)

        out, _ = process.communicate()
        return dict(map(lambda x: tuple(x.split(' ')), out.splitlines()))

    def getJobsIDs(self, include_server = True):
        """Get the list of jobs ids'

        Keyword Arguments:
            include_server {bool} -- Include server ID in the list (default: {True})

        Returns:
            List[str] -- List of jobs ids'
        """

        data = list(map(lambda x: str(x.job_id), self.study.groups))
        if include_server:
            data.append(str(self.study.server_obj[0].job_id))
        return data

    def getServerID(self):
        """Get server ID

        Returns:
            str -- server ID
        """

        return str(self.study.server_obj[0].job_id)

    def getFailedParametersList(self):
        """Get list of failed parameters in the study

        Returns:
            list -- nested list of failed parameters
        """

        data = filter(lambda x: x.nb_restarts > self.jobRestartThreshold ,self.study.groups)
        return list(map(lambda x: x.param_set, data))


    def getSobolConfidenceInterval(self):
        """Get current sobol confidence interval
        If Sobol Indicies wasn't selected in the options file, function will always return None.
        
        Returns:
            float -- real number between 1 and 2 
            or
            None -- if confidence interval couldn't be calculated at the moment
        """

        return self.study.threads['messenger'].confidence_interval.get('Sobol', None)

    def plotCoresUsage(self, ax):
        """Automatically plot cores usage as time series

        Arguments:
            ax {matplotlib.axes} -- Axes object that should be plotted
        """

        ax.clear()
        self.coreUsageData[datetime.datetime.now() - self.timeStart] = self.getCPUCount()
        ax.plot(list(map(lambda x: str(x), self.coreUsageData.keys())), list(self.coreUsageData.values()))
        ax.set_title('Cores usage vs time')
        ax.get_figure().autofmt_xdate()

    def plotJobStatus(self, ax):
        """Automatically plot job statuses as pie chart

        Arguments:
            ax {matplotlib.axes} -- Axes object that should be plotted
        """

        jobStatusData = self.getJobStatusData()
        ax.clear()
        sumOfJobs = sum(jobStatusData.values())
        sizes = [x/sumOfJobs*100 for x in jobStatusData.values()]
        labels = [x for x in jobStatusData.keys()]
        ax.pie(sizes, labels=labels, autopct='%1.1f%%', startangle=90)
        ax.set_title('Job statuses')

    def plotSobolConfidenceInterval(self, ax):
        """Automatically plot cores usage as time series.
        If Sobol Indicies wasn't selected in the options file, nothing will be plotted.

        Arguments:
            ax {matplotlib.axes} -- Axes object that should be plotted
        """

        data = self.getSobolConfidenceInterval()
        if data is not None:
            ax.clear()
            self.sobolConfidenceInterval[datetime.datetime.now() - self.timeStart] = data
            ax.plot(list(map(lambda x: str(x), self.sobolConfidenceInterval.keys())), list(self.sobolConfidenceInterval.values()))
            ax.set_title('Sobol confidence interval')
            ax.get_figure().autofmt_xdate()

    def _createJobsCPUCountWidget(self):
        """Create jobs cpu count widget, used by showJobsCPUCount & MelissaDash

        Returns:
            widgets.HTML -- customed widget for showing Jobs remaining time
        """

        style = {'description_width': 'initial'}
        self.jobsCPUCountWidget = widgets.HTML(value="",
                                        description='Jobs CPU count: ',
                                        style=style,
                                        )
        return self.jobsCPUCountWidget

    def showJobsCPUCount(self):
        """Create widget (if not created) & show jobs cpu count of your jobs on cluster 
        """

        if self.jobsCPUCountWidget is None:
            self._createJobsCPUCountWidget()
            display(self.jobsCPUCountWidget)

        data = self.getJobsCPUCount()
        data = ['{} - {}'.format(k,v) for k,v in data.items()]
        value = '<br/>'.join(data)
        self.jobsCPUCountWidget.value = value


    def _createRemainingJobsTimeWidget(self):
        """Create remaining time widget, used by showRemainingJobsTime & MelissaDash

        Returns:
            widgets.HTML -- customed widget for showing Jobs remaining time
        """

        style = {'description_width': 'initial'}
        self.timeWidget = widgets.HTML(value="",
                                        description='Remaining job time: ',
                                        style=style,
                                        )
        return self.timeWidget

    def showRemainingJobsTime(self):
        """Create widget (if not created) & show remaining time of your jobs on cluster 
        """

        if self.timeWidget is None:
            self._createRemainingJobsTimeWidget()
            display(self.timeWidget)

        data = self.getRemainingJobsTime()
        data = ['{} - {}'.format(k,v) for k,v in data.items()]
        value = '<br/>'.join(data)
        self.timeWidget.value = value

    def _createServerStatusWidget(self):
        """Create server status widget, used by showServerStatus & MelissaDash

        Returns:
            widgets.HTML -- customed widget for showing server status
        """

        style = {'description_width': 'initial'}
        self.serverStatusWidget = widgets.HTML(value="",
                                        description='Server status: ',
                                        style=style,
                                        )
        
        return self.serverStatusWidget

    def showServerStatus(self):
        """Create widget (if not created) & show the status of the Melissa server
        """

        if self.serverStatusWidget is None:
            self._createServerStatusWidget()
            display(self.serverStatusWidget)


        self.serverStatusWidget.value = self.getServerStatusData()

    def _createFailedParametersWidget(self):
        """Create failed parameters widget, used by showServerStatus & MelissaDash

        Returns:
            widgets.HTML -- customed widget for showing failed parameters
        """

        style = {'description_width': 'initial'}
        self.failedParametersWidget = widgets.HTML(value="",
                                            description='Failed parameters: ',
                                            style=style,
                                            )
        return self.failedParametersWidget

    def showFailedParameters(self):
        """Create widget (if not created) & show simulations' failed parameters
        """

        if self.failedParametersWidget is None:
            self._createFailedParametersWidget()
            display(self.failedParametersWidget)

        data = self.getFailedParametersList()
        value = '<br/>'.join(map(lambda x: str(x), data))
        self.failedParametersWidget.value = value

    def cleanUp(self):
        """Clean up after study
        """

        self.thread.join()
        self.timeStop = datetime.datetime.now()
        self.thread = None
        self.state_checker = None
        if self.timeWidget is not None:
            self.timeWidget.close()
            self.timeWidget = None
        if self.serverStatusWidget is not None:
            self.serverStatusWidget.close()
            self.serverStatusWidget = None
        if self.failedParametersWidget is not None:
            self.failedParametersWidget.close()
            self.failedParametersWidget = None
        if self.jobsCPUCountWidget is not None:
            self.jobsCPUCountWidget.close()
            self.jobsCPUCountWidget = None

    def getStudyInfo(self):
        """Get info about performed study such as time and cores used
        
        Returns:
            str -- info about study 
        """

        info = """
        Study started: {}
        Study ended: {}
        Elapsed time: {}
        Max cores used: {}
        Avg cores used: {}
        """.format(self.timeStart, self.timeStop, self.timeStop - self.timeStart,
        max(list(self.coreUsageData.values())), statistics.mean(list(self.coreUsageData.values())))
        return info

    
