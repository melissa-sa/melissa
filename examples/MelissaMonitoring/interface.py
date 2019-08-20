from time import sleep

import ipywidgets as widgets
from IPython.display import display

from .main import MelissaMonitoring
from .internal.plots import CoreUsageDashPlot, JobStatusesDashPlot
from .utils import HiddenPrints


class MelissaDash:

    def __init__(self, study_options, melissa_stats, user_functions):
        self.monitoring = MelissaMonitoring(study_options, melissa_stats, user_functions)
        self.__tabbedPlots = [CoreUsageDashPlot(self.monitoring), JobStatusesDashPlot(self.monitoring)]
        self.__tabsPlot = None
        self.__menu = None
        self.app = None
        self.__initTabs()
        self.__initMenu()
        self.__initApp()

    def __initTabs(self):
        tabNames = [x.name for x in self.__tabbedPlots]
        children = [x.widget for x in self.__tabbedPlots]
        tab = widgets.Tab()
        tab.children = children
        for index, title in enumerate(tabNames):
            tab.set_title(index,title)

        self.__tabsPlot = tab

    def __initMenu(self):
        self.__menu = widgets.ToggleButtons(
            options=['Plots', 'Failed parameters', 'Study information'],
            value = 'Plots',
            description='',
            disabled=False,
            button_style='', # 'success', 'info', 'warning', 'danger' or ''
        )

    def __initApp(self):
        header = widgets.HTML("<h1>Melissa Dashboard</h1><hr/>", layout=widgets.Layout(height='auto'))
        header.style.text_align='center'

        self.app = widgets.AppLayout(header=header,
                                    left_sidebar=self.__menu,
                                    center=self.__tabsPlot,
                                    right_sidebar=None,
                                    footer=None)

    def __displayApp(self):
        display(self.app)
        for plot in self.__tabbedPlots:
            with plot.widget:
                display(plot.figure)

    def start(self, refreshRate:float = 1):
        with HiddenPrints():
            self.__displayApp()
            self.monitoring.startStudyInThread()
            while self.monitoring.isStudyRunning():
                for plot in self.__tabbedPlots:
                    plot.plot()
                sleep(refreshRate)