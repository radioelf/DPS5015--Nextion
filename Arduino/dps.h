/* 
https://www.mediafire.com/folder/3iogirsx1s0vp/DPS_communication_upper_computer

-Configurar con display/teclado original
Configuración comunicaciones desde el hardware:
desconecte la entrada de alimentación y la carga, seguidamente mantenga presionada la tecla flecha arriba
y conectar la alimentación para acceder al área de configuración de la interfaz.
configurar:
ID de la unidad Modbus -> 1 para la fuente 1 y 2 para la fuente 2
velocidad en baudios   ->19200
pin bluetooth
Presione 'set' dos veces para salir.

configurar opción memorias:
alimentar el equipo y una vez arrancado pulsar set
acceder a la opción S-ini y seleccionar ON o OFF  para cada memoria 
ON->al acceder a la memoria la salida no cambia, OFF->al acceder  a la memoria la salida de la fuente se apaga

DPS5015 dispone de 10 memorias (0-9), pero realmente son 9 ya que la memoria 0 es la de trabajo y toma el valor 
de la memoria seleccionada

DPS5015 tiene un tiempo de respuesta a una petición de datos muy alto y variable de 240 a 500ms

DS5015 no responde a una petición de datos si se envían peticiones a otro dispositivo con distinta dirección
Modbus durante el periodo de espera de respuesta (240-500ms)

DPS5015 (SOLO usa tres funciones Modbus RTU):
 0x03; < Modbus función 0x03 leer uno o más registros  
 0x06; < Modbus función 0x06 escribir en un registro
 0x10; < Modbus función 0x10 escribir múltiples registros
la latencia es MUY alta, y de un periodo variable de 200 ms a unos 460ms.
NO se puede enviar datos en el periodo de espera de respuesta, a otro dispositivo usando otra dirección Modbus

Registros
0x00 U-SET	ajuste de voltaje -lectura/escritura-
0x01 I-SET	Ajuste de corriente-lectura/escritura-
0x02 UOUT	Salida por display de la tensión -lectura-
0x03 IOUT	Salida por el display de la corriente -lectura-
0x04 POTENCIA	Salida por el display de la potencia -lectura-
0x05 UIN	valor de indicación de tensión de entrada -lectura-
0x06 LOCK	bloqueo -lectura/escritura-
0x07 PROTECT Estado de la Protección -lectura-
0x08 CV/CC	Constante Voltaje / corriente -lectura-
0x09 ON/OFF	Estado del interruptor -lectura/escritura-

0x0A B_LED	nivel de brillo de la retro iluminación -lectura/escritura-
0x0B MODELO	Modelo del Producto -lectura-
0x0C VERSON	firmware Versión -lectura-

0x23 EXTRACT_M	Acceso directo para mostrar el conjunto de datos requerido -escritura-

0x24 Auto_Test 5 secuencias de autotest de tensión e intensidad con pausas de un periodo de 100ms (mínimo 1segundo)
0x4F Auto_Test pausa
0x44 escaneo tensión, corriente de salida, tensión de marcha, tensión pausa, paso de 2, pausas de un periodo de 100ms (mínimo 1segundo)
0x43 Escaneo de tensión indicación pausa
0x4a escaneo intensidad, salida, corriente de inicio, corriente con pausas de un periodo de 100ms (mínimo 1segundo)
0x49 Escaneo de corriente indicación pausa

Grupo de memorias 0x50 + (memoria*16)
0x50 U-SET	ajustes de voltaje -lectura/escritura-
0x51 I-SET	ajuste de la corriente -lectura/escritura-
0x52 S-OVP	valor de protección de la sobre tensión -lectura/escritura-
0x53 S-OCP	Sobre corriente valor de protección -lectura/escritura-
0x54 S-OPP	Protección frente sobe potencia -lectura/escritura-
0x55 B-LED	los niveles de brillo de la retro iluminación -lectura/escritura-
0x56 M-PRE	Número de La memoria preset -lectura/escritura-
0x57 S-INI	interruptor de potencia de salida -lectura/escritura-  
******************************************************************************************************
Creative Commons License Disclaimer

UNLESS OTHERWISE MUTUALLY AGREED TO BY THE PARTIES IN WRITING, LICENSOR OFFERS THE WORK AS-IS
AND MAKES NO REPRESENTATIONS OR WARRANTIES OF ANY KIND CONCERNING THE WORK, EXPRESS, IMPLIED,
STATUTORY OR OTHERWISE, INCLUDING, WITHOUT LIMITATION, WARRANTIES OF TITLE, MERCHANTIBILITY,
FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT, OR THE ABSENCE OF LATENT OR OTHER DEFECTS,
ACCURACY, OR THE PRESENCE OF ABSENCE OF ERRORS, WHETHER OR NOT DISCOVERABLE. SOME JURISDICTIONS
DO NOT ALLOW THE EXCLUSION OF IMPLIED WARRANTIES, SO SUCH EXCLUSION MAY NOT APPLY TO YOU.
EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGES.

http://creativecommons.org/licenses/by-sa/3.0/

  Author: Radioelf  http://radioelf.blogspot.com.es/
*/
#ifndef DPS_H_
#define DPS_H_

#define ON_DPS1  7
#define ON_DPS2  8

#define Read_HoldingRegisters 0x03                            // Modbus función 0x03 petición leer uno o más registros
#define Write_SingleRegister 0x06                             // Modbus función 0x06 escribir en un registro
#define Write_MultipleRegisters 0x10                          // Modbus función 0x10 escribir múltiples registros

uint8_t esclavo =1, funcion =0x03, longitud =1, status_RX_DPS =0, byte_datos_RX[80] ={0}, byte_datos_TX[40] ={0};   // buffer máximo para la RX y TX de datos de un byte
uint16_t datos_RX_DPS[40] ={0}, datos_TX[32] ={0};            // buffer máximo para la RX y TX de datos de dos byte

// Calculo CRC
uint16_t calcCRC(uint8_t longitud, bool r_w){
uint16_t crc = 0xFFFF, x, flag;
  for (unsigned char i =0; i <=longitud; i++){
    if (r_w)
      crc = crc ^ byte_datos_TX[i];                           // TX
    else
      crc = crc ^ byte_datos_RX[i];                           // RX
    for (unsigned char z =1; z <= 8; z++){
      flag = crc & 0x0001;
      crc >>=1;
      if (flag) crc ^= 0xA001;
    }
  }
  // invertimos
  x = crc >> 8;
  crc = (crc << 8) | x;
  crc &= 0xFFFF;
  return crc;
}
// Escribir en registros, funciones 0x06 o 0x10
void write_Register(uint8_t esclavo, uint8_t funcion, uint8_t registro, uint8_t palabra){
uint8_t i, data_lon =6;
uint16_t crc;
  byte_datos_TX[0] =esclavo; 
  byte_datos_TX[1] =funcion;                                  // función 
  byte_datos_TX[2] =0x00;                                     // registro formado por 2 bytes
  byte_datos_TX[3] =registro;                                 // registro de 0 a 255-> 0x00 0x00-ff  
  if (funcion ==0x10){                                        // funcion 0x10 varios registros
    byte_datos_TX[4] =0x00;                                   // palabra formada por 2 bytes
    byte_datos_TX[5] =palabra;                                // palabra de 0 a 255-> 0x00 0x00-ff   
    byte_datos_TX[6] =palabra*2;                              // número de bytes de datos      
      data_lon =7;
      uint8_t  palabras =palabra+1;   
      for (i =1; i <palabras; i++) {
        byte_datos_TX[data_lon++] =highByte (datos_TX[i]);    // cada palabra contiene 2 bytes (int16)
        byte_datos_TX[data_lon++] =lowByte (datos_TX[i]);
      }
  }else{                                                      // funcion 0x06 un registro
     byte_datos_TX[4] =highByte (datos_TX[0]);                // valor a escribir
     byte_datos_TX[5] =lowByte (datos_TX[0]);
  }
  crc =calcCRC(data_lon-1, 1);
  byte_datos_TX[data_lon++] =highByte (crc);                  // CRC contiene 2 bytes (int16)
  byte_datos_TX[data_lon] =lowByte (crc);
  for (i =0; i <=data_lon; i++){
    Serial.write(byte_datos_TX[i]);
  }
  Serial.flush();                                             // esperamos que finalice la transmisión de todos los datos serie
  delay (3);                                                  // >2ms, (3.5 character times)
}
// Petición leer registros 0x03
void read_Register(uint8_t esclavo, uint8_t registro, uint8_t palabras){
uint8_t i;
uint16_t crc;
  byte_datos_TX[0] =esclavo; 
  byte_datos_TX[1] =Read_HoldingRegisters;                    // funcion 0x03
  byte_datos_TX[2] =0x00;                                     // registro formado por 2 bytes
  byte_datos_TX[3] =registro;                                 // registro de 0 a 255-> 0x00 0x00-ff  
  byte_datos_TX[4] =0x00;                                     // palabra formado por 2 bytes
  byte_datos_TX[5] =palabras;                                 // palabra de 0 a 255-> 0x00 0x00-ff   
  crc =calcCRC(5, 1);
  byte_datos_TX[6] =highByte (crc);                           // CRC contiene 2 bytes (int16)
  byte_datos_TX[7] =lowByte (crc);
  for (i =0; i <=7; i++){
    Serial.write(byte_datos_TX[i]);
  }
  Serial.flush();
  delay (3);                                                  // >2ms, (3.5 character times)
}
// Recepción de datos de los nodos, indicamos en la variable status_RX_DPS a 0 si no tenemos datos, >199 si tenemos errores en la recepción y 2 si tenemos datos
void RX_serial_DPS(bool clear_data){
uint8_t buffer_rx =0, x =8, y=0;
uint16_t crc, Temp;
uint32_t StartTime = millis();
  status_RX_DPS =0;
  while (x && !status_RX_DPS){
    if (Serial.available()){
      byte_datos_RX[buffer_rx++] =Serial.read();
      x--;
      if (buffer_rx ==5){
        if ((byte_datos_RX[0] ==1 || byte_datos_RX[0] ==2) && (byte_datos_RX[1] ==0x03 || byte_datos_RX[1] ==0x06 || byte_datos_RX[1] ==0x10)){  // comprobamos que es una dirección y función valida
          if (byte_datos_RX[1] ==0x03){                        // obtenemos los bytes que nos faltan por recibir
            x =(byte_datos_RX[2]);
          }else{
            x =3;                                              // número de bytes que faltan por recibir 4 bytes, palabra dirección inicio y registro 0 dato 
          }
        }else{
          status_RX_DPS =200;                                  // error datos
        }
      }
    }   
    if ((millis() - StartTime) > 2000){                        //-+ 475bytes + 2.0 ms (3.5 character times)
      status_RX_DPS =210;                                      // error time
    }
  }
  buffer_rx =buffer_rx-1;

  if (!status_RX_DPS){                                         // si no tenemos errores
    if (clear_data){                                           // recepción de datos sin gestión
      status_RX_DPS =3;
      return;
    }
    crc =calcCRC((buffer_rx-2), 0);                            // menos los dos bytes del crc
    if(highByte (crc) == byte_datos_RX[buffer_rx-1] && lowByte (crc) == byte_datos_RX[buffer_rx]){
      datos_RX_DPS[0] =byte_datos_RX[0];                       // ID del nodo
      datos_RX_DPS[1] =byte_datos_RX[1];                       // función, 0x03, 0x06 o 0x10
      if (byte_datos_RX[1] ==0x03){                            // si es funcion 0x03
        datos_RX_DPS[2] =byte_datos_RX[2];
        y =3;                                                  // posición primera palabra
      }
      else{                                                    // si es funcion 0x06 o 0x10
        Temp =(int) byte_datos_RX[2]<<8;    
        Temp =(int) Temp + byte_datos_RX[3]; 
        datos_RX_DPS[2] =Temp;                                  // dirección del registro
        y=4;                                                    // posición primera palabra
      }
      for (byte i =y; i <=(datos_RX_DPS[2]+2); i++){            // resto de palabras
        Temp =(int) byte_datos_RX[i]<<8;    
        i++;
        Temp =(int) Temp + byte_datos_RX[i]; 
        datos_RX_DPS[y] =Temp;  
        y++;
      }
      status_RX_DPS =y;                                         // OK >3 y <100   
    }else 
      status_RX_DPS =230;                                       // error  CRC
  }
}
//**************************************************************************************************
// Seleccionamos a que fuente se le enviarán y se recibirán datos a través del 74HC126
void selec_esclavo(uint8_t id){
    digitalWrite(ON_DPS2, LOW);
    digitalWrite(ON_DPS1, LOW);
    esclavo =id;
    delayMicroseconds(1);
    if (esclavo ==1){
      digitalWrite(ON_DPS1, HIGH);
    }else{
      digitalWrite(ON_DPS2, HIGH);
    }
    delayMicroseconds(1);
}

// 0x06 leer o escribir una palabra en el registro indicado
bool reg_DPS(uint8_t id, bool r_w, uint8_t registro_DPS, uint16_t valor){
uint16_t x;
  selec_esclavo(id);
  if (r_w){                                                        // escribir en un registro 
    datos_TX[0] =valor;
    write_Register(id, Write_SingleRegister, registro_DPS, 1);     // dirección, funcion escribir un registro 0x06, una palabra
    return 1;
  }
  else{ 
    read_Register(id, registro_DPS, 1);                            // dirección, funcion leer un registro 0x03, una palabra
    return 1;
  }
  return 0;
}

// 0x03 leer el número de palabras indicadas a partir del registro indicando 
void read_DPS(uint8_t id, uint8_t registro_read, uint8_t palabras){
uint8_t y;
  selec_esclavo(id);
  read_Register(id, registro_read, palabras);   
}

// 0x10 escribir el número de datos indicados en la posición inicial del registro indicado
void wire_DPS(uint8_t id, uint8_t registro_wire, uint8_t palabras){
uint8_t y;
  selec_esclavo(id);
  write_Register(id, Write_MultipleRegisters, registro_wire, palabras); 
}

#endif
