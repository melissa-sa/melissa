import threading
from collections import Counter

from launcher.study import Study

class MelissaMonitoring(object):

    def __init__(self, study_options, melissa_stats, user_functions):
        self.study = Study(study_options, melissa_stats, user_functions)
        self.jobStates = {-1:'Not submitted', 0:'Waiting', 1:'Running', 2:'Finished', 4:'Timeout'}

    def startStudyInThread(self):
        self.thread = threading.Thread(target=self.study.run)
        self.thread.start()
        return self.thread

    def isStudyRunning(self):
        return self.study.state_checker.running_study

    def getJobStatusData(self):
        data = dict(Counter(map(lambda x: x.job_status, self.study.groups)))
        return {self.jobStates[statusCode]: value for statusCode, value in data.items()}

    def plotJobStatus(self, ax):
        jobStatusData = self.getJobStatusData()
        ax.clear()
        sumOfJobs = sum(jobStatusData.values())
        sizes = [x/sumOfJobs*100 for x in jobStatusData.values()]
        labels = [x for x in jobStatusData.keys()]
        ax.pie(sizes, labels=labels, autopct='%1.1f%%', startangle=90)

    