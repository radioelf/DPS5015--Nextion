# -*- coding: utf-8 -*-

'''
original;
https://github.com/lambcutlet/DPS5005_pyGUI


Trama para DPS5015 (SOLO usa tres funciones):
byte 0 -> dirección esclavo
byte 1 -> función, 0x03 leer uno o más registros en el esclavo
byte 2 -> parte alta de la primera dirección MSB
byte 3 -> parte baja de la primera dirección LSB
byte 4 -> parte alta del número de registros a leer
byte 5 -> parte baja del número de registros a leer
byte 7 -> parte alta del valor del control de errores CRC MSB
byte 8 -> parte baja del valor del control de errores CRC LSB

byte 0 -> dirección esclavo
byte 1 -> función  0x06 escribe el dato de un único registro en el esclavo
byte 2 -> parte alta de la primera dirección MSB
byte 3 -> parte baja de la primera dirección LSB
byte 4 -> parte alta del valor a escribir MSB
byte 5 -> parte baja del valor a escribir LSB
byte 6 -> parte alta del valor del control de errores CRC MSB
byte 7 -> parte baja del valor del control de errores CRC LSB

byte 0 -> dirección esclavo
byte 1 -> función  0x10 escribe el dato/s en uno o más registros en el esclavo
byte 2 -> parte alta de la primera dirección MSB
byte 3 -> parte baja de la primera dirección LSB
byte 4 -> número de bytes a escribir, multiplo de 2 (1 palabra/variable = 2 bytes)
byte 5 -> Contenido del primer registro
byte 6 -> Contenido del segundo registro
byte x -> Contenido del x registro
byte x -> Contenido del x registro
byte 4+(número de bytes a escribir *2) -> parte alta del valor del control de errores CRC MSB
byte 5+(número de bytes a escribir *2) -> parte baja del valor del control de errores CRC LSB

Trama RX:
[23.0, 0.51, 0.0, 0.0, 0.0, 54.19, 0, 0, 0, 0, 4, 5015, 1.6, 0, 0, 0]
byte 0 U-set, byte 1 I-set, byte 3 Uout, byte 4 Iout, byte 5 potenecia,
byte 6 Uint, byte 7 lock, byte 8 protec, byte 9 salida ,byte 10 iluminación,
byte 11 modelo, byte 12 versión, byte 13 , byte 14 byte 15
'''

import minimal_modbus                                                   # pip install minimalmodbus
import time
import csv
import ConfigParser

''' 
import the system limit thresholds from the *.ini file.
having a separate file allows simple modification for other versions of DPS supplies.
these limits prevent the program from issuing silly values.
'''
class Import_limits:
	def __init__(self, filename):
		Config = ConfigParser.ConfigParser()
		Config.read(filename)
		b = Config.options('SectionOne')                                # obtenemos los parametro de la seccion SectionOne del fichero dsp5015_limits
		for x in range(len(b)):
			c = b[x]
			exec("self.%s = %s" % (c, Config.get('SectionOne', c)))		

'''
# original inspiration for this came from here:
# DPS3005 MODBUS Example By Luke (www.ls-homeprojects.co.uk) 
#
# Requires minimalmodbus library from: https://github.com/pyhys/minimalmodbus
'''		
class Serial_modbus:
	def __init__(self, port1, addr, baud_rate, byte_size, debug_cmd ):
		self.instrument = minimal_modbus.Instrument(port1, addr)         # nombre del puerto, salvamos direccions (en decimal)
		#self.instrument.serial.port                                    # nombre del puerto serie
		self.instrument.serial.baudrate = baud_rate                     # Baud rate 9600 
		self.instrument.serial.bytesize = byte_size
		self.instrument.serial.timeout = 0.5                            # debe ser baja latencia
		self.instrument.mode = minimal_modbus.MODE_RTU                   # RTU modo
		self.instrument.debug = debug_cmd

	def read(self, reg_addr, decimal_places):                           # funcion 0x03 leer un registro
		return self.instrument.read_register(reg_addr, decimal_places)  # registro, posició en decimal
		
	def read_block(self, reg_addr, size_of_block):                      # funcion 0x03 leer un bloque
		return self.instrument.read_registers(reg_addr, size_of_block)  # registro, tamaño del bloque
			
	def write(self, reg_addr, value, decimal_places):                   # funcion 0x06 escribir en un registro
		self.instrument.write_register(reg_addr, value, decimal_places) # registro, valor, posicione en decimal
	
	def write_block(self, reg_addr, value):                             # funcion 0x10 escribir multiplos registros
		self.instrument.write_registers(reg_addr, value)                # registro, valor
				
class Dps5015:
	def __init__(self, ser, limits):
		self.serial_data = ser
		self.limits = limits
# Ajuste de voltaje, lectura/escritura
	def voltage_set(self, RWaction='r', value=0.0):	
		return self.function(0x00, 2, RWaction, value, self.limits.voltage_set_max, self.limits.voltage_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value 
# ajuste intensidad lectura/escritura
	def current_set(self, RWaction='r', value=0.0):	
		return self.function(0x01, 2, RWaction, value, self.limits.current_set_max, self.limits.current_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value 
# tensión de salida, lectura
	def voltage(self):	
		return self.function(0x02, 2) 
# intensidad de salida, lectura
	def current(self):
		return self.function(0x03, 2)
# potencia, lectura, lectura
	def power(self):	
		return self.function(0x04, 2)
# tensión de entrada, lectura
	def voltage_in(self):
		return self.function(0x05, 2)
# boqueo, lectura/escritura
	def lock(self, RWaction='r', value=0):
		return self.function(0x06, 0, RWaction, value, self.limits.lock_set_max, self.limits.lock_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
# protección, lectura
	def protect(self):
		return self.function(0x07, 0)
# constante de tensión y corriente, lectura
	def cv_cc(self):
		return self.function(0x08, 0)
# marcha/paro, lectura/escritura
	def onoff(self, RWaction='r', value=0):
		return self.function(0x09, 0, RWaction, value, self.limits.onoff_set_max, self.limits.onoff_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
# nivel retroiluminación, lectura/escritura
	def b_led(self, RWaction='r', value=0):
		return self.function(0x0A, 0, RWaction, value, self.limits.b_led_set_max, self.limits.b_led_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
# Modelo, lectura
	def model(self):
		return self.function(0x0B, 0)
# versión , lectura
	def version(self):
		return self.function(0x0C, 1)
# Acceso directo para mostrar la memoria requerida, ectura/escritura
	def extract_m(self, RWaction='r', value=0.0):
		return self.function(0x23, 0, RWaction, value, self.limits.extract_m_set_max, self.limits.extract_m_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
#---memorias		
# Ajuste de voltaje memrias, lectura/escritura
	def voltage_set_mem(self, RWaction='r', memory =0, value=0.0):
		memory =(memory*16)+80
		return self.function(memory, 2, RWaction, value, self.limits.voltage_set2_max, self.limits.voltage_set2_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# ajuste intensidad memorias lectura/escritura
	def current_set_mem(self, RWaction='r', memory =0, value=0.0):
		memory =(memory*16)+81
		return self.function(memory, 2, RWaction, value, self.limits.current_set2_max, self.limits.current_set2_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# protección sobre-tensión, lectura/escritura
	def s_ovp(self, RWaction='r', memory =0, value=0):
		memory =(memory*16)+82
		return self.function(memory, 2, RWaction, value, self.limits.s_ovp_set_max, self.limits.s_ovp_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# protección sobre-corriente, lectura/escritura
	def s_ocp(self, RWaction='r', memory =0, value=0):	
		memory =(memory*16)+83
		return self.function(memory, 2, RWaction, value, self.limits.s_ocp_set_max, self.limits.s_ocp_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# protección sobe-potencia, lectura/escritura
	def s_opp(self, RWaction='r', memory =0, value=0):
		memory =(memory*16)+84
		return self.function(memory, 1, RWaction, value, self.limits.s_opp_set_max, self.limits.s_opp_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# nivel retroiluminación2, lectura/escritura
	def b_led2(self, RWaction='r', memory =0, value=0):
		memory =(memory*16)+85
		return self.function(memory, 0, RWaction, value, self.limits.b_led2_set_max, self.limits.b_led2_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# Número de La memoria, lectura/escritura
	def m_pre(self, RWaction='r', memory =0, value=0):
		memory =(memory*16)+86
		return self.function(memory, 0, RWaction, value, self.limits.m_pre_set_max, self.limits.m_pre_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# Interruptor de potencia, lectura/escritura
	def s_ini(self, RWaction='r', memory =0, value=0):
		memory =(memory*16)+87
		return self.function(memory, 0, RWaction, value, self.limits.s_ini_set_max, self.limits.s_ini_set_min) # reg_addr, decimal_places, RWaction, value, max_value, min_value
		
# Leer los datos como un bloque, mucho más rápido que las lecturas individuales comprobar el numero real que envia
	def read_all(self, RWaction='r', value=0.0):
		data = self.functions(0x00, 16, RWaction, value)                # reg_addr, número de bytes, RWaction, valor
		#--- ajustar valores a puntos flotantes
		data[0] = data[0] / 100.0	                                    # tensión_set
		data[1] = data[1] / 100.0	                                    # intensidad_set
		data[2] = data[2] / 100.0	                                    # tensión
		data[3] = data[3] / 100.0	                                    # intensidad
		data[4] = data[4] / 100.0	                                    # potencia
		data[5] = data[5] / 100.0	                                    # tensión_in
		data[12] = data[12] / 10.0	                                    # versión
		return data
# escribir tensión e intensidad en un bloque
	def write_voltage_current(self, RWaction='r', value=0):	            
		reg_addr = 0x00 
		value[0] = int(value[0] * 100)
		value[1] = int(value[1] * 100)
		self.functions(reg_addr, 0, 'w', value)
		
# escribir bloque
	def write_all(self, reg_addr=0, value=0):	                        
		self.functions(reg_addr, 0, 'w', value)
		
# FUNCION
	def function(self, reg_addr=0, decimal_places=0, RWaction='r', value=0.0, max_value=0, min_value=0):
		if value > max_value or value < min_value: value = 0.0
		if RWaction != 'w':
			return self.serial_data.read(reg_addr, decimal_places)      # escritura->registro, No_of_decimal_places
		else:
			self.serial_data.write(reg_addr, value, decimal_places)     # lectura->registro, valor, No_of_decimal_places
			return 
# funciones
	def functions(self, reg_addr=0, num_of_addr=0, RWaction='r', value=0):
		if RWaction != 'w':
			return self.serial_data.read_block(reg_addr, num_of_addr)   # escritura->registro, dirección
		else:
			self.serial_data.write_block(reg_addr, value)               # lectura->registro, valor
			return 
#retardo
	def delay(self, value):
		global time_old
		value = float(value)
		if value == 0.0:
			time_old = time.time()

		while True:
			time_interval = time.time() - time_old
			if time_interval >= value:
				time_old = time.time()
				break
			time.sleep(0.01)
# Gestión fichero CSV
	def action_csv_file(self, filename='sample.csv', value=0):
		try:
			with open(filename, 'r') as f:
				csvReader = csv.reader(f)                               # separador por=',', leer fichero
				next(csvReader, None)                                   # salta encabezado
				data_list = list(csvReader)
				print data_list
			# inicialización estado dps
				self.voltage_set('w', float(0.0))                       # ajuste tensión a cero
				self.current_set('w', float(0.0))                       # ajuste corriente a cero
				self.onoff('w', 1)                                      # habilitamos salida
				total_time = 0
			# calculo del tiempo de test
				for row in data_list:
					total_time += float(row[0])
				print("Tiempo de muestra: %5.1fsegundos" % (total_time))
			# realizar test
				for row in data_list:
					value0 = float(row[0])
					value1 = float(row[1])
					value2 = float(row[2])
					print(" Periodo: %5.1fs, Voltaje: %5.2fV, Corriente: %5.2fA" % (value0, value1, value2))
					self.voltage_set('w', value1)
					self.current_set('w', value2)
					self.delay(value0)
			self.onoff('w', 0)							                # desactivar la salida
			print("Finalizado!")
			return
		except:
			print("Error al cargar el archivo.")
'''
Este archivo puede trabajar de forma independiente a través de líneas de
comandos, pero es más comodo utilizar interfaz GUI.
'''
if __name__ == '__main__':
	ser = Serial_modbus('/dev/ttyUSB1', 1, 19200, 8, 0)
	limits = Import_limits("dps5015_limits.ini")
	dps = Dps5015(ser, limits)
	bloqueo =False
	try:
		while True:
			print "\nCOMANDOS:"
			print "q    ->salir"
			print "t    ->leer trama"
			print "r    ->leer datos" 
			print "m    ->leer memorias"
			print "w    ->escribir 5V 1A" 
			print "b    ->brillo LCD"
			print "s    -> estado interruptor" 
			print "on   ->encendemos" 
			print "off  ->apagamos"
			print "vset ->ajustar tensión"
			print "iset ->ajustar intensidad"
			print "lock ->bloqueo ON/OFF"
			print "sovp -> protección sobre-tensión"
			print "socp -> protección sobre-intensidad"
			print "sopp -> protección sobre-potencia"
			print "l    -> listar campos"
			print "f    -> dps-control-Book1.csv\n"
			if bloqueo == False:
				dps.lock('w', float(1))                                 # bloqueamos teclado
				bloqueo =True
			route = raw_input("Entrar comando: ")
			if route == "q":
				dps.lock('w', float(0))                                 # desbloqueamos teclado
				quit()
			elif route == "t":
				start = time.time()
				print dps.read_all()
				print (time.time() - start) 
			elif route == "w":
				intensidad = 1/(1*10.0)
				value = [5.00, 1]
				dps.write_voltage_current('w', value)
			elif route == "r":
				start = time.time()
				print("voltage_set :  %6.2f" % dps.voltage_set())
				print("current_set :  %6.3f" % dps.current_set())
				print("voltage     :  %6.2f" % dps.voltage())
				print("current     :  %6.2f" % dps.current())
				print("power       :  %6.2f" % dps.power())
				print("voltage_in  :  %6.2f" % dps.voltage_in())
		
				print("lock        :  %6s" % dps.lock())
				print("protection  :  %6s" % dps.protect())	
				print("cv_cc       :  %6s" % dps.cv_cc())
				print("onoff       :  %6s" % dps.onoff())
				print("b_led       :  %6s" % dps.b_led())
				print("model       :  %6s" % dps.model())
				print("version     :  %6s" % dps.version())
				print("extract_m   :  %6s" % dps.extract_m())
				
				print("voltage_set-0:  %6s" % dps.voltage_set_mem())       # ajuste tensión
				print("current_set-0:  %6s" % dps.current_set_mem())       # ajuste intensidad
				print("s_ovp-0     :  %6s" % dps.s_ovp())               # ajuste protección tensión
				print("s_ocp-0     :  %6s" % dps.s_ocp())               # ajuste proteccion corriente
				print("s_opp-0     :  %6s" % dps.s_opp())               # ajuste protección potencia
				print("b_led2-0    :  %6s" % dps.b_led2())              # nivel pantalla
				print("m_pre-0     :  %6s" % dps.m_pre())               # memoria 
				print("s_ini-0     :  %6s" % dps.s_ini())               # on/off salida fuente
				print (time.time() - start)
			elif route == "m":
				value = int (raw_input("Entrar número bloque de memoria 0-9: "))
				print("voltage_set :  %6s" % dps.voltage_set_mem('r', value))   # ajuste tensión
				print("current_set :  %6s" % dps.current_set_mem('r', value))   # ajuste intensidad
				print("s_ovp       :  %6s" % dps.s_ovp('r', value))          # ajuste protección tensión
				print("s_ocp       :  %6s" % dps.s_ocp('r', value))          # ajuste proteccion corriente
				print("s_opp       :  %6s" % dps.s_opp('r', value))          # ajuste protección potencia
				print("b_led2      :  %6s" % dps.b_led2('r', value))         # nivel pantalla
				print("m_pre       :  %6s" % dps.m_pre('r', value))          # memoria 
				print("s_ini       :  %6s" % dps.s_ini('r', value))          # on/off salida fuente
			elif route == "vset":
				value = raw_input("Entrar valor de la tensión: ")
				dps.voltage_set('w', float(value))
			elif route == "iset":
				value = raw_input("Entrar valor de la intensidad: ")
				dps.current_set('w', float(value))
			elif route == "lock":
				dps.lock('w', float(0))
				print ("De-bloqueo durante 5 segundos")
				time.sleep(5)
				bloqueo =False
				print ("Bloqueado")
			elif route == "on":
				dps.onoff('w', 1)
			elif route == "off":
				dps.onoff('w', 0)
			elif route == "b":
				value = raw_input("Entrar valor (de 0 a 5): ")
				dps.b_led('w', float(value))
			elif route == "sovp":
				value = raw_input("Entrar valor sobre tensión: ")
				dps.s_ovp('w', float(value))
			elif route == "socp":
				value = raw_input("Entrar valorsobre intensidad: ")
				dps.s_ocp('w', float(value))
			elif route == "sopp":
				value = raw_input("Entrar valor sobre potencia: ")
				dps.s_opp('w', float(value))	
			elif route == "s":
				value = raw_input("Entrar estado interruptor al arrancar 1-0: ")
				dps.s_ini('w', float(value))
			elif route == "l":	
				for i in dir(dps):
					print i
			elif route == "f":	
				dps.action_csv_file('dps-control-Book1.csv')
			else:
				pass

	except KeyboardInterrupt:  	                                        # Ctrl+C 
		print "Cerrar"
	finally:
		dps.onoff('w', 0)
		dps.lock('w', float(0))                                         # desbloqueamos teclado
		quit()
