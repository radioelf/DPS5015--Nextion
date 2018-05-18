#! /usr/bin/env python
# -*- coding: utf-8 -*-
'''
original;
https://github.com/lambcutlet/DPS5005_pyGUI

info:
https://www.mediafire.com/folder/3iogirsx1s0vp/DPS_communication_upper_computer
www.ls-homeprojects.co.uk

Configuración comunicaciones desde el hardware:
desconecte la entrada de potencia y carga, seguidamente mantenga presionada la tecla flecha arriba
mientras se enciende para acceder al área de configuración de la interfaz.
configurar:
ID de la unidad Modbus
velocidad en baudios
pin bluetooth
Presione 'set' dos veces para salir. 

Calibración:
desconecte la entrada de potencia y carga, seguidamente mantenga presionada la tecla SET y
conecte la fuente de alimentación de entrada, hasta que la pantalla LCD se encienda.

'''
import traceback, sys
import subprocess
import glob
import serial
import time
import os
import csv
import datetime

from dps_modbus import Serial_modbus
from dps_modbus import Dps5015
from dps_modbus import Import_limits

from PyQt5.QtCore import pyqtSlot, pyqtSignal, QRunnable, QThreadPool, QTimer, QThread, QCoreApplication, QObject, QMutex
from PyQt5.QtWidgets import QApplication, QMainWindow, QSlider, QAction, QFileDialog, QGraphicsView
from PyQt5.QtGui import QIcon
from PyQt5.uic import loadUi

import pyqtgraph as pg
from pyqtgraph import PlotWidget, GraphicsLayoutWidget
import numpy as np
import logging

mutex = QMutex()
dps = 0
contador = 0

class WorkerSignals(QObject):
	finished = pyqtSignal()
	error = pyqtSignal(tuple)
	result = pyqtSignal(object)
	progress = pyqtSignal(int)

class Worker(QRunnable):
    def __init__(self, fn, *args, **kwargs):
	super(Worker, self).__init__()
	# Almacenar argumentos de constructor (reutilizados para el procesamiento)
	self.fn = fn
	self.args = args
	self.kwargs = kwargs
	self.signals = WorkerSignals()
	# Inclulle la rellamada a kwargs
	kwargs['progress_callback'] = self.signals.progress

    @pyqtSlot()
    def run(self):
		'''
		Inicializa la función con los argumentos pasados, kwargs.
		'''
		try:
			result = self.fn(*self.args, **self.kwargs)
		except:
			traceback.print_exc()
			exctype, value = sys.exc_info()[:2]
			self.signals.error.emit((exctype, value, traceback.format_exc()))
		else:
			self.signals.result.emit(result)                            # retorna el resultado del proceso
		finally:
			self.signals.finished.emit()                                # finalizado


class dps_GUI(QMainWindow):
	def __init__(self):
		super(dps_GUI,self).__init__()
		loadUi('dps_GUI.ui', self)
		self.setWindowTitle('DPS5015')

	#--- threading
		self.threadpool = QThreadPool()
		logging.basicConfig(level=logging.DEBUG)
		#print("Multihilos como máximo %d hilos" % self.threadpool.maxThreadCount())

	#--- globales
		self.serialconnected = False
		self.slider_in_use = False
		self.initial_state = True
		self.connect_ok = False
		self.CSV_file = ''
		self.CSV_list = []
		self.graph_X = []
		self.graph_Y1 = []
		self.graph_Y2 = []
		self.time_old = time.time()
		
		self.comboBox_datarate.setEnabled(False)
		self.radioButton_debug.setEnabled(True)
		self.lineEdit_slave_addr.setEnabled(False)
		self.pushButton_connect.setEnabled(False)
		self.pushButton_mem.setEnabled(False)
		self.label_lock.setEnabled(False)
		self.label_model.setEnabled(False)
		self.label_version.setEnabled(False)
		self.label_onoff.setEnabled(False)
		self.label_cccv.setEnabled(False)
		self.label_protect.setEnabled(False)
		self.label_brightness.setEnabled(False)
		self.horizontalSlider_brightness.setEnabled(False)
		self.lcdNumber_vin.setEnabled(False)
		self.lineEdit_vset.setEnabled(False)
		self.lineEdit_iset.setEnabled(False)
		self.pushButton_set.setEnabled(False)
		self.pushButton_onoff.setEnabled(False)
		self.lcdNumber_vset.setEnabled(False)
		self.lcdNumber_iset.setEnabled(False)
		self.lcdNumber_vout.setEnabled(False)
		self.lcdNumber_iout.setEnabled(False)
		self.lcdNumber_pout.setEnabled(False)
		self.lcdNumber_wh.setEnabled(False)
		self.lcdNumber_mah.setEnabled(False)
		self.pushButton_CSV.setEnabled(False)
		self.pushButton_CSV_clear.setEnabled(False)
		self.pushButton_save_plot.setEnabled(False)
		self.label_memory.setEnabled(False)
		self.pushButton_clear.setEnabled(False)
		self.graphicsView.setEnabled(False)
		self.pushButton_update.setEnabled(False)

	#--- PlotWidget
		self.pg_plot_setup()

	#--- interconexiones
		self.pushButton_update.clicked.connect(self.pushButton_update_clicked)
		self.pushButton_save_plot.clicked.connect(self.pushButton_save_plot_clicked)
		self.pushButton_clear.clicked.connect(self.pushButton_clear_clicked)

		self.pushButton_onoff.clicked.connect(self.pushButton_onoff_clicked)
		self.pushButton_set.clicked.connect(self.pushButton_set_clicked)
		self.pushButton_connect.clicked.connect(self.pushButton_connect_clicked)
		self.pushButton_mem.clicked.connect(self.pushButton_mem_clicked)

		self.pushButton_CSV.clicked.connect(self.pushButton_CSV_clicked)
		self.pushButton_CSV_clear.clicked.connect(self.pushButton_CSV_clear_clicked)

		self.horizontalSlider_brightness.sliderReleased.connect(self.horizontalSlider_brightness_sliderReleased)
		self.horizontalSlider_brightness.sliderPressed.connect(self.horizontalSlider_brightness_sliderPressed)
		self.actionOpen.triggered.connect(self.file_open)
		self.actionExit.triggered.connect(self.close)

	#--- Inicia
		self.combobox_populate()

	#--- configurar y ejecutar la tarea de fondo
		self.timer2 = QTimer()
		self.timer2.setInterval(10)
		self.timer2.timeout.connect(self.action_CSV)

		self.timer = QTimer()
		self.timer.setInterval(1000)
		self.timer.timeout.connect(self.read_all)


	def pg_plot_setup(self):                                            # el eje derecho no está conectado a la escala automática a la izquierda, 'A' icono en la parte inferior LHD
		self.p1 = self.graphicsView.plotItem
		self.p1.setClipToView(True)
	# eje x 
		self.p1.setLabel('bottom', 'Tiempo', units='s', color='g', **{'font-size':'10pt'})
		self.p1.getAxis('bottom').setPen(pg.mkPen(color='g', width=1))

	# eje Y1 
		self.p1.setLabel('left', 'Tensión', units='V', color='r', **{'font-size':'10pt'})
		self.p1.getAxis('left').setPen(pg.mkPen(color='r', width=1))

	# configura viewbox para el eje derecho
		self.p2 = pg.ViewBox()
		self.p1.showAxis('right')
		self.p1.scene().addItem(self.p2)
		self.p1.getAxis('right').linkToView(self.p2)
		self.p2.setXLink(self.p1)

	# eje Y2
		self.p1.setLabel('right', 'Corriente', units="A", color='c', **{'font-size':'10pt'})
		self.p1.getAxis('right').setPen(pg.mkPen(color='c', width=1))

	# escala ViewBox para escena
		self.p1.vb.sigResized.connect(self.updateViews)

	def updateViews(self):
		self.p2.setGeometry(self.p1.vb.sceneBoundingRect())
		self.p2.linkedViewChanged(self.p1.vb, self.p2.XAxis)

#--- actualiza graph
	def update_graph_plot(self):
		start = time.time()
		X = np.asarray(self.graph_X, dtype=np.float32)
		Y1 = np.asarray(self.graph_Y1, dtype=np.float32)
		Y2 = np.asarray(self.graph_Y2, dtype=np.float32)

		pen1=pg.mkPen(color='r',width=1.0)
		pen2=pg.mkPen(color='c',width=1.0)

		self.p1.clear()
		self.p2.clear()

		self.p1.plot(X,Y1,pen=pen1, name="V")
		self.p2.addItem(pg.PlotCurveItem(X,Y2,pen=pen2, name="I"))

		app.processEvents()

		a = (time.time() - start) * 1000.0
		self.label_plot_rate.setText(("Plot Rate  : %6.3fms" % (a)))

	def file_open(self):
		filename = QFileDialog.getOpenFileName(self, "Open File", '', 'CSV(*.csv)')
		if filename[0] != '':
			self.CSV_file = filename[0]
		if self.CSV_file != '':
			self.open_CSV(self.CSV_file)

	def file_save(self):
		filename, _ = QFileDialog.getSaveFileName(self, "Save File", datetime.datetime.now().strftime("%Y-%m-%d_%H:%M:%S")+".csv", "All Files (*);; CSV Files (*.csv)")
		if filename != '':
			rows = zip(self.graph_X, self.graph_Y1, self.graph_Y2)
			with open(filename, 'w') as f:
				writer = csv.writer(f)
				row = ['time(s)','voltage(V)','current(A)']
				writer.writerow(row)
				for row in rows:
					writer.writerow(row)

#--- Código relacionado con el hilo
	def progress_fn(self, n):
		print("%d%% terminado" % n)

	def print_output(self, s):
		print(s)
		
	def thread_complete(self):
		print("Finalizado!")

#--- Pulsación botones
	def pushButton_update_clicked(self):                                # actualizar puerto
		self.combobox_populate()
		if self.connect_ok:
			self.lineEdit_info.setText("Puerto OK!")
			self.radioButton_debug.setEnabled(True)
		else:
			self.lineEdit_info.setText("NO encontrado")

	def pushButton_save_plot_clicked(self):                             # guardar
		self.file_save()                                                

	def pushButton_clear_clicked(self):                                 # borrar
		self.graph_X = []
		self.graph_Y1 = []
		self.graph_Y2 = []
		self.time_old = time.time()
		self.p1.clear()
		self.p2.clear()

#	def radioButton_lock_clicked(self):                                 # bloqueo
#		if self.radioButton_lock.isChecked():
#			value = 1
#		else:
#			value = 0
#		self.pass_2_dps('lock', 'w', float(value))

	def pushButton_onoff_clicked(self):                                 # marcha/paro
		if self.pushButton_onoff.isChecked():
			value = 1
			self.pushButton_onoff.setText("Encendida")
		else:
			value = 0
			self.pushButton_onoff.setText("Apagada")
		self.pass_2_dps('onoff', 'w', str(value))

	def pushButton_set_clicked(self):
		if self.lineEdit_vset.text() != '' or self.lineEdit_iset.text() != '':
			try:
				value1 = float(self.lineEdit_vset.text())
			except ValueError:
				self.lineEdit_vset.setText("¡Escribe un valor!")
				return
			try:
				value2 = float(self.lineEdit_iset.text())
				#value2 = value2/10
			except ValueError:
				self.lineEdit_iset.setText("¡Escribe un valor!")
				return
			self.pass_2_dps('write_voltage_current', 'w', [value1, value2])

	def pushButton_connect_clicked(self):
		if self.pushButton_connect.isChecked():
			self.serial_connect()
			self.initial_state = True
		else:
			self.serial_disconnect()

	def pushButton_CSV_clicked(self):
		if self.CSV_list != '':
			self.action_CSV()

	def pushButton_CSV_clear_clicked(self):
		self.CSV_list = []
		self.timer2.stop()
		self.label_CSV.setText("Pasos restantes: %2d" % len(self.CSV_list))

	def pushButton_mem_clicked(self):
		memoria = int(self.combobox_mem_read())
		self.lineEdit_info_mem.setText("V-out %.2fV, I-out%.2fA, S-ovp%.2fV, S-ocp%.2fA, S-opp%.1fW" % (self.pass_2_dps('voltage_set_mem', 'r', memoria), self.pass_2_dps('current_set_mem', 'r', memoria), self.pass_2_dps('s_ovp', 'r', memoria), self.pass_2_dps('s_ovp', 'r', memoria), self.pass_2_dps('s_opp', 'r', memoria)))

	def open_CSV(self, filename):
		with open(filename, 'r') as f:
			csvReader = csv.reader(f)#, delimiter=',')                  # leer fichero
			next(csvReader, None)                                       # saltar encabezados
			data_list = list(csvReader)
			self.CSV_list = data_list
		self.labelCSV(len(self.CSV_list))

	def labelCSV(self, value):
		self.label_CSV.setText("Pasos restantes: %2d" % value)
# enviar tensión e intensidad leidas del fichero csv
	def action_CSV(self):
		if self.pushButton_onoff.isChecked() == True:
			if len(self.CSV_list) > 0:
				self.label_CSV.setText("Pasos restantes: %2d" % len(self.CSV_list))
				data_list = self.CSV_list

				if len(self.CSV_list) > 1:
					value0 = float(data_list[1][0]) - float(data_list[0][0])
				else:
					value0 = 0
				value1 = float(data_list[0][1])                         # tensión
				value2 = float(data_list[0][2])                         # intensidad

				self.pass_2_dps('write_voltage_current', 'w', [value1, value2])
				self.lineEdit_info.setText("TX-> %.2fV, %.2fA" % (value1, value2))
				
				data_list.pop(0)
				self.timer2.stop()
				self.timer2.setInterval(value0*1000)
				self.timer2.start()
				self.label_CSV.setText("Pasos restantes: %2d" % len(self.CSV_list))
			else:
				self.timer2.stop()

#--- control deslizante para nivel del brillo de la pantalla
	def horizontalSlider_brightness_sliderPressed(self):
		self.slider_in_use = True

	def horizontalSlider_brightness_sliderReleased(self):
		self.pass_2_thread(self.slider_change)
		self.slider_in_use = False

	def slider_change(self, progress_callback):
		value = self.horizontalSlider_brightness.value()
		self.pass_2_dps('b_led', 'w', str(value))
		self.label_brightness.setText('Nivel de brillo: %s' % value)

#--- Pasar a la función para ejecutar
	def pass_2_thread(self, func):
		worker = Worker(func)                                           # Cualquier otro args, kwargs pasan a la función de ejecución
		worker.signals.result.connect(self.print_output)
		worker.signals.finished.connect(self.thread_complete)
		worker.signals.progress.connect(self.progress_fn)
		self.threadpool.start(worker)

	def update_values_fast(self, progress_callback):
		self.refresh_all()
		progress_callback.emit(1)

#--- leer y mostrar los valores del display DPS
	def read_all(self):
		start = time.time()
		data = self.pass_2_dps('read_all')
		if data != False:
			self.vout = ("%5.2f" % data[2])	                            # vout
			self.iout = ("%5.2f" % data[3])	                            # iout

			self.time_interval = time.time() - self.time_old
			#print self.time_interval

			self.graph_X.append(self.time_interval)#len(self.graph_Y1))
			self.graph_Y1.append(self.vout)
			self.graph_Y2.append(self.iout)

			self.update_graph_plot()

			self.lcdNumber_vset.display("%5.2f" % data[0])              # vset
			self.lcdNumber_iset.display("%5.2f" % data[1])              # iset
			self.lcdNumber_vout.display(self.vout)                      # vout
			self.lcdNumber_iout.display(self.iout)                      # iout

			self.lcdNumber_pout.display("%5.2f" % data[4])              # potencia
			self.lcdNumber_vin.display("%5.2f" % data[5])               # vin
		# bloqueo
			value = data[6]                                             # bloqueo
			if value == 1:
				self.label_lock.setText('Bloqueo     :   ON')
			else:
				self.label_lock.setText('Bloqueo     :   OFF')
		# protección
			value = data[7]                                             # protección
			if value == 1:
				self.label_protect.setText('Protección :   OVP')        # sabretensión
			elif value == 2:
				self.label_protect.setText('Protección :   OCP')        # sobreintensidad
			elif value == 3:
				self.label_protect.setText('Protección :   OPP')        # sobrepotencia
			else:
				self.label_protect.setText('Protección :   OK')

		# cv/cc
			if data[8] == 1:                                            # modo tensión/corriente constante
				self.label_cccv.setText('Modo        :   CC')
			else:
				self.label_cccv.setText('Modo        :   CV')

		# on/off
			value = data[9]
			if value == 1 and self.pushButton_onoff.isChecked() == True:
				self.label_onoff.setText('Salida      :   ON')	        # on
			elif value == 1 and self.pushButton_onoff.isChecked() == False:
				self.label_onoff.setText('Salida      :   ON')	        # on
				self.pushButton_onoff.setChecked(True)
				self.pushButton_onoff_clicked()
			elif value == 0 and self.pushButton_onoff.isChecked() == True:
				self.label_onoff.setText('Salida      :   OFF')	        # off
				self.pushButton_onoff.setChecked(False)
				self.pushButton_onoff_clicked()
			elif value == 0 and self.pushButton_onoff.isChecked() == False:
				self.label_onoff.setText('Salida      :   OFF')	        # off
			if value ==1:
				contador = int (time.time())
			else:
				contador =0
				self.label_mah.setText("mAh")
				self.lcdNumber_wh.display(0)
				self.lcdNumber_mah.display(0)
		# control deslizante
			if self.slider_in_use == False:				    # actualizar cuando no esté en uso
				value = int(data[10])
				self.horizontalSlider_brightness.setValue(value)	# brillo
				self.label_brightness.setText('Nivel de brillo: %s' % value)
			self.label_model.setText("Modelo      :   %s" % data[11])	# model
			
			self.label_version.setText("Versión     :   %s" % data[12])	# versión
			#print data
			if contador and data[4]:
				wh = float (data[4] * ((time.time() -contador)/3600))        # Wh = P*t
				self.lcdNumber_wh.display("%4.2f" % wh)
				mah = float ((wh*1000)/data[2])                              # (Wh*1000)/V = mAh
				if mah >999:
					self.label_mah.setText("Ah")
				self.lcdNumber_mah.display("%4.1f" % mah)
		a = (time.time() - start) * 1000.0
		self.label_data_rate.setText("Velocidad datos : %6.3fms" % a)

	def combobox_ports_read(self):
		return self.comboBox_ports.currentText()

	def combobox_datarate_read(self):
		return self.comboBox_datarate.currentText()

	def combobox_mem_read(self):
		return self.comboBox_mem.currentText()

	def combobox_populate(self):                                        # recopila información sobre el inicio
		self.comboBox_ports.clear()
		self.comboBox_ports.addItems(self.serial_ports())

		self.comboBox_datarate.clear()
		self.comboBox_datarate.addItems(["19200", "9600", "4800", "2400"])

		self.comboBox_mem.clear()
		self.comboBox_mem.addItems(["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"])

	def pass_2_dps(self, function, cmd = "r", value = 0):
		if self.serialconnected != False:
			mutex.lock()
			a = eval("dps.%s('%s', %s)" % (function, cmd, value))
			mutex.unlock()
			return(a)
		return False

#--- Puerto serie 
	def serial_ports(self):
		if sys.platform.startswith('win'):
			ports = ['COM%s' % (i + 1) for i in range(256)]
		elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
			# esto excluye su terminal actual "/dev/tty"
			ports = glob.glob('/dev/tty[A-Za-z]*')
		elif sys.platform.startswith('darwin'):
			ports = glob.glob('/dev/tty.*')
		else:
			raise EnvironmentError('Plataforma NO soportada')

		result = []
		for port in ports:
			try:
				s = serial.Serial(port)
				s.close()
				result.append(port)
			except (OSError, serial.SerialException):
				pass
		if result:
			self.pushButton_connect.setEnabled(True)
			self.comboBox_datarate.setEnabled(True)
			self.lineEdit_slave_addr.setEnabled(True)
			self.pushButton_update.setEnabled(False)
			self.connect_ok = True
		else:
			self.lineEdit_info.setText("NO conectado")
			logging.debug('se crea el eje semilogaritmico')
			self.pushButton_update.setEnabled(True)
			self.radioButton_debug.setEnabled(False)
		return result

	def serial_connect(self):
		port = str(self.combobox_ports_read())
		baudrate = int(self.combobox_datarate_read())
		slave_addr = int(self.lineEdit_slave_addr.text())
		try:
			limits = Import_limits("dps5015_limits.ini")
			ser = Serial_modbus(port, slave_addr, baudrate, 8, self.radioButton_debug.isChecked())
			global dps
			dps = Dps5015(ser, limits)                                  #'/dev/ttyUSBx', 1, 19200, 8)
			if dps.version() != '':
				self.radioButton_debug.setEnabled(False)
				self.label_lock.setEnabled(True)
				self.label_model.setEnabled(True)
				self.label_version.setEnabled(True)
				self.label_onoff.setEnabled(True)
				self.label_cccv.setEnabled(True)
				self.label_protect.setEnabled(True)
				self.label_brightness.setEnabled(True)
				self.horizontalSlider_brightness.setEnabled(True)
				self.lcdNumber_vin.setEnabled(True)
				self.lineEdit_vset.setEnabled(True)
				self.lineEdit_iset.setEnabled(True)
				self.pushButton_set.setEnabled(True)
				self.pushButton_onoff.setEnabled(True)
				self.pushButton_mem.setEnabled(True)
				self.lcdNumber_vset.setEnabled(True)
				self.lcdNumber_iset.setEnabled(True)
				self.lcdNumber_vout.setEnabled(True)
				self.lcdNumber_iout.setEnabled(True)
				self.lcdNumber_pout.setEnabled(True)
				self.lcdNumber_wh.setEnabled(True)
				self.lcdNumber_mah.setEnabled(True)
				self.pushButton_CSV.setEnabled(True)
				self.pushButton_CSV_clear.setEnabled(True)
				self.pushButton_save_plot.setEnabled(True)
				self.pushButton_clear.setEnabled(True)
				self.graphicsView.setEnabled(True)
				self.label_memory.setEnabled(True)
				self.lineEdit_info.setText("!OK!")
				self.pushButton_connect.setText("Desconectar")
				self.serialconnected = True
				self.timer.start()
				self.pass_2_dps('lock', 'w', float(1))                    # si estamos conectado bloqueamos teclado
		except Exception as detail:
			print datetime.datetime.now().strftime("%y-%m-%d %H:%M:%S"), "Error ", detail
			self.serialconnected = False
			self.lineEdit_info.setText("Error !!!")
			self.pushButton_connect.setChecked(False)

	def serial_disconnect(self):
		self.pass_2_dps('lock', 'w', float(0))                          # desbloqueamo teclado
		self.pass_2_dps('onoff', 'w', str(0))
		self.serialconnected = False
		self.timer.stop()
		self.lineEdit_info.setText("Desconectado")
		self.pushButton_connect.setText("Conectar")
		self.radioButton_debug.setEnabled(True)
		self.comboBox_datarate.setEnabled(False)
		self.radioButton_debug.setEnabled(True)
		self.lineEdit_slave_addr.setEnabled(False)
		self.pushButton_mem.setEnabled(False)
		self.label_lock.setEnabled(False)
		self.label_model.setEnabled(False)
		self.label_version.setEnabled(False)
		self.label_onoff.setEnabled(False)
		self.label_cccv.setEnabled(False)
		self.label_protect.setEnabled(False)
		self.label_brightness.setEnabled(False)
		self.horizontalSlider_brightness.setEnabled(False)
		self.lcdNumber_vin.setEnabled(False)
		self.lineEdit_vset.setEnabled(False)
		self.lineEdit_iset.setEnabled(False)
		self.pushButton_set.setEnabled(False)
		self.pushButton_onoff.setEnabled(False)
		self.lcdNumber_vset.setEnabled(False)
		self.lcdNumber_iset.setEnabled(False)
		self.lcdNumber_vout.setEnabled(False)
		self.lcdNumber_iout.setEnabled(False)
		self.lcdNumber_pout.setEnabled(False)
		self.lcdNumber_wh.setEnabled(False)
		self.lcdNumber_mah.setEnabled(False)
		self.pushButton_CSV.setEnabled(False)
		self.pushButton_CSV_clear.setEnabled(False)
		self.pushButton_save_plot.setEnabled(False)
		self.label_memory.setEnabled(False)
		self.pushButton_clear.setEnabled(False)
		self.graphicsView.setEnabled(False)
		self.pushButton_update.setEnabled(False)

app = QApplication(sys.argv)
widget = dps_GUI()
widget.show()

sys.exit(app.exec_())


