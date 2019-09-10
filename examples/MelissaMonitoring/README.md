# Melissa Monitoring

## 1. What is Melissa Monitoring

Melissa monitoring is a python module for running and monitoring progress of the study, preferably in Jupyter Notebook. It can be used in two possible ways. The first way is to use it as an API to a running study. User can manualy gather and plot data as he wish. The second option is to use Melissa Dash, Jupyter Notebook user interface (dashboard) built with ipywidgets and plotly. It plots and shows every possible information about the study via single method call.

## 2. Instalation guide

Requirements:
*  Jupyter Notebook - `pip install --user jupyter`
*  Matplotlib - `pip install ---user matplotlib` (used in manual plotting)
*  Plotly - `pip install --user plotly` (used in Melissa Dash)
*  Ipywidgets - `pip install --user ipywidgets` (used in Melissa Dash and manual plotting)

*Requirements for manual plotting are optional. There are methods to make manual plotting easier which use these libraries. If you use your own plotting libraries or just want the raw data, you don't have to install them.*

For plotly and ipywidgets to work propperly in Jupyer Notebook, you will have to enable their extensions. Add `--sys-prefix` if virtualenv is used.

```bash
jupyter nbextension enable --py widgetsnbextension
jupyter nbextension enable --py plotlywidget
```

You can check if extensions were enabled successfully with `jupyter nbextension list`

```
$ jupyter nbextension list
Known nbextensions:
  config dir: /home/**USER**/.jupyter/nbconfig
    notebook section
      jupyter-js-widgets/extension  enabled 
      - Validating: OK
      plotlywidget/extension  enabled 
      - Validating: OK
```

**!! Be mindful that this instalation guide is ment for Jupyter Notebook. Enabling extensions is different when JupyterLab is used. Please reffer to [ipywidgets installation guide](https://ipywidgets.readthedocs.io/en/latest/user_install.html#installing-the-jupyterlab-extension) and [plotly.py github page](https://github.com/plotly/plotly.py#jupyterlab-support-python-35)  !!**

### 2.1 Running Notebook on the cluster

To run Jupyter Notebook on the cluster please follow [this tutorial](https://oncomputingwell.princeton.edu/2018/05/jupyter-on-the-cluster/). Remember to source **melissa_set_env.sh** and import right python modules **before** running Jupyter Notebook!

## 3. Manual plotting

*The notebook contains examples on manual plotting and Melissa Dash*

To use manual plotting, following steps have to be made:

* Import options dictionaries and MelissaMonitoring from MelissaMonitoring.main package.
  * (Optional) Import matplotlib to create axes to use with existing plotting functions.
  * (Optional) Import HiddenPrints from MelissaMonitoring.utils package to supress printing. It is used to reduce visual bloat.
* Create MelissaMonitoring object using imported dictionaries.
* Run `startStudyInThread()` method followed by `waitForInitialization()`.
* While study is running (via `isStudyRunning()` method) get raw data using `get__()` methods and use them as necessary.
  * (Optional) Use created ealier matplotlib axes in conjunction with `plot__(ax)` methods to automaticly plot particular data.
  * (Optional) Install ipywidgets to use `show__()` methods. These will automaticly show informations as widgets.
* At the end use `cleanUp()` method.

Every method is described by docstrings. If you ever need a remainder, just type `help(MelissaMonitoring)` in Python interpreter.

## 4. Melissa Dashboard

*The notebook contains examples on manual plotting and Melissa Dash*

To use Melissa Dash:
* Import options dictionaries and MelissaDash from MelissaMonitoring.interface
* Create MelissaDash object using imported dictionaries
* Run `start()` method. It takes one argument `refreshRate`, the delay after which Melissa Dash refresh the data.

## 5. Additional improvements (TODOs)

* Make option in Melissa Dash to make plots use WebGL (Web Graphics Library). It will increase the performance and give the ability to plot **MUCH** more data.
* Implement cluster generalization.
* Fix performance issues with batch scheduler querying, optimize querying.****
* Minor Quality Of Life improvements.