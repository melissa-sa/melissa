from abc import ABC, abstractmethod
from datetime import datetime

import ipywidgets as widgets
import plotly.graph_objs as go

class DashPlot(ABC):

    @abstractmethod
    def __init__(self):
        super().__init__()
        self.widget = go.FigureWidget()
        self.dataFunction = None
        self.name = None

    @abstractmethod
    def plot(self):
        raise NotImplementedError
        

class JobStatusesDashPlot(DashPlot):

    def __init__(self, melissaMonitoring):
        super().__init__()
        self.name = 'Job statuses'
        self.dataFunction = melissaMonitoring.getJobStatusData

        self.widget.add_pie()
        self.widget.data[0].textinfo = 'label+percent'
        self.widget.data[0].hoverinfo = 'label+percent+value'
        self.widget.layout.title.text = 'Jobs statuses'

    def plot(self):
        self.data = self.dataFunction()
        self.widget.data[0].labels = list(self.data.keys())
        self.widget.data[0].values = list(self.data.values())

class CoreUsageDashPlot(DashPlot):

    def __init__(self, melissaMonitoring):
        super().__init__()
        self.name = 'Cores usage'
        self.dataFunction = melissaMonitoring.getCPUCount
        self.data = []
        self.time = []

        self.widget.add_scatter(name="Cores used")
        self.widget.layout.autosize = True
        self.widget.layout.showlegend = True
        self.widget.layout.xaxis.rangeslider.visible = True
        self.widget.layout.title.text = 'Core usage vs time'
        self.widget.layout.hovermode = 'x'

    def plot(self):
        self.data.append(self.dataFunction())
        self.time.append(datetime.now())
        self.widget.data[0].x = self.time
        self.widget.data[0].y = self.data

class SobolConfidenceIntervalDashPlot(DashPlot):

    def __init__(self, melissaMonitoring):
        super().__init__()
        self.name = 'Sobol confidence interval'
        self.dataFunction = melissaMonitoring.getSobolConfidenceInterval
        self.data = {}

        self.widget.add_scatter(name="Confidence interval")
        self.widget.layout.autosize = True
        self.widget.layout.showlegend = True
        self.widget.layout.xaxis.rangeslider.visible = True
        self.widget.layout.title.text = 'Sobol confidence interval'
        self.widget.layout.hovermode = 'x'

    def plot(self):
        interval = self.dataFunction()
        if interval is not None:
            self.data[datetime.now()] = interval

        self.widget.data[0].x = list(self.data.keys())
        self.widget.data[0].y = list(self.data.values())