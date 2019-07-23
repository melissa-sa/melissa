import threading
from collections import Counter, OrderedDict
import os, sys, subprocess
import time, datetime
import statistics

from IPython import display

from launcher.study import Study

class HiddenPrints:
    """Context manager used to suppress prints.
    Used to reduce visual bloat in Jupyter Notebook.
    """
    def __enter__(self):
        self._original_stdout = sys.stdout
        sys.stdout = open(os.devnull, 'w')

    def __exit__(self, exc_type, exc_val, exc_tb):
        sys.stdout.close()
        sys.stdout = self._original_stdout
        
class MelissaMonitoring:

    def __init__(self, study_options, melissa_stats, user_functions):
        self.study = Study(study_options, melissa_stats, user_functions)
        self.jobStates = {-1:'Not submitted', 0:'Waiting', 1:'Running', 2:'Finished', 4:'Timeout'}
        self.timeStart = None
        self.timeStop = None
        self.thread = None
        self.state_checker = None
        self.__coreUsageData = None

    def startStudyInThread(self):
        """Starts study with options from the constructor
        
        Returns:
            Thread -- Thread object used to control the study
        """

        self.__coreUsageData = OrderedDict()
        self.thread = threading.Thread(target=self.study.run)
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
            Server job status
        """

        return self.jobStates[statusCode]: self.study.server_obj[0].job_status

    def getCPUCount(self):
        """Get the number of user's current total CPU usage. Slurm specific
        
        Returns:
            int -- number of CPU's in usage
        """
        process = subprocess.Popen('squeue -h -o "%C" -u ${USER}',
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)

        out, _ = process.communicate()
        return sum([int(x) for x in list(out.splitlines())])

    def getRemainingWalltime(self):
        """Get the current remaining walltime of your jobs. Slurm specific
        
        Returns:
            Dictionary -- Mapped as name_of_the_job -> remaining_walltime
        """
        process = subprocess.Popen('squeue -h -o "%j %L" -u ${USER}',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)

        out, _ = process.communicate()
        return dict(map(lambda x: tuple(x.split(' ')), out.splitlines()))

    def plotCoresUsage(self, ax):
        """Automatically plot cores usage as time series
        
        Arguments:
            ax {matplotlib.axes} -- Axes object that should be plotted
        """

        ax.clear()
        self.__coreUsageData[datetime.datetime.now() - self.timeStart] = self.getCPUCount()
        ax.plot(list(map(lambda x: str(x), self.__coreUsageData.keys())), list(self.__coreUsageData.values()))

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

    def cleanUp(self):
        """Clean up after study
        """

        self.thread.join()
        self.timeStop = datetime.datetime.now()
        self.thread = None
        self.state_checker = None
        display.clear_output(wait=True)

    def getStudyInfo(self):
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
