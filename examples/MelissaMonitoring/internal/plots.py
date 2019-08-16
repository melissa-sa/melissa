from abc import ABC, abstractmethod

import ipywidgets as widgets
import matplotlib.pyplot as plt

class DashPlot(ABC):

    @abstractmethod
    def __init__(self):
        super().__init__()
        self.widget = widgets.Output()
        self.figure = plt.figure()
        self.ax = self.figure.axes[0]
        self.plotFunction = None
        self.name = None
    
    def plot(self):
        with self.widget:
            self.plotFunction(self.ax)
            self.draw()

    def draw(self):
        self.figure.canvas.draw()
        

class JobStatusesDashPlot(DashPlot):

    def __init__(self, melissaMonitoring):
        super().__init__()
        self.name = 'Job statuses'
        self.plotFunction = melissaMonitoring.plotJobStatus

class CoreUsageDashPlot(DashPlot):

    def __init__(self, melissaMonitoring):
        super().__init__()
        self.name = 'Cores usage'
        self.plotFunction = melissaMonitoring.plotCoresUsage