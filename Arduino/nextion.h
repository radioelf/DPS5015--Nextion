/*
 * 
 * https://www.itead.cc/wiki/Nextion_HMI_Solution
 * 
 Nota:
 Todas las instrucciones y parámetros se envían a la pantalla nextion en ascii
 y minúsculas y la instrucción finaliza con tres bytes "0xff 0xff 0xff"
 *
 desde nextion a arduino:
 pagina 0 Principal->245->0xF5 
 pagina 1 opciones ->246->0xF6
 pagina 2 config   ->247->0xF7
 pagina 3 test     ->248->0xF8
 pagina 4 gráfica 1->249->0xF9
 pagina 4 gráfica 2->250->0xFA
 pagina 5 memorias ->251->0xFB
 pagina 6 teclado  ->252->0xFC
 
 Registros:
 0x00 + 1-5 bytes          U-set fuente 1
 0x01 + 1-5 bytes          I-set fuente 1
 0x02 + 1-5 bytes          Uout fuente 1
 0x03 + 1-5 bytes          Iout fuente 1
 0x04 + 1-5 bytes          Wout fuente 1
 0x05 + 1-5 bytes          Uint fuente 1
 0x06 + 1 byte             LOCK fuente 1 (1->bloqueada)
 0x07 + 1 byte             PROTECT fuente 1 (0->OK, 1->OVP, 2->OCP y 3->OPP)
 0x08 + 1 byte             CV/CC fuente 1
 0x09 + 1 byte             fuente 1 ON/OFF (1 on)
 0x0A + 1 byte             fuente 1 retro iluminación (0-4)
 0x0B + 2 byte             modelo fuente 1
 0x0C + 2 byte             versión fuente 1
 0x23 + 1 byte             EXTRACT_M fuente 1 (número de bloque 0-9)
 0x24                      Auto_Test fuente 1 RUN
 0x4F                      Auto_Test fuente 1 pausa       
 0x44                      escaneo tensión fuente 1 RUN
 0x43                      escaneo tensión fuente 1 pausa
 0x4A                      escaneo intensidad fuente 1 RUN
 0x49                      escaneo intensidad fuente 1 pausa                

 0x50-0x57 grupo de memoria 0 fuente 1
 0x60-0x67 grupo de memoria 1 fuente 1
 0x70-0x77 grupo de memoria 2 fuente 1
 0x80-0x87 grupo de memoria 3 fuente 1
 0x90-0x97 grupo de memoria 4 fuente 1
 0xA0-0xA7 grupo de memoria 5 fuente 1
 0xB0-0xB7 grupo de memoria 6 fuente 1
 0xC0-0xC7 grupo de memoria 7 fuente 1
 0xD0-0xD7 grupo de memoria 8 fuente 1
 0xE0-0xE7 grupo de memoria 9 fuente 1
 
 0x10 + 1-5 bytes          U-set fuente 2
 0x11 + 1-5 bytes          I-set fuente 2
 0x12 + 1-5 bytes          Uout fuente 2
 0x13 + 1-5 bytes          Iout fuente 2
 0x14 + 1-5 bytes          Wout fuente 2
 0x15 + 1-5 bytes          Uint fuente 2
 0x16 + 1 byte             LOCK fuente 2 (1->bloqueada)
 0x17 + 1 byte             PROTECT fuente 2 (0->OK, 1->OVP, 2->OCP y 3->OPP)
 0x18 + 1 byte             CV/CC fuente 2
 0x19 + 1 byte             fuente 2 ON/OFF 1 (on)
 0x1A + 1 byte             fuente 2 retro iluminación (0-4)
 0x1B + 2 byte             modelo fuente 2
 0x1C + 2 byte             versión fuente 2
 0x33 + 1 byte             EXTRACT_M fuente 2 (número de bloque 0-9)
 0x34                      Auto_Test fuente 2
 0x4E                      Auto_Test fuente 2 pausa 
 0x45                      escaneo tensión fuente 2
 0x42                      escaneo tensión fuente 2 pausa
 0x4B                      escaneo intensidad fuente 2
 0x48                      escaneo intensidad fuente 2 pausa 
 
 0x58-0x5F grupo de memoria 0 fuente 2
 0x68-0x6F grupo de memoria 1 fuente 2
 0x78-0x7F grupo de memoria 2 fuente 2
 0x88-0x8F grupo de memoria 3 fuente 2
 0x98-0x9F grupo de memoria 4 fuente 2 
 0xA8-0xAF grupo de memoria 5 fuente 2
 0xB8-0xBF grupo de memoria 6 fuente 2
 0xC8-0xCF grupo de memoria 7 fuente 2
 0xD8-0xDF grupo de memoria 8 fuente 2
 0xE8-0xEF grupo de memoria 9 fuente 2
 
Opciones:
 0x0D selección encoder + valor V-set fuente 1
 0x1F selección encoder + estado pulsador bto
 0x20 fin encoder
 0x25 selección encoder + valor V-set fuente 2
 0x2A selección encoder + estado pulsador bt1

 0x40 error en escaneo o test
 0xF0 zumbador on/off
 0xF1 encoder on/off
 0xF2 nº pasos por pulso encoder 
 
 0xFF fin de trama

NO usados:
0x21, 0x22, 0x26, 0x27, 0x28, 0x29, 0x2B, 0x2C, 0x2D, 0x2E,
0x2F, 0x30, 0x31, 0x32, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41,
0x44, 0x46, 0x47, 0x4A, 0x4C, 0x4D, 0xEE, 0XEF, 0XF3, 0XF3,
0XF4, 0XF5, 0XF6, 0XF7, 0XF8, 0XF9, 0XFD, 0XFE. 
 
***********************************************************************************************

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
#ifndef NEXTION_H
#define NEXTION_H

#include <SoftwareSerial.h>                                                       // https://www.arduino.cc/en/Reference/SoftwareSerial (57600 bps max. arduino nano)

SoftwareSerial nextionSerial(10, 11);                                             // pines para la comunicación serie con la pantalla nextion RX->10, TX->11

bool on_off_buzzer =1, on_off_encoder =1, on_off_run_1 =0, on_off_run_2 =0,
     run1_encoder =0, run2_encoder =0, pulsador_encoder =0, fuente1_run =0, fuente2_run =0;
uint8_t registro=0, pantalla, z, pasos_enc =5, ciclo =100, 
        digito[5]={0}, selec_pulsador =0, grupo_memoria =0x50, byte_rx_nextion[76]={0}; // máximo 76 bytes
volatile uint8_t estado_encoder =0;
int16_t data_encoder =0;

// Pasamos los dígitos recibidos a una palabra int para enviar  a las fuentes
uint16_t dig_palabra(uint8_t digitos){
  switch (digitos){
    case 4:
      return ((digito[1]*1000) + (digito[2]*100) + (digito[3]*10) + (digito[4]));
    case 3:
      return ((digito[1]*100) + (digito[2]*10) + (digito[3]));
    case 2:
      return ((digito[1]*10) + (digito[2]));
    default:
      return (digito[1]);
  }   
}
// Obtenemos el registro y el valor enviado por la pantalla nextion
uint8_t nextion_rx (bool data_clear){
uint8_t y, Nom_datos =0, digitos =0, palabras =1, x =0x4f, z =0x58, byte_rx =0; 
uint32_t RX_Time =millis();
    byte_rx =nextionSerial.available();                                         // obtenemos el numero de byte en el buffer de RX y se borra la indicación de datos en el buffer
    if (!byte_rx) return 0;                                                     // si no tenemos datos salimos
    do{
      if (data_clear){
          char clear_byte = nextionSerial.read();
      }else{
        byte_rx_nextion [Nom_datos++] =nextionSerial.read();                    // obtenemos las datos del buffer
      }
      byte_rx--;      
    }while (byte_rx !=0);
    if (data_clear) return 0;
    if (byte_rx_nextion[Nom_datos-1] !=0xff){                                   // comprobamos si recibimos indicador fin trama
      do{
      if (nextionSerial.available()) byte_rx_nextion [Nom_datos++] =nextionSerial.read();// obtenemos el dato cuando lo tenemos en el buffer
      if ((millis() - RX_Time) > 500){
        nextionSerial.overflow();                                               // borra la bandera de desbordamiento
        return 0;                                                               // si no tenemos fin trama 0xff y espiro el tiempo (86.8us 10 bits)
      }
      }while (byte_rx_nextion [Nom_datos-1] !=0xff);
    }
    // comprobamos si el penúltimo y antepenúltimo byte recibido fue 0xff (trama de estado pantalla-> estado 0xff 0xff 0xff)
    //if (byte_rx_nextion[Nom_datos-2] ==0xff && byte_rx_nextion[Nom_datos-3] ==0xff);
      //return 0;
    byte_rx =Nom_datos-1;                                                       // obtenemos los bytes recibidos
    registro =byte_rx_nextion[0];
    if ((registro >244 && registro <253)){                                      // comprobamos si es un acceso a una pagina (pagina 0->245, pagina 6->252)
      pantalla = registro-245;
      registro =255;                                                            // No registro, SI pagina
      return 0; 
    }
    if (byte_rx ==2){                                                           // si el valor es de un solo byte + registro y fin trama 
      datos_TX[1] =int (byte_rx_nextion[1]); 
    }else{
    // gestionamos la recepción de dígitos para genera la palabra despuÃ©s del registro
      for (y =1; y <byte_rx; y++){                                              // recorremos todos los bytes recibidos (a excepción del registro)                                                            
        if (byte_rx_nextion[y] >0x2f && byte_rx_nextion[y] <0x3A){              // comprobamos valores correctos, de 0 a 9 (HEX)
          digito[++digitos] =byte_rx_nextion[y]-48;                             // obtenemos el número de dígitos recibidos
        }else{                                                                  // valores de punto decimal '.' o separado de palabra '-' o fin de trama 0xff
          if (byte_rx_nextion[y] ==0x2D ){                                      // comprobamos si es un separador de palabra 
            datos_TX[palabras++] =dig_palabra(digitos--);                       // convertimos dígitos en palabras y obtenemos el número de palabra 
            digitos =0;
          }
        }
      }
      datos_TX[palabras] =dig_palabra(digitos);
    } 
    // la duración de la gestión es de unos 500us para 1 byte a unos 14ms para 76 bytes
    esclavo =1;
    if (registro <0x10){                                                        // comprobamos si son registros de la fuente 1 0x00-0x0f
       if (registro ==0x0d) data_encoder  =datos_TX[1];                         // la recepción la pasamos a la variable de trabajo del encoder
      return palabras;
    }
    if (registro ==0x20){                                                       // comprobamos si es el fin del modo encoder
      selec_pulsador =0;
      EIMSK = 0b00000000;                                                       // des-habilitamos interrupciones int0 y int1
      return palabras;
    }
    if (registro ==0x23 || registro ==0x33){                                    // extracción de grupo de memoria
      esclavo =1;
      if (registro ==0x33){
        registro =0x23;
        esclavo =2;
      } 
      grupo_memoria =0x50+(16*datos_TX[1]);
    }
    if ((registro >0x0f && registro <0x1d) || (registro ==0x33) || (registro ==0x34)){ // comprobamos si son registros de la fuente 2 (datos 0x10-0x1c, acceso memoria 0x33 o auto test 0x34)
      esclavo =2;
      registro =registro -16;                                                   // obtenemos el valor del registro
      return palabras; 
    }
    if((registro ==0x25 || registro ==0x2a)){                                   // comprobamos si son opciones de la fuente 2 (encoder 0x25, 0x2a)
      esclavo =2;
      if (registro ==0x25) data_encoder  =datos_TX[1];                          // la recepción la pasamos a la variable de trabajo del encoder
      return palabras;
    }
    if ((registro ==0x45) || (registro ==0x4b)){                                // comprobamos si son registros de las fuente 2 (escaneo 0x45, 0x4b)
      esclavo =2;
      registro =registro-1;                                                     // obtenemos el valor del registro
      return palabras; 
    }
    if ((registro ==0x4e) || (registro ==0x42) || (registro ==0x48)){           // comprobamos si son registros de las fuente 2 (STOP test o escaneo)
      esclavo =2;
      registro =registro+1;                                                     // obtenemos el valor del registro
      return palabras; 
    }

    for (y =0; y <10; y++){                                                     // grupos de memorias para fuente 1 (de 0 a 9)
      if((registro >x && registro <z)){                                         // x =0x4f, z =0x58
        esclavo =1;
        grupo_memoria =0x50+(16*y);
        return palabras; 
      }
      x +=16;
      z +=16;
    }
    x =0x57;
    z =0x60;
    for (y =0; y <10; y++){                                                     // grupos de memorias para fuente 2 (de 0 a 9)
      if((registro >x && registro <z)){ 
        esclavo =2;
        registro =registro -8;                                                  // obtenemos el valor del registro
        grupo_memoria =0x50+(16*y);                                            
        return palabras; 
      }
      x +=16;
      z +=16;
    }
    return palabras;   
}
// Comandos fin trama
void end_nextion(){
    nextionSerial.write(0xff);
    nextionSerial.write(0xff);
    nextionSerial.write(0xff);
}
// Instrucción inicializar sin uso de ack
void ini_nextion(){
    nextionSerial.print("");
    end_nextion();
    nextionSerial.print("bkcmd=0");                                             // no usamos ACK
    end_nextion();
    nextionSerial.print("page 0");                                             // pagina inicio
    end_nextion();
}
// Instrucción reset
void reset_nextion(){
    nextionSerial.print("rest");
    end_nextion();
    delay(250);
    ini_nextion();
}
// Instrucción retroiluminación a 1
void dim_nextion(){
    nextionSerial.print("dim=1");
    end_nextion();
}
// Instrucción reposo
void sleep_nextion(bool on_off){
    nextionSerial.write("sleep=");
    if (on_off)
      nextionSerial.write("1");
    else
      nextionSerial.write("0");
    end_nextion();
}
// Envió comando
void cmd_nextion(const char *cmd){
    nextionSerial.print(cmd);
    end_nextion();
}
// Envió secuencia texto
void txt_nextion(const char* num,  const char* valor){
    nextionSerial.write("t");
    nextionSerial.write(num);
    nextionSerial.write(".txt=");
    nextionSerial.write(0x22);                                                  // comillas
    nextionSerial.write (valor);
    nextionSerial.write(0x22);                                                  // comillas
    end_nextion();
}
// Envió de un valor
void num_nextion(const char* num,  uint16_t valor, uint8_t tipo, bool opcion){
int  dig = valor;
  switch (tipo){
  case 0:
    nextionSerial.write("va");
  break;
  case 1:
     nextionSerial.write("p");
  break;
  case 2:
    nextionSerial.write("r");
  break;
  case 3:
    nextionSerial.write("bt");
  break;
  case 4:
    nextionSerial.write("c");
  break;
  case 5:
    nextionSerial.write("n");
  break;
  }
  nextionSerial.write(num);
  if (opcion)
    nextionSerial.write(".pic=");
  else
    nextionSerial.write(".val=");
  if (valor >9999){
      nextionSerial.write((dig/10000)+0x30);                                    // cinco dígitos
      dig =dig%10000;
    }
    if (valor >999){  
      nextionSerial.write((dig/1000)+0x30);                                     // cuatro dígitos
      dig =dig%1000;
    }
    if (valor >99){
      nextionSerial.write((dig/100)+0x30);                                      // tres dígitos
      dig =dig%100;
    }
    if (valor >9) nextionSerial.write((dig/10)+0x30);                           // dos dígitos
      nextionSerial.write((dig%10)+0x30);                                       // un dígito
    end_nextion();
}
// Envió comando wave
void wave_nextion_tx(uint8_t dps){
uint8_t x =0, z, v;
  if (dps ==1){
    read_DPS(1, 2, 3);                                                          // ID 1, leemos 3 palabras comenzando en el registro 2 (UOUT, IOUT, POUT)
     
  }else{
    read_DPS(2, 2, 3);                                                          // ID 2, leemos 3 palabras comenzando el el registro 2 (UOUT, IOUT, POUT)    
  }
  do{
    RX_serial_DPS(0);                                                           // recepción de la respuesta y gestión
  }while (status_RX_DPS <2);  
  while (x!=3){
    if (x ==0){
      v =datos_RX_DPS[3]/100;
      z =183 + round(v*1.5);                                                    // ajustamos para posición de la gráfica tensión de 184->1V a 255->50V
      num_nextion("10", v, 5, 0);                                               // n10.val=v
    }
    if (x ==1){
      v =datos_RX_DPS[4]/100;
      z =93 + round(v*6.0);                                                     // ajustamos para posición de la gráfica intensidad de 93->0A a 183->15A
      num_nextion("11", v, 5, 0);                                               // n11.val=v
    }
    if (x ==2){
      v =datos_RX_DPS[5]/100;                                
      z =round(v*0.124);                                                        // ajustamos para posición de la gráfica potencia de 0->0W a 93->750W 
      num_nextion("12", v, 5, 0);                                               // n12.val=v
    }  
    nextionSerial.write("add 1,");                                              // id
    nextionSerial.write(x+48);                                                  // canal
    nextionSerial.write(",");
    if (z >99){                                                                 // tres dígitos 
      nextionSerial.write((z/100)+48);                                          
      uint8_t dig =z%100;
      nextionSerial.write((dig/10)+48); 
      nextionSerial.write((dig%10)+48);
    }else{
      if (z >9){
        nextionSerial.write((z/10)+48);
        nextionSerial.write((z%10)+48);
      }else
        nextionSerial.write(z+48);
    }
    end_nextion();
    x++;
  }
}
// Envió de los datos correctos según pagina en nextion
// datos_RX_DPS[0] ID, datos_RX_DPS[1] función, datos_RX_DPS[2] función 0x03 nº bytes, función 0x06 registro
// inicio primer dato en posición 3:
// datos_RX_DPS[3] =V-set, datos_RX_DPS[4] =I-set, datos_RX_DPS[5] =V-out, datos_RX_DPS[6] = I-out, datos_RX_DPS[7] =W-out, datos_RX_DPS[8] = V-int
// datos_RX_DPS[9] =bloqueo, datos_RX_DPS[10] = protección, datos_RX_DPS[11] =CV/CC, datos_RX_DPS[12] = ON/OFF, datos_RX_DPS[13] = brillo,
// datos_RX_DPS[14] =modelo, datos_RX_DPS[15] =versión, datos_RX_DPS[16] =on/off test, datos_RX_DPS[17] =on/off escaneo tensión, datos_RX_DPS[18] =on/off escaneo corriente
void data_nextion_TX(uint8_t dps, bool mem_data){
  switch (pantalla){
    case 0:                                                                     // pantalla principal
      if (dps ==1){ 
        if (mem_data ==1){                                                      // si es una actualización de memorias  S-OVP, s-OCP Y S-OPP
          num_nextion("8", datos_RX_DPS[3], 0, 0);                              // enviamos ajuste limite tensión fuente 1 OVP a pantalla nextion
          num_nextion("9", datos_RX_DPS[4], 0, 0);                              // enviamos ajuste limite intensidad fuente 1 OCP a pantalla nextion
          num_nextion("10", datos_RX_DPS[5], 0, 0);                             // enviamos ajuste limite potencia fuente 1 OPP a pantalla nextion
        }else{
          on_off_run_1 =!on_off_run_1;
          num_nextion("0", on_off_run_1, 2, 0);                                 // OK fuente 1 r0.val=on_off
          if (datos_RX_DPS[12]){                                                // comprobamos si tenemos la fuente 1 en marcha
            fuente1_run =1;
            num_nextion("0", datos_RX_DPS[5], 0, 0);                            // enviamos tensión fuente 1 UOUT a pantalla nextion va0.val=datos_RX_DPS[5]
            num_nextion("1", datos_RX_DPS[6], 0, 0);                            // enviamos tensión fuente 1 IOUT a pantalla nextion va1.val=datos_RX_DPS[6]
            if (datos_RX_DPS[10]){                                              // estado de protección cuando la fuente se encuentra en marcha
              num_nextion("14", datos_RX_DPS[10], 0, 0);                        // indicación razón del bloqueo en imagen conector fuente 1, va14.val=1-3
            }else{
              num_nextion("14", datos_RX_DPS[11], 0, 0);                        // indicación CV o CC, va14.val=4-5     
            }                                                  
          }else{
            fuente1_run =0;
            num_nextion("0", datos_RX_DPS[3], 0, 0);                            // enviamos V-set fuente 1 va0.val=datos_RX_DPS[3]
            num_nextion("1", datos_RX_DPS[4], 0, 0);                            // enviamos I-set fuente 1 va1.val=datos_RX_DPS[4]
            num_nextion("14", 0, 0, 0);                                         // indicación OK, va14.val=0   
          }

          num_nextion("2", datos_RX_DPS[7], 0, 0);                              // enviamos potencia fuente 1 POUT a pantalla nextion
          num_nextion("6", datos_RX_DPS[8], 0, 0);                              // enviamos tensión entrada fuente 1 UIN a pantalla nextion
          num_nextion("0", datos_RX_DPS[12], 3, 0);                             // enviamos estado de salida ON/OFF de fuente 1, bt0.val=0-1
        }
      }else{
        if (mem_data ==1){                                                      // si es una actualización de memorias S-OVP, s-OCP Y S-OPP
          num_nextion("11", datos_RX_DPS[3], 0, 0);                             // enviamos ajuste limite tensión fuente 2 OVP a pantalla nextion
          num_nextion("12", datos_RX_DPS[4], 0, 0);                             // enviamos ajuste limite intensidad fuente 2 OCP a pantalla nextion
          num_nextion("13", datos_RX_DPS[5], 0, 0);                             // enviamos ajuste limite potencia fuente 2 OPP a pantalla nextion   
        }else{
          on_off_run_2 =!on_off_run_2;
          num_nextion("1", on_off_run_2, 2, 0);                                 // OK fuente 2 r1.val=on_off
          if (datos_RX_DPS[12]){                                                // comprobamos si tenemos la fuente 2 en marcha
            fuente2_run =1;
            num_nextion("3", datos_RX_DPS[5], 0, 0);                            // enviamos tensión fuente 2 UOUT a pantalla nextion
            num_nextion("4", datos_RX_DPS[6], 0, 0);                            // enviamos tensión fuente 2 IOUT a pantalla nextion
            if (datos_RX_DPS[10]){
              num_nextion("15", datos_RX_DPS[10], 0, 0);                        // indicación bloqueo por protección en imagen conector fuente 2, va15.val=1-3
            }else{
              num_nextion("15", datos_RX_DPS[11], 0, 0);                        // indicación CV o CC, fuente 2, va14.val=4-5     
            }      
          }else{
            fuente2_run =0;
            num_nextion("3", datos_RX_DPS[3], 0, 0);                            // enviamos V-set fuente 2 
            num_nextion("4", datos_RX_DPS[4], 0, 0);                            // enviamos I-set fuente 2
            num_nextion("15", 0, 0, 0);                                         // indicación OK, va15.val=0, fuente 2
          }
          num_nextion("5", datos_RX_DPS[7], 0, 0);                              // enviamos potencia fuente 2 POUT a pantalla nextion
          num_nextion("7", datos_RX_DPS[8], 0, 0);                              // enviamos tensión entrada fuente 2 UIN a pantalla nextion
          num_nextion("1", datos_RX_DPS[12], 3, 0);                             // enviamos estado de salida ON/OFF de fuente 2, bt1.val=0-1
        }
      }
    break;
    case 2:                                                                     // pantalla config
      if (dps ==1){
        num_nextion("0", datos_RX_DPS[3], 0, 0);                                // va0.val=modelo fuente 1
        num_nextion("2", datos_RX_DPS[4], 0, 0);                                // va2.val=firmware fuente 1
        num_nextion("0", on_off_buzzer, 4, 0);                                  // c0.val= on/off,  enviamos opción zumbador
        num_nextion("1", on_off_encoder, 4, 0);                                 // c1.val= on/off, enviamos opción encoder
        num_nextion("1", pasos_enc, 5, 0);                                      // n1.val= paso para el encoder
      }else{
        num_nextion("1", datos_RX_DPS[3], 0, 0);                                // va1.val=modelo fuente 2
        num_nextion("3", datos_RX_DPS[4], 0, 0);                                // va3.val=firmware fuente 2
      }
    break;
    case 3:                                                                     // pantalla test - escaneo
      if (mem_data ==0){                                                        // si estamos en escaneo
        num_nextion("5", datos_RX_DPS[3], 0, 0);                                // va5.val=U-out, primera palabra
        num_nextion("6", datos_RX_DPS[4], 0, 0);                                // va6.val=I-out, segunda plabra 
      }else{                                                                    // si estamos en test
        num_nextion("7", datos_RX_DPS[3], 0, 0);                                // va7.val=U-out, primera palabra
        num_nextion("8", datos_RX_DPS[4], 0, 0);                                // va8.val=I-out, segunda plabra
      }
      num_nextion("11", datos_RX_DPS[8], 0, 0);                                 // va11.val=protección->0->OK, 1->OVP, 2->OCP y 3->OPP
      num_nextion("13", datos_RX_DPS[9], 0, 0);                                 // va13.val=0->CC, 1->CV
    break;
    case 6:                                                                     // pantalla memorias
      num_nextion("m0", datos_RX_DPS[3], 0, 0);                                 // vam0.val=U-set
      num_nextion("m1", datos_RX_DPS[4], 0, 0);                                 // vam1.val=I-set
      num_nextion("m2", datos_RX_DPS[5], 0, 0);                                 // vam2.val=S-ovp
      num_nextion("m3", datos_RX_DPS[6], 0, 0);                                 // vam3.val=S-ocp
      num_nextion("m4", datos_RX_DPS[7], 0, 0);                                 // vam4.val=S-opp
      num_nextion("0", datos_RX_DPS[10], 4, 0);                                 // c0.val=S-ini
      //va6.val=x  1->ok memorias, 2->error, 10->OK leido bloque memorias
      //num_nextion("m6", 10, 0, 0);                                            // vam6.val=10
    break;
  }
}
#endif  /* NEXTION_H */
