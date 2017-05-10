#!/usr/bin/python
# -*- coding : utf-8 -*-

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from copy import deepcopy
import os, sys, signal
import getpass
import imp
import time

cwd = os.getcwd()
if len(sys.argv) == 2:
    master_path = sys.argv[1]
else:
    master_path = "./"

signal.signal(signal.SIGINT, signal.SIG_DFL)

#class load_dialog(QDialog):
#    def __init__(self, parent = None):
#        QWidget.__init__(self, parent)
#        self.vbox = QVBoxLayout()
#        self.QLineEdit_path = QLineEdit(cwd+"/../examples/heat_example")
#        self.vbox.addWidget(self.QLineEdit_path)
#        self.hbox_button = QHBoxLayout()
#        self.QPushButton_load = QPushButton("Load",self)
#        self.QPushButton_cancel = QPushButton("Cancel",self)
#        self.hbox_button.addWidget(self.QPushButton_load)
#        self.hbox_button.addWidget(self.QPushButton_cancel)
#        self.connect(self.QPushButton_load,  SIGNAL("clicked()"), self.load_master)
#        self.connect(self.QPushButton_load,  SIGNAL("clicked()"), self.close)
#        self.connect(self.QPushButton_cancel,  SIGNAL("clicked()"), self.close)
##        self.connect(self.QPushButton_cancel,  SIGNAL("clicked()"), parent.close)
#        self.vbox.addItem(self.hbox_button)
#        self.setLayout(self.vbox)

#    def load_master (self):
##        sys.path.append(cwd+"/../examples/heat_example")
#        master_module = os.getcwd()+"/../examples/heat_example/master.py"
#        imp.load_source("master", master_module)
#        from master import *


class melissa_gui(QWidget):
    def __init__(self, parent = None):
        QWidget.__init__(self,parent)

        self.fbox = QFormLayout()

#        self.plop = load_dialog(parent = self)
#        self.plop.setModal(True)
#        self.plop.exec_()

        self.username       = getpass.getuser()
        self.nb_parameters  = nb_parameters
#        self.nb_parameters  = 0
        self.sampling_size  = sampling_size
        self.nb_time_steps  = nb_time_steps
        self.operations     = operations
        self.threshold      = threshold
        self.mpi_options    = mpi_options
        self.nb_proc_simu   = nb_proc_simu
        self.nb_proc_server = nb_proc_server
        self.server_path    = server_path
#        self.server_path    = "../server"
        self.range_min = []
        self.range_max = []
#        for i in range(nb_parameters)
#            self.range_min.append = range_min[i]
#            self.range_max.append = range_max[i]
#        self.range_min      = range_min
#        self.range_max      = range_max
        self.coupling       = coupling

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
        self.modified = False

        # Username #
        self.label_username = QLabel("User Name")
        self.QLineEdit_username = QLineEdit(self.username)
        self.QLineEdit_username.setMaxLength(128);
        self.QLineEdit_username.editingFinished.connect(self.change_username)
        self.fbox.addRow(self.label_username,self.QLineEdit_username)

        print self.modified
        # batch scheduller #
        self.label_sched = QLabel("Batch Scheduller")
        self.QComboBo_sched = QComboBox()
        self.QComboBo_sched.addItems(["Slurm", "CCC", "OAR"])
        self.fbox.addRow(self.label_sched,self.QComboBo_sched)

        # number of parameters #
        self.dialog = None
        self.label_param = QLabel("Parameters")
        self.new_param_name = "NewParam"
        self.new_param_range_min = 0
        self.new_param_range_max = 1
        self.item_list = []
        self.parameter_list = []
        self.QListWidget_param = QListWidget()
#        self.QListWidget_param.itemDoubleClicked.connect(self.print_plop)
        self.QListWidget_param.itemDoubleClicked.connect(self.show_modif_param_dialog)
        if nb_parameters > 0:
            for i in range(nb_parameters):
                self.add_parameter(range_min[i], range_max[i])
        else:
            self.add_parameter()
        self.fbox.addRow(self.label_param,self.QListWidget_param)
        self.QPushButton_add = QPushButton("Add Parameter")
        self.connect(self.QPushButton_add,  SIGNAL("clicked()"), self.show_add_param_dialog)
        self.connect(self.QPushButton_add,  SIGNAL("clicked()"), self.check_nb_param)
        self.fbox.addRow(QLabel(""),self.QPushButton_add)

        # number of groups #
        self.label_groups = QLabel("Sampling size")
        self.QSpinBox_sampling_size = QSpinBox()
        self.QSpinBox_sampling_size.setRange(2,60000)
        self.QSpinBox_sampling_size.setValue(self.sampling_size)
        self.QSpinBox_sampling_size.valueChanged.connect(self.change_sampling_size)
        self.fbox.addRow(self.label_groups,self.QSpinBox_sampling_size)

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
        self.QDoubleSpinBox_threshold.setValue(self.threshold)
        self.connect(self.QDoubleSpinBox_threshold,  SIGNAL("editingFinished()"), self.modif_threshold)

        # buttons #
        self.hbox_button = QHBoxLayout()
        self.QPushButton_ok = QPushButton("Start")
        self.QPushButton_save = QPushButton("Save")
        self.QPushButton_cancel = QPushButton("Close")
        self.hbox_button.addWidget(self.QPushButton_ok)
        self.hbox_button.addWidget(self.QPushButton_save)
        self.hbox_button.addWidget(self.QPushButton_cancel)
        self.connect(self.QPushButton_ok,  SIGNAL("clicked()"), self.launch_heatc)
        self.connect(self.QPushButton_save,  SIGNAL("clicked()"), self.save)
        self.connect(self.QPushButton_cancel,  SIGNAL("clicked()"), self.close_gui)
        self.fbox.addItem(self.hbox_button)

        # layout #
        self.setLayout(self.fbox)
        self.setWindowTitle("Melissa Gui")

    def close_gui(self):
        if self.modified == True:
            close_dial = QMessageBox()
            close_dial.setIcon(QMessageBox.Question)
            close_dial.setText("\nSave unsaved changes ?\n")
            close_dial.setStandardButtons(QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel)
            retval = close_dial.exec_()
            if retval == close_dial.Yes:
                self.save()
                self.close()
            elif retval == close_dial.No:
                self.close()
            else:
                close_dial.close()
        else:
            self.close()


    def launch_heatc(self):
        self.setDisabled(True)
        QApplication.processEvents()
        self.set_operations_string()
        self.set_parameters()
        launch_heatc(self.nb_parameters,
                     self.sampling_size,
                     self.nb_time_steps,
                     self.operations,
                     self.threshold,
                     self.mpi_options,
                     self.nb_proc_simu,
                     self.nb_proc_server,
                     self.server_path,
                     self.range_min,
                     self.range_max,
                     self.coupling)
        self.setEnabled(True)
        QApplication.processEvents()

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

    def set_parameters(self):
        for i in range(self.nb_parameters):
            self.range_min.append(deepcopy(self.parameter_list[i].range_min))
            self.range_max.append(deepcopy(self.parameter_list[i].range_max))


    def change_username(self):
        if self.username != self.QLineEdit_username.text():
            self.username = self.QLineEdit_username.text()
            self.modified = True

    def change_sampling_size(self):
        self.sampling_size = self.QSpinBox_sampling_size.value()
        self.modified = True

    def change_operations(self):
        self.mean_op = self.QCheckBox_mean.isChecked()
        self.variance_op = self.QCheckBox_variance.isChecked()
        self.min_max_op = self.QCheckBox_min_max.isChecked()
        self.threshold_op = self.QCheckBox_threshold.isChecked()
        self.sobol_op = self.QCheckBox_sobol.isChecked()
        if operations == []:
            self.mean_op = True
            self.variance_op = True
        self.modified = True

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

    def add_parameter(self, range_min = 0, range_max = 1):
        if len(self.parameter_list)<nb_parameters:
            self.parameter_list.append(melissa_parameter(range_min = range_min, range_max = range_max))
        if len(self.parameter_list)<1:
            self.parameter_list.append(melissa_parameter())
            self.nb_parameters = 1
            self.modified = True
        self.QListWidget_param.addItem(self.parameter_list[-1].name+"  ["+str(self.parameter_list[-1].range_min)+" : "+str(self.parameter_list[-1].range_max)+"]")
        self.QListWidget_param.setCurrentRow(self.nb_parameters-1)
        self.check_nb_param()

    def remove_parameter(self):
        if len(self.parameter_list)>1:
            row = self.QListWidget_param.currentRow()
            self.nb_parameters -= 1
            self.QListWidget_param.takeItem(row)
            del self.parameter_list[row]
        else:
            print "Can not remove the last parameter"
        self.check_nb_param()
        self.modified = True

    def modif_threshold(self):
        self.threshold=float(self.QDoubleSpinBox_threshold.value())
        self.modified = True

    def save (self):
        os.getcwd()
        file = open ("./options.py", "r")
        contenu = ""
        for ligne in file:
            if "nb_parameters =" in ligne or "nb_parameters=" in ligne:
                contenu += "nb_parameters = "+str(self.nb_parameters)+"\n"
                contenu += "range_min = np.zeros(nb_parameters,float)\n"
                contenu += "range_max = np.zeros(nb_parameters,float)\n"
                for i in range (self.nb_parameters):
                    contenu += "range_min["+str(i)+"] = "+str(self.parameter_list[i].range_min)+"\n"
                    contenu += "range_max["+str(i)+"] = "+str(self.parameter_list[i].range_max)+"\n"
            elif "sampling_size =" in ligne or "sampling_size=" in ligne:
                contenu += "sampling_size = "+str(self.sampling_size)+"\n"
            elif "operations =" in ligne or "operations=" in ligne:
                self.set_operations_string()
                contenu += "operations = "+str(self.operations)+"\n"
            elif "threshold =" in ligne or "threshold=" in ligne:
                contenu += "threshold = "+str(self.threshold)+"\n"
            elif not ("range_min" in ligne or "range_max" in ligne):
                contenu += ligne
        file.close()
        file = open ("./options.py", "w")
        file.write(contenu)
        file.close()
        self.modified = False
        print "Study saved"

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
        parent.modified = True

    def change_range_max(self):
        self.new_parameter.range_max=float(self.QDoubleSpinBox_param_max.value())
        parent.modified = True

    def add_parameter(self):
        self.parent.nb_parameters += 1
        self.parent.parameter_list.append(deepcopy(self.new_parameter))
        parent.modified = True

    def edit_parameter(self):
        row = self.parent.QListWidget_param.row(self.item)
        self.parent.parameter_list[row]=deepcopy(self.new_parameter)
#        self.parent.QListWidget_param.setItemWidget(self.item, QLabel(self.parent.parameter_list[row].name))
        self.parent.QListWidget_param.takeItem(row)
        self.parent.QListWidget_param.insertItem(row, self.parent.parameter_list[row].name+"  ["+str(self.parent.parameter_list[row].range_min)+" : "+str(self.parent.parameter_list[row].range_max)+"]")
        self.parent.QListWidget_param.setCurrentRow(row)
        parent.modified = True

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


class error_dialog(QMessageBox):
    def __init__(self, parent = None):
        QMessageBox.__init__(self, parent)
        self.setIcon(QMessageBox.Critical)
        self.vbox = QVBoxLayout()
        self.setText("\nERROR: Missing Melissa Master file\n")
        self.setStandardButtons(QMessageBox.Ok)
        self.buttonClicked.connect(self.close)
        self.setDetailedText("Usage :\neither run melissa_gui.py in a directory containing a master.py file\nor give \"path/to/melissa_master/directory\" as an argument to melissa_gui.py.")


if __name__ == '__main__':
    app = QApplication(sys.argv)
    if not os.path.isfile(master_path+"/master.py"):
        print "ERROR no master file given"
        gui = error_dialog()
    else:
        os.chdir (master_path)
        imp.load_source("master", "./master.py")
        imp.load_source("options", "./options.py")
        from options import *
        from master import *
        gui = melissa_gui()
    gui.show()
    resolution = QDesktopWidget().screenGeometry(QDesktopWidget().screenNumber(gui))
    gui.move(resolution.x() + (resolution.width() / 2) - (gui.frameSize().width() / 2),
            (resolution.y() + resolution.height() / 2) - (gui.frameSize().height() / 2))
    sys.exit(app.exec_())
