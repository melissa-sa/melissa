from time import sleep

import ipywidgets as widgets
from IPython.display import display

from .main import MelissaMonitoring
from .internal.plots import CoreUsageDashPlot, JobStatusesDashPlot


class MelissaDash:

    def __init__(self, study_options, melissa_stats, user_functions):
        self.monitoring = MelissaMonitoring(study_options, melissa_stats, user_functions)
        self.__tabbedPlots = [CoreUsageDashPlot(self.monitoring), JobStatusesDashPlot(self.monitoring)]
        self.__tabs = None
        self.app = None
        self.__initTabs()
        self.__initApp()
        
    def __initTabs(self):
        tabNames = [x.name for x in self.__tabbedPlots]
        children = [x.widget for x in self.__tabbedPlots]
        tab = widgets.Tab()
        tab.children = children
        for index, title in enumerate(tabNames):
            tab.set_title(index,title)

        self.__tabs = tab

    def __initApp(self):
        header = widgets.HTML("<h1>Melissa Dashboard</h1>", layout=widgets.Layout(height='auto'))
        header.style.text_align='center'

        self.app = widgets.AppLayout(header=header,
                                    left_sidebar=None,
                                    center=self.__tabs,
                                    right_sidebar=None,
                                    footer=None)

    def start(self, refreshRate: float):
        display(self.app)
        self.monitoring.startStudyInThread()
        while self.monitoring.isStudyRunning():
            for plot in self.__tabbedPlots:
                plot.plot()
            sleep(refreshRate)