#!/usr/bin/python
# -*- coding : utf-8 -*-

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from copy import deepcopy
import os, sys, signal
import getpass
cwd = os.getcwd()
sys.path.append(cwd+"/../examples/heat_example")
from master import *

signal.signal(signal.SIGINT, signal.SIG_DFL)

class load_dialog(QDialog):
    def __init__(self, parent = None):
        QWidget.__init__(self, parent)
        self.vbox = QVBoxLayout()
        self.QLineEdit_name = QLineEdit(cwd+"/../examples/heat_example")
        self.vbox.addWidget(self.QLineEdit_name)
        self.hbox_button = QHBoxLayout()
        self.QPushButton_load = QPushButton("Load",self)
        self.QPushButton_cancel = QPushButton("Cancel",self)
        self.hbox_button.addWidget(self.QPushButton_load)
        self.hbox_button.addWidget(self.QPushButton_cancel)
        self.connect(self.QPushButton_load,  SIGNAL("clicked()"), self.close)
        self.connect(self.QPushButton_cancel,  SIGNAL("clicked()"), self.close)
        self.vbox.addItem(self.hbox_button)
        self.setLayout(self.vbox)

class melissa_gui(QWidget):
    def __init__(self, parent = None):
        QWidget.__init__(self,parent)

        self.fbox = QFormLayout()

        self.plop = load_dialog(parent = self)
        self.plop.setModal(True)
        self.plop.exec_()

        self.username       = getpass.getuser()
        self.nb_parameters  = nb_parameters
#        self.nb_parameters  = 0
        self.nb_simu        = nb_simu
        self.nb_groups      = nb_groups
        self.nb_time_steps  = nb_time_steps
        self.operations     = operations
        self.threshold      = threshold
        self.mpi_options    = mpi_options
        self.nb_proc_simu   = nb_proc_simu
        self.nb_proc_server = nb_proc_server
        self.server_path    = server_path
#        self.server_path    = "../server"
        self.range_min      = []
        self.range_max      = []
#        self.range_min      = range_min
#        self.range_max      = range_max
        self.coupling       = coupling
        self.pyzmq          = pyzmq

        self.QCheckBox_sobol = QCheckBox("Sobol Indices")

        self.mean_op = True
        self.variance_op = True
        self.min_max_op = False
        self.threshold_op = False
        self.sobol_op = False
        if operations != []:
            if (not "mean" in operations):
                self.mean_op = False
            if (not "variance" in operations):
                self.variance_op = False
            if ("min" in operations or "max" in operations):
                self.min_max_op = True
            if ("threshold" in operations):
                self.threshold_op = True
            if ("sobol" in operations):
                self.sobol_op = True

        # Username #
        self.label_username = QLabel("User Name")
        self.QLineEdit_username = QLineEdit(self.username)
        self.QLineEdit_username.setMaxLength(128);
        self.QLineEdit_username.editingFinished.connect(self.change_username)
        self.fbox.addRow(self.label_username,self.QLineEdit_username)

        # batch scheduller #
        self.label_sched = QLabel("Batch Scheduller")
        self.QComboBo_sched = QComboBox()
        self.QComboBo_sched.addItems(["Slurm", "CCC", "OAR"])
        self.fbox.addRow(self.label_sched,self.QComboBo_sched)

        # number of parameters #
        self.dialog = None
        self.label_param = QLabel("Parameters")
        self.new_param_name = "NewParam"
        self.new_param_range_min = 0.0
        self.new_param_range_max = 1.0
        self.item_list = []
        self.parameter_list = []
        self.QListWidget_param = QListWidget()
#        self.QListWidget_param.itemDoubleClicked.connect(self.print_plop)
        self.QListWidget_param.itemDoubleClicked.connect(self.show_modif_param_dialog)
        if nb_parameters > 0:
            for i in range(nb_parameters):
                self.add_parameter()
        else:
            self.add_parameter()
        self.fbox.addRow(self.label_param,self.QListWidget_param)
        self.QPushButton_add = QPushButton("Add Parameter")
        self.connect(self.QPushButton_add,  SIGNAL("clicked()"), self.show_add_param_dialog)
        self.connect(self.QPushButton_add,  SIGNAL("clicked()"), self.check_nb_param)
        self.fbox.addRow(QLabel(""),self.QPushButton_add)

        # number of groups #
        self.label_groups = QLabel("Number of Simulation Groups")
        self.QSpinBox_nb_groups = QSpinBox()
        self.QSpinBox_nb_groups.setRange(2,60000)
        self.QSpinBox_nb_groups.setValue(self.nb_groups)
        self.QSpinBox_nb_groups.valueChanged.connect(self.change_nb_groups)
        self.fbox.addRow(self.label_groups,self.QSpinBox_nb_groups)

        # operations #
        self.label_operations = QLabel("Operations")
        self.vbox_operations = QVBoxLayout()
        self.QCheckBox_mean = QCheckBox("Mean")
        self.QCheckBox_mean.setChecked(self.mean_op)
        self.QCheckBox_mean.stateChanged.connect(self.change_operations)
        self.vbox_operations.addWidget(self.QCheckBox_mean)
        self.QCheckBox_variance = QCheckBox("Variance")
        self.QCheckBox_variance.setChecked(self.variance_op)
        self.QCheckBox_variance.stateChanged.connect(self.change_operations)
        self.vbox_operations.addWidget(self.QCheckBox_variance)
        self.QCheckBox_min_max = QCheckBox("Min Max")
        self.QCheckBox_min_max.setChecked(self.min_max_op)
        self.QCheckBox_min_max.stateChanged.connect(self.change_operations)
        self.vbox_operations.addWidget(self.QCheckBox_min_max)
        self.QCheckBox_threshold = QCheckBox("Threshold Exceedance")
        self.QCheckBox_threshold.setChecked(self.threshold_op)
        self.QCheckBox_threshold.stateChanged.connect(self.disable_threshold)
        self.QCheckBox_threshold.stateChanged.connect(self.change_operations)
        self.vbox_operations.addWidget(self.QCheckBox_threshold)
        self.check_nb_param()
        self.QCheckBox_sobol.stateChanged.connect(self.change_operations)
        self.QCheckBox_sobol.stateChanged.connect(self.check_nb_param)
        self.vbox_operations.addWidget(self.QCheckBox_sobol)
        self.fbox.addRow(self.label_operations,self.vbox_operations)

        # threshold #
        self.label_threshold = QLabel("Threshold Value")
        self.QDoubleSpinBox_threshold = QDoubleSpinBox()
        self.QDoubleSpinBox_threshold.setRange(-60000,60000)
        self.fbox.addRow(self.label_threshold,self.QDoubleSpinBox_threshold)

        # buttons #
        self.hbox_button = QHBoxLayout()
        self.QPushButton_ok = QPushButton("OK")
        self.QPushButton_save = QPushButton("Save")
        self.QPushButton_cancel = QPushButton("Cancel")
        self.hbox_button.addWidget(self.QPushButton_ok)
        self.hbox_button.addWidget(self.QPushButton_save)
        self.hbox_button.addWidget(self.QPushButton_cancel)
        self.connect(self.QPushButton_ok,  SIGNAL("clicked()"), self.launch_heatc)
        self.connect(self.QPushButton_cancel,  SIGNAL("clicked()"), self.close)
        self.fbox.addItem(self.hbox_button)

        # layout #
        self.setLayout(self.fbox)
        self.setWindowTitle("Melissa Gui")


    def launch_heatc(self):
        os.chdir("../examples/heat_example")
        self.set_operations_string()
        self.set_parameters()
        launch_heatc(self.nb_parameters,
                     self.nb_simu,
                     self.nb_groups,
                     self.nb_time_steps,
                     self.operations,
                     self.threshold,
                     self.mpi_options,
                     self.nb_proc_simu,
                     self.nb_proc_server,
                     self.server_path,
                     self.range_min,
                     self.range_max,
                     self.coupling,
                     self.pyzmq)
        os.chdir(cwd)

    def set_operations_string(self):
        self.operations = []
        if self.mean_op == True:
            self.operations.append("mean")
        if self.variance_op == True:
            self.operations.append("variance")
        if self.min_max_op == True:
            self.operations.append("min")
            self.operations.append("max")
        if self.threshold_op == True:
            self.operations.append("threshold")
        if self.sobol_op == True:
            self.operations.append("sobol")
        print self.operations

    def set_parameters(self):
        for i in range(self.nb_parameters):
            self.range_min.append(deepcopy(self.parameter_list[i].range_min))
            self.range_max.append(deepcopy(self.parameter_list[i].range_max))


    def change_username(self):
        self.username = self.QLineEdit_username.text()

    def change_nb_groups(self):
        self.nb_groups = self.QSpinBox_nb_groups.value()
        self.nb_simu = self.QSpinBox_nb_groups.value()

    def change_operations(self):
        self.mean_op = self.QCheckBox_mean.isChecked()
        self.variance_op = self.QCheckBox_variance.isChecked()
        self.min_max_op = self.QCheckBox_min_max.isChecked()
        self.threshold_op = self.QCheckBox_threshold.isChecked()
        self.sobol_op = self.QCheckBox_sobol.isChecked()
        if operations == []:
            self.mean_op = True
            self.variance_op = True

    def check_nb_param(self):
        if self.nb_parameters < 2:
            self.QCheckBox_sobol.setChecked(False)
            self.QCheckBox_sobol.setDisabled(True)
        else:
            self.QCheckBox_sobol.setDisabled(False)
            self.QCheckBox_sobol.setChecked(self.sobol_op)


    def disable_threshold(self):
        self.QDoubleSpinBox_threshold.setDisabled(not self.QCheckBox_threshold.isChecked())
        self.label_threshold.setDisabled(not self.QCheckBox_threshold.isChecked())

    def show_add_param_dialog(self):
         self.dialog = melissa_param_dialog(parent = self, status = "Add", item = None)
#         self.dialog = melissa_param_dialog(parent = None, status = "Add", item = None)
         self.dialog.setModal(True)
         self.dialog.show()

    def show_modif_param_dialog(self, item):
         self.dialog = melissa_param_dialog(parent = self, status = "Edit", item = item)
         self.dialog.setModal(True)
         self.dialog.show()

    def add_parameter(self):
        if len(self.parameter_list)<nb_parameters:
            self.parameter_list.append(melissa_parameter())
        if len(self.parameter_list)<1:
            self.parameter_list.append(melissa_parameter())
            self.nb_parameters = 1
        self.QListWidget_param.addItem(self.parameter_list[-1].name+"  ["+str(self.parameter_list[-1].range_min)+" : "+str(self.parameter_list[-1].range_max)+"]")
        self.QListWidget_param.setCurrentRow(self.nb_parameters-1)
        self.check_nb_param()

        print self.parameter_list[-1].name
        self.check_nb_param()

    def remove_parameter(self):
        if len(self.parameter_list)>1:
            row = self.QListWidget_param.currentRow()
            print "Parameter "+str(row+1)+" deleted"
            self.nb_parameters -= 1
            self.QListWidget_param.takeItem(row)
            del self.parameter_list[row]
        else:
            print "Can not remove the last parameter"
        self.check_nb_param()

    def print_plop(self, item):
        print "plop "+str(item.text())+" "+str(self.QListWidget_param.currentRow())

class melissa_param_dialog(QDialog):
    def __init__(self, parent = None, status = "Add", item = None):
        QWidget.__init__(self, parent)

        self.parent = parent
        self.item = item
        self.fbox = QFormLayout()
        if status == "Edit":
            self.new_parameter = deepcopy(self.parent.parameter_list[self.parent.QListWidget_param.row(self.item)])
        else:
            self.new_parameter = (melissa_parameter())
        # name #
        self.label_name = QLabel("Name")
        self.QLineEdit_name = QLineEdit(self.new_parameter.name)
        self.QLineEdit_name.setMaxLength(128);
        self.QLineEdit_name.setReadOnly(False);
        self.connect(self.QLineEdit_name,  SIGNAL("validated()"), self.change_name)
        self.change_name()
        self.fbox.addRow(self.label_name,self.QLineEdit_name)
        # range #
        self.hbox_range_param=QHBoxLayout()
#        self.QLineEdit_param_min = QLineEdit(str(parent.new_param_range_min))
        self.QDoubleSpinBox_param_min = QDoubleSpinBox()
        self.QDoubleSpinBox_param_min.setValue(self.new_parameter.range_min)
        self.QDoubleSpinBox_param_min.setRange(-60000,60000)
#        self.QLineEdit_param_min.setValidator(QDoubleValidator())
#        self.QLineEdit_param_min.setMaxLength(12)
        self.connect(self.QDoubleSpinBox_param_min,  SIGNAL("editingFinished()"), self.change_min_of_max)
        self.connect(self.QDoubleSpinBox_param_min,  SIGNAL("editingFinished()"), self.change_range_min)
#        self.change_range_min()
        self.QLabel_range_min = QLabel("From")
#        self.QLineEdit_param_max = QDoubleSpinBox(str(parent.new_param_range_max))
        self.QDoubleSpinBox_param_max = QDoubleSpinBox()
        self.QDoubleSpinBox_param_max.setValue(self.new_parameter.range_max)
        self.QDoubleSpinBox_param_max.setRange(-60000,60000)
#        self.QLineEdit_param_max.setValidator(QDoubleValidator());
#        self.QLineEdit_param_max.setMaxLength(12);
        self.connect(self.QDoubleSpinBox_param_max,  SIGNAL("editingFinished()"), self.change_max_of_min)
        self.connect(self.QDoubleSpinBox_param_max,  SIGNAL("editingFinished()"), self.change_range_max)
#        self.change_range_max()
        self.QLabel_range_max = QLabel("To")
        self.change_max_of_min()
        self.change_min_of_max()
        self.hbox_range_param.addWidget(self.QLabel_range_min)
        self.hbox_range_param.addWidget(self.QDoubleSpinBox_param_min)
        self.hbox_range_param.addWidget(self.QLabel_range_max)
        self.hbox_range_param.addWidget(self.QDoubleSpinBox_param_max)
        self.fbox.addRow(QLabel("Range"),self.hbox_range_param)
        # button #
        self.hbox_button = QHBoxLayout()
        self.add = QPushButton(status,self)
        if status == "Edit":
            self.remove = QPushButton("Delete",self)
            self.connect(self.remove,  SIGNAL("clicked()"), parent.remove_parameter)
            self.connect(self.remove,  SIGNAL("clicked()"), self.close)
        self.cancel = QPushButton("Cancel",self)
        self.connect(self.add,  SIGNAL("clicked()"), self.change_range_min)
        self.connect(self.add,  SIGNAL("clicked()"), self.change_range_max)
        self.connect(self.add,  SIGNAL("clicked()"), self.change_name)
        if status == "Edit":
            parent.connect(self.add,  SIGNAL("clicked()"), self.edit_parameter)
        else:
            self.connect(self.add,  SIGNAL("clicked()"), self.add_parameter)
            self.connect(self.add,  SIGNAL("clicked()"), parent.add_parameter)
        self.connect(self.add,  SIGNAL("clicked()"), self.close)
        self.connect(self.cancel,  SIGNAL("clicked()"), self.close)
        self.hbox_button.addWidget(self.add)
        if status == "Edit":
            self.hbox_button.addWidget(self.remove)
        self.hbox_button.addWidget(self.cancel)
        if status == "Edit":
            self.setWindowTitle("Edit Parameter")
        else:
            self.setWindowTitle("New Parameter")
        self.setWindowModality(Qt.ApplicationModal)
        self.fbox.addRow(QLabel(""),self.hbox_button)
        self.setLayout(self.fbox)
#        self.open()

    def change_name(self):
        self.new_parameter.name=self.QLineEdit_name.text()

    def change_range_min(self):
        self.new_parameter.range_min=float(self.QDoubleSpinBox_param_min.value())

    def change_range_max(self):
        self.new_parameter.range_max=float(self.QDoubleSpinBox_param_max.value())

    def add_parameter(self):
        self.parent.nb_parameters += 1
        self.parent.parameter_list.append(deepcopy(self.new_parameter))

    def edit_parameter(self):
        row = self.parent.QListWidget_param.row(self.item)
        self.parent.parameter_list[row]=deepcopy(self.new_parameter)
#        self.parent.QListWidget_param.setItemWidget(self.item, QLabel(self.parent.parameter_list[row].name))
        self.parent.QListWidget_param.takeItem(row)
        self.parent.QListWidget_param.insertItem(row, self.parent.parameter_list[row].name+"  ["+str(self.parent.parameter_list[row].range_min)+" : "+str(self.parent.parameter_list[row].range_max)+"]")
        self.parent.QListWidget_param.setCurrentRow(row)

    def change_min_of_max(self):
        if self.QDoubleSpinBox_param_max.value() < self.QDoubleSpinBox_param_min.value():
            self.QDoubleSpinBox_param_max.setValue(self.QDoubleSpinBox_param_min.value())
#        self.QDoubleSpinBox_param_max.setMinimum(self.QDoubleSpinBox_param_min.value())
#        self.QDoubleSpinBox_param_max.correctionMode()

    def change_max_of_min(self):
        if self.QDoubleSpinBox_param_min.value() > self.QDoubleSpinBox_param_max.value():
            self.QDoubleSpinBox_param_min.setValue(self.QDoubleSpinBox_param_max.value())
#        self.QDoubleSpinBox_param_min.setMaximum(self.QDoubleSpinBox_param_max.value())

class melissa_parameter:
    def __init__(self, name="NewParameter", range_min = 0.0, range_max = 1.0):
        self.name = name
        self.range_min = range_min
        self.range_max = range_max


if __name__ == '__main__':
    app = QApplication(sys.argv)
    gui = melissa_gui()
    gui.show()
    sys.exit(app.exec_())
