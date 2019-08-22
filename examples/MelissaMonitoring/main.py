from threading import Thread
from collections import Counter, OrderedDict
from typing import Dict, List
import subprocess
import time, datetime
import statistics

from IPython.display import display
import ipywidgets as widgets
import matplotlib

from launcher.study import Study
from launcher.study import server as MelissaServer

        
class MelissaMonitoring:

    def __init__(self, study_options, melissa_stats, user_functions):
        self.study = Study(study_options, melissa_stats, user_functions)
        self.jobStates = {-1:'Not submitted', 0:'Waiting', 1:'Running', 2:'Finished', 4:'Timeout'}
        self.timeStart = None
        self.timeStop = None
        self.thread = None
        self.state_checker = None
        self.jobRestartThreshold = 3

        self.__coreUsageData = None
        self._timeWidget = None
        self._serverStatusWidget = None
        self._failedParametersWidget = None

    def startStudyInThread(self) -> Thread:
        """Starts study with options from the constructor
        
        Returns:
            Thread -- Thread object used to control the study
        """

        self.__coreUsageData = OrderedDict()
        self.thread = Thread(target=self.study.run)
        self.thread.start()

        # wait for the state checker thread to initialize
        while self.study.threads.get('state_checker', None) is None:
            time.sleep(0.001)
        else:
            self.state_checker = self.study.threads['state_checker']
        
        while not self.state_checker.is_alive():
            time.sleep(0.001)

        self.timeStart = datetime.datetime.now()

        return self.thread

    def isStudyRunning(self) -> bool:
        """Checks if study is still running
        
        Returns:
            Bool -- Is study still running?
        """

        return self.state_checker.running_study if self.state_checker.is_alive() else False

    def getJobStatusData(self) -> Dict[str, int]:
        """Get dictionary with current number of jobs with particular job status
        
        Returns:
            Dictionary -- Mapped as jobStatus -> numberOfJobs
        """

        data = dict(Counter(map(lambda x: x.job_status, self.study.groups)))
        return {self.jobStates[statusCode]: value for statusCode, value in data.items()}

    def getServerStatusData(self) -> str:
        """Get server job status

        Returns:
            string -- Server job status
        """

        return self.jobStates[MelissaServer.status]

    def getCPUCount(self) -> int:
        """Get the number of user's current total CPU usage. Slurm specific

        Returns:
            int -- number of CPU's in usage
        """

        ids = self.getJobsIDs()
        process = subprocess.Popen(f'squeue -h -o "%C" -j {",".join(ids)} -t RUNNING',
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)

        out, _ = process.communicate()
        return sum([int(x) for x in list(out.splitlines())])

    def getJobCPUCount(self, ID:str) -> str:
        """Get CPU usage of particular job. Slurm specific

        Arguments:
            ID {str} -- id of the job

        Returns:
            str -- CPU usage of the job
        """

        process = subprocess.Popen(f'squeue -h -o "%C" -j {ID} -t RUNNING',
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)
        out, _ = process.communicate()
        return str(out).strip()

    def getJobsCPUCount(self) -> Dict[str,str]:
        """Get the current CPU usage of your jobs. Slurm specific

        Returns:
            Dict[str,str] -- Mapped as name_of_the_job -> CPU_usage
        """

        ids = self.getJobsIDs()
        process = subprocess.Popen(f'squeue -h -o "%j %C" -j {",".join(ids)} -t RUNNING',
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)

        out, _ = process.communicate()
        return dict(map(lambda x: tuple(x.split(' ')), out.splitlines()))

    def getRemainingJobsTime(self) -> Dict[str, str]:
        """Get the current remaining time of your jobs. Slurm specific
        
        Returns:
            Dict[str,str] -- Mapped as name_of_the_job -> remaining_time
        """

        ids = self.getJobsIDs()
        process = subprocess.Popen(f'squeue -h -o "%j %L" -j {",".join(ids)} -t RUNNING',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)

        out, _ = process.communicate()
        return dict(map(lambda x: tuple(x.split(' ')), out.splitlines()))

    def getJobsIDs(self, include_server:bool = True) -> List[str]:
        """Get the list of jobs ids'

        Keyword Arguments:
            include_server {bool} -- Include server ID in the list (default: {True})

        Returns:
            List[str] -- List of jobs ids'
        """

        data = list(map(lambda x: str(x.job_id), self.study.groups))
        if include_server:
            data.append(str(MelissaServer.job_id))
        return data

    def getServerID(self) -> str:
        """Get server ID

        Returns:
            str -- server ID
        """

        return str(MelissaServer.job_id)

    def getFailedParametersList(self) -> List:
        """Get list of failed parameters in the study
        
        Returns:
            list -- nested list of failed parameters
        """

        data = filter(lambda x: x.nb_restarts > self.jobRestartThreshold ,self.study.groups)
        return list(map(lambda x: x.param_set, data))

    def plotCoresUsage(self, ax: matplotlib.axes) -> None:
        """Automatically plot cores usage as time series
        
        Arguments:
            ax {matplotlib.axes} -- Axes object that should be plotted
        """

        ax.clear()
        self.__coreUsageData[datetime.datetime.now() - self.timeStart] = self.getCPUCount()
        ax.plot(list(map(lambda x: str(x), self.__coreUsageData.keys())), list(self.__coreUsageData.values()))
        ax.set_title('Cores usage vs time')
        ax.get_figure().autofmt_xdate()

    def plotJobStatus(self, ax: matplotlib.axes) -> None:
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

    def showRemainingJobsTime(self) -> None:
        """Show remaining time of your jobs on cluster 
        """

        if self._timeWidget is None:
            style = {'description_width': 'initial'}
            self._timeWidget = widgets.HTML(value="",
                                            description='Remaining Job Time: ',
                                            style=style,
                                            )
            display(self.__timeWidget)

        data = self.getRemainingJobsTime()
        data = [f'{k} - {v}' for k,v in data.items()]
        value = '<br/>'.join(data)
        self._timeWidget.value = value

    def showServerStatus(self) -> None:
        """Show the status of the Melissa server
        """

        if self._serverStatusWidget is None:
            style = {'description_width': 'initial'}
            self._serverStatusWidget = widgets.HTML(value="",
                                            description='Server status: ',
                                            style=style,
                                            )
            display(self._serverStatusWidget)


        self._serverStatusWidget.value = self.getServerStatusData()

    def showFailedParameters(self) -> None:

        if self._failedParametersWidget is None:
            style = {'description_width': 'initial'}
            self._failedParametersWidget = widgets.HTML(value="",
                                            description='Failed Parameters: ',
                                            style=style,
                                            )
            display(self._failedParametersWidget)

        data = self.getFailedParametersList()
        value = '<br/>'.join(map(lambda x: str(x), data))
        self._failedParametersWidget.value = value

    def cleanUp(self) -> None:
        """Clean up after study
        """

        self.thread.join()
        self.timeStop = datetime.datetime.now()
        self.thread = None
        self.state_checker = None
        if self._timeWidget is not None:
            self._timeWidget.close()
            self._timeWidget = None
        if self._serverStatusWidget is not None:
            self._serverStatusWidget.close()
            self._serverStatusWidget = None
        if self._failedParametersWidget is not None:
            self._failedParametersWidget.close()
            self._failedParametersWidget = None

    def getStudyInfo(self) -> str:
        """Get info about performed study such as time and cores used
        
        Returns:
            str -- info about study 
        """

        info = f"""
        Study started: {self.timeStart}
        Study ended: {self.timeStop}
        Elapsed time: {self.timeStop - self.timeStart}
        Max cores used: {max(list(self.__coreUsageData.values()))}
        Avg cores used: {statistics.mean(list(self.__coreUsageData.values()))}
        """
        return info

    
