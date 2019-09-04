from time import sleep
from threading import Thread

import ipywidgets as widgets
from IPython.display import display

from .main import MelissaMonitoring
from .internal.plots import CoreUsageDashPlot, JobStatusesDashPlot, SobolConfidenceIntervalDashPlot
from .utils import HiddenPrints


class MelissaDash:

    def __init__(self, study_options, melissa_stats, user_functions):
        self.monitoring = MelissaMonitoring(study_options, melissa_stats, user_functions)

        self.__tabbedPlots = [CoreUsageDashPlot(self.monitoring), JobStatusesDashPlot(self.monitoring), SobolConfidenceIntervalDashPlot(self.monitoring)]
        self.__plots = None
        self.__studyStatus = None
        self.__studyInfo = None
        self.__failedParamInfo = None
        self.__footerOutput = None
        self.app = None
        self.__init()

    def __init(self):
        self.__initPlots()
        self.__initStudyInfo()
        self.__initStudyStatus()
        self.__initFooter()
        self.__initApp()

    def __initPlots(self):
        tabNames = [x.name for x in self.__tabbedPlots]
        children = [x.widget for x in self.__tabbedPlots]
        tab = widgets.Tab()
        tab.children = children
        for index, title in enumerate(tabNames):
            tab.set_title(index,title)

        self.__plots = tab

    def __initStudyInfo(self):
        self.__studyInfo = widgets.Tab()
        tabNames = ['Remaining job time', 'CPU job usage', 'Failed parameters']
        children = [self.monitoring._createRemainingJobsTimeWidget(), self.monitoring._createJobsCPUCountWidget(), self.monitoring._createFailedParametersWidget()]
        self.__studyInfo.children = children
        for index, title in enumerate(tabNames):
            self.__studyInfo.set_title(index,title)

    def __initStudyStatus(self):
        header = widgets.HTML("<h2>Study status</h2><hr/>")
        self.__studyStatus = widgets.VBox([header, self.monitoring._createServerStatusWidget()])

    def __initFooter(self):
        self.__footerOutput = widgets.Output()

    def __initApp(self):
        header = widgets.HTML("<h1>Melissa Dashboard</h1><hr/>", layout=widgets.Layout(height='auto'))
        header.style.text_align='center'

        self.app = widgets.AppLayout(header=header,
                                    left_sidebar=None,
                                    center=widgets.VBox([self.__plots, self.__studyInfo]),
                                    right_sidebar=self.__studyStatus,
                                    footer=self.__footerOutput)

    def start(self, refreshRate = 1):
        with HiddenPrints():
            display(self.app)
            self.monitoring.startStudyInThread()
            self.monitoring.showServerStatus()
            self.monitoring.waitForInitialization()
            while self.monitoring.isStudyRunning():
                for plot in self.__tabbedPlots:
                    plot.plot()
                self.monitoring.showServerStatus()
                self.monitoring.showFailedParameters()
                self.monitoring.showRemainingJobsTime()
                self.monitoring.showJobsCPUCount()
                sleep(refreshRate)