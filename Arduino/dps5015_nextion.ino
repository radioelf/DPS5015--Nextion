/*
Programa para el control de dos fuentes DPS5015 a traves de pantalla nextion


Mod. ~/.arduino15/packages/arduino/hardware/avr/1.6.21/cores/arduino/HardwareSerial.h
#define SERIAL_RX_BUFFER_SIZE 64
por
#define SERIAL_RX_BUFFER_SIZE 128
Mod. ~/.arduino15/packages/arduino/hardware/avr/1.6.21/libraries/SoftwareSerial/src/SoftwareSerial.h
#define _SS_MAX_RX_BUFF 64
por 
#define _SS_MAX_RX_BUFF 128

configurar nextion 62 61 75 64 73 3d 35 37 36 30 30 ff ff ff  ->bauds=57600ÿÿÿ

*************************************************************************************************
*Creative Commons License Disclaimer

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

  13-5-18

*/
#include "dps.h"
#include "nextion.h"                                                       
#include <TimerOne.h>
#include <EEPROM.h>

#define ENCODE_A 2
#define ENCODE_B 3
#define PULSADOR_PIN 4                                                      // pin pulsador encoder
#define LED_F1_PIN 5
#define BUZZER_PIN 6                 
//#define ON_DPS1  7                                                        // pin control TX/RX fuente 1 (74HC126)
//#define ON_DPS2  8                                                        // pin control TX/RX fuente 2 (74HC126)
#define ON_encoder  9                                                       // pin habilitación encoder (habilitado->0)
//RX->10 nextion
//TX->11 nextion
#define ON_5_3v3 12
#define LED_PIN 13                                                          // led pcb nano
#define USB_PC_PIN A0                                                       // pin 14 indica selección control por PC A0 (USB)
#define LED_F2_PIN 15

bool pc_control =0;
uint32_t iniTime;

void setup(){
  nextionSerial.begin(57600);                                               // puerto 1 serie nextion a 57600Bps, 128 bytes buffer (Bps máximo para arduino nano)
  Serial.begin(19200);                                                      // puerto 0 serie Modbus a 19200Bps, 128 bytes buffer, 2.0 ms (3.5 character times)
  Serial.setTimeout(2000);                                                  // máximo en milisegundos de espera para la lectura a 2 segundos
  pinMode(10, INPUT);                                                       // pin 10 RX puerto 1
  pinMode(11, OUTPUT);                                                      // pin 11 TX puerto 1
  pinMode(USB_PC_PIN,INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_F1_PIN, OUTPUT);
  pinMode(LED_F2_PIN, OUTPUT);
  pinMode(ON_DPS1, OUTPUT);
  pinMode(ON_DPS2, OUTPUT);
  pinMode(ON_encoder, OUTPUT);
  pinMode(ON_5_3v3, OUTPUT);
  pinMode(PULSADOR_PIN, INPUT_PULLUP);
  pinMode(ENCODE_A, INPUT_PULLUP);
  pinMode(ENCODE_B, INPUT_PULLUP);
  
  digitalWrite(ON_encoder, HIGH);                                           // OFF encoder
  digitalWrite(BUZZER_PIN, LOW);                                            
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_F1_PIN, LOW);
  digitalWrite(LED_F2_PIN, LOW);
  digitalWrite(ON_5_3v3, HIGH);
  
  delay (500);                                                                                                       
  ini_nextion();
  if (analogRead(USB_PC_PIN) >500){                                         // comprobamos si tenemos el modo de control por PC
    digitalWrite(ON_5_3v3, LOW);
    txt_nextion("32", "Control  USB");
    cmd_nextion("tsw 255,0");
    dim_nextion();
    digitalWrite(ON_DPS2, LOW);
    digitalWrite(ON_DPS1, LOW);   
    while (analogRead(USB_PC_PIN) >500){
      delay (250);                                                          // esperamos que se pase a control por nextion
    }                                      
    digitalWrite(ON_5_3v3, HIGH);
    selec_esclavo(esclavo);
    reset_nextion();
  }
  if (EEPROM.read(10) !=10){                                                // comprobamos si se programo la eeprom con los valores de defecto
    EEPROM.write(10, 10);                                                   // indicamos programación valores por defecto
    EEPROM.write(0, 1);                                                     // encoder ON
    EEPROM.write(1, 1);                                                     // zumbador ON
    EEPROM.write(2, 5);                                                     // pasos a 5
  }
  if (EEPROM.read(0)){
    EICRA = 0b00001010;                                                     // configuramos flanco de bajada
    EIMSK = 0b00000000;                                                     // interrupciones int0 y int1 des-habilitamos
    pasos_enc =EEPROM.read(2);                                              // obtenemos el número de pasos para el encoder 
    on_off_encoder =1;
    digitalWrite(ON_encoder, LOW);                                          // ON encoder
  }
  if (EEPROM.read(1)){
    Timer1.initialize(100000);                                              // inicializamos a 100ms 
    Timer1.attachInterrupt(buzzer_OFF);                                     // interrupción gestión zumbador                                      
    Timer1.stop();
    on_off_buzzer =1;
  }
  selec_esclavo(1);
  delay (1500);                                                             // esperamos que inicialicen las fuente y la pantalla nextion  
  if (reg_DPS (1, 1, 10, 0)){                                               // ID 1, escribir 1, registro 10, valor 0 (retro iluminación a 0), 01 06 00 0a 00 00 a9 c8
    iniTime = millis();
    do{                                                                     // la espera de la respuesta puede tardar de 200ms a 500ms!!
      RX_serial_DPS(1);                                                     // leemos y NO gestionamos los datos recibidos
      if ((millis() - iniTime) > 750) status_RX_DPS =255;                   // si no hay respuesta n 750 ms salimos (error!)
    }while (status_RX_DPS <2);                                              // esperamos respuesta    
  } 
  if (status_RX_DPS !=255){
    if (reg_DPS (1, 1, 6, 1)){                                              // ID 1, escribir 1, registro 6, valor 1 (bloqueo ON), 01 06 00 06 00 01 a8 0b
      iniTime = millis();
      do{
        RX_serial_DPS(1);                                                   // leemos y NO gestionamos los datos recibidos
        if ((millis() - iniTime) > 750) status_RX_DPS =255;
      }while (status_RX_DPS <2);
    }
  }
  if (status_RX_DPS !=255){   
    if (reg_DPS (1, 0, 6, 0)){                                               // ID 1, leer 0, registro 6, valor 0 (estado bloqueo), 01 03 00 06 00 01 64 0b 
      iniTime = millis();
      do{
        RX_serial_DPS(0);                                                    // leemos y gestionamos los datos recibidos
        if ((millis() - iniTime) > 750) status_RX_DPS =255;
      }while (status_RX_DPS <2);
    }
  }
  // para continuar tenemos que tener el bloqueo teclado de la fuentes a ON->1
  while (datos_RX_DPS[3] !=1 || status_RX_DPS ==255){                         // el valor de la respuesta se encuentra en la posición 3
    digitalWrite(BUZZER_PIN , HIGH);
    digitalWrite(LED_F1_PIN , HIGH);
    delay (500);
    digitalWrite(BUZZER_PIN , LOW);
    digitalWrite(LED_F1_PIN , LOW);
    delay (3000);
  }
  num_nextion("0", 1, 2, 0);                                                  // OK fuente 1 r0.val=1
  
  selec_esclavo(2);
  if (reg_DPS (2, 1, 10, 0)){                                                 // ID 2, escribir 1, registro 10, valor 0 (retro iluminación a 0),02 06 00 0a 00 00 a9 fb
    iniTime = millis();
    do{
      RX_serial_DPS(1);                                                       // leemos y NO gestionamos los datos recibidos
      if ((millis() - iniTime) > 750) status_RX_DPS =255;  

    }while (status_RX_DPS <2);                                            
  }
  if (status_RX_DPS !=255){
    if (reg_DPS (2, 1, 6, 1)){                                                // ID 2, escribir 1, registro 6, valor 1 (bloqueo ON), 02 06 00 06 00 01 a8 38  
      iniTime = millis();
      do{
        RX_serial_DPS(1);                                                     // leemos y NO gestionamos los datos recibidos
        if ((millis() - iniTime) > 750) status_RX_DPS =255;
      }while (status_RX_DPS <2);                                                
    }
  }
  if (status_RX_DPS !=255){
    if (reg_DPS (2, 0, 6, 0)){                                                // ID 2, leer 1, registro 6, valor 0 (estado bloqueo),02 03 00 06 00 01 64 38 
      iniTime = millis();
      do{
        RX_serial_DPS(0);                                                     // leemos y gestionamos los datos recibidos
        if ((millis() - iniTime) > 750) status_RX_DPS =255;                                               
      }while (status_RX_DPS <2);
    }
  }
  // para continuar tenemos que tener el bloqueo teclado a ON->1
  while (datos_RX_DPS[3] !=1 || status_RX_DPS ==255){                         // el valor de la respuesta se encuentra en la posición 3
    digitalWrite(BUZZER_PIN , HIGH);
    digitalWrite(LED_F2_PIN , HIGH);
    delay (150);
    digitalWrite(BUZZER_PIN , LOW);
    digitalWrite(LED_F2_PIN , LOW);
    delay (100);
    digitalWrite(BUZZER_PIN , HIGH);
    digitalWrite(LED_F2_PIN , HIGH);
    delay (150);
    digitalWrite(BUZZER_PIN , LOW);
    digitalWrite(LED_F2_PIN , LOW);
    delay (3000);
  }
  num_nextion("1", 1, 2, 0);                                                  // OK fuente 2 r1.val=1 
  selec_esclavo(1);
}
bool datos_DPS =0, trama_borrada =0, espera_TX_DPS =0, stop_update =0, 
selec_pulsador_tx =0, pausa_test =0, reposo =0;
uint8_t fuente =1, update_dps_1 =0, update_dps_2 =0, fin_reg, esc_test, nextion_Palabras =0;
//******************Principal*************************************************
void loop(){
  switch (pantalla){
    case 0:                                                                   // pantalla principal petición de datos a las fuentes y enviá a la pantalla
      if (ciclo !=0){
        update_pantalla (0, 3);                                               // actualizamos datos U-SET, I-SET, UOUT, IOUT, POUT, UINT, LOCK, PROTECT, CV_CC y ON_OFF  de las fuentes 1 y 2
        update_pantalla (1, 3);                                               // actualizamos datos de memoria S-OVP, S-OCP Y S-OPP  de las fuentes 1 y 2  (-SET, I-SET de la memoria 0 es igual a U-SET, I-SET)  
        if (ciclo ==100){                                                     // arranque 
          nextion_rx(1);                                                      // no aseguramos de iniciar si datos en el buffer de RX de nextion (88 ff ff ff)
        } 
        if (fuente1_run ==1){
          reg_DPS (1, 1, 9, 0);                                               // salida de la fuente fuente 1 parada
          do{
            RX_serial_DPS(1);                                                 // recepción de la respuesta sin gestión
          }while (status_RX_DPS <2); 
          fuente1_run =0;
        }
        buzzer_ON(100000);
        if (fuente2_run ==1){      
          reg_DPS (2, 1, 9, 0);                                               // salida de la fuente fuente 2 parada    
          do{
            RX_serial_DPS(1);                                                 // recepción de la respuesta sin gestión
          }while (status_RX_DPS <2);
          fuente2_run =0;
        }          
        ciclo =0;   
      }   
      data_nextion_RX(0);                                                     // comprobamos si tenemos datos de la pantalla y si tenemos datos los enviamos a la fuente 
      if (pantalla !=0) break;                                                // se tenemos una nueva pagina, salimos
      pulsador();
      if (stop_update ==0) update_pantalla (0, fuente);                       // actualizamos datos U-SET, I-SET, UOUT, IOUT, POUT, UINT, LOCK, PROTECT, CV_CC y ON_OFF  de las fuentes 1 0 2
     
    if (pulsador_encoder ==1) pulsador_selec();                               // si tenemos pulsación
    if (estado_encoder !=0) encoder();                                        // si incrementa->1 o decrementa->2 el encoder
    if (fuente ==1){
      if (selec_pulsador ==6 || selec_pulsador ==30) update_dps_2 =10;        // si tenemos activo el encoder para la fuente 1
      if (fuente1_run ==0 || fuente2_run ==1 || update_dps_2 >9){             // si la fuente 2 esta en marcha o la 1 parada
        fuente =2;
        digitalWrite(LED_PIN, LOW); 
        update_dps_2 =0;
      }else update_dps_2++;
    }else{
      if (selec_pulsador ==1 || selec_pulsador ==20) update_dps_1 =10;        // si tenemos activo el encoder para la fuente 2
      if (fuente2_run ==0 || fuente1_run ==1 || update_dps_1 >9){             // si la fuente 1 esta en marcha o la 2 parada
        fuente =1;
        digitalWrite(LED_PIN, HIGH);
        update_dps_1 =0;
      }else update_dps_1++;
    }
    break;    
    case 1:                                                                   // pantalla opciones
        if (ciclo !=1){
          if (datos_DPS){                                                     // si estábamos esperando datos de la pantalla anterior (principal)
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            datos_DPS =0;
          }
          if (fuente1_run ==1){
            reg_DPS (1, 1, 9, 0);                                             // salida de la fuente fuente 1 parada
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2); 
            fuente1_run =0;
          }
          buzzer_ON(100000);
          if (fuente2_run ==1){      
            reg_DPS (2, 1, 9, 0);                                             // salida de la fuente fuente 2 parada    
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            fuente2_run =0;
          }
          ciclo =1;
      }  
      nextion_rx(0);
      break;     
     case 2:                                                                  // pantalla config
        if (ciclo !=2){
          read_DPS(1, 0x0b, 2);                                               // pedimos datos de la fuente 1, leemos modelo y firmware 0x0b y 0x0c
          do{
            RX_serial_DPS(0);
          }while (status_RX_DPS <2);                                          // esperamos respuesta
          data_nextion_TX(1, 0);                                              // enviamos los datos de la fuente 1 a la pantalla de configuración   
          buzzer_ON(100000);
          read_DPS(2, 0x0b, 2);                                               // pedimos datos de la fuente 2, leemos modelo y firmware 0x0b y 0x0c
          do{
            RX_serial_DPS(0);
          }while (status_RX_DPS <2);                                          // esperamos respuesta
          data_nextion_TX(2, 0);                                              // enviamos los datos del nodo 2 a la pantalla 2 
          ciclo =2;
        }
        nextion_Palabras =nextion_rx(0);                                      // comprobamos si tenemos datos de la pantalla
        if (nextion_Palabras !=0){                                            // si tenemos respuesta de la pantalla
          if (registro ==0xf0){                                               // zumbador
            registro =1;
            on_off_buzzer =datos_TX[1];
            num_nextion("0", on_off_buzzer, 4, 0);                            // c0.val= on/off,  enviamos opción zumbador
          }
          if (registro ==0xf1){                                               // encoder
            selec_pulsador =0;
            registro =0;
            if (datos_TX[1] ==1){
              //EIMSK = 0b00000011;                                           // habilitamos interrupciones int0 y int1
              on_off_encoder =1;
            }else{
              EIMSK = 0b00000000;                                             // des-habilitamos interrupciones int0 y int1    
              on_off_encoder =0;       
            }
            num_nextion("1", on_off_encoder, 4, 0);                           // c1.val= on/off, enviamos opción encoder
          }
          if (registro ==0xf2){                                               // pasos encoder
            registro =2;
            pasos_enc =datos_TX[1];         
          }
          buzzer_ON(100000);
          EEPROM.update(registro, datos_TX[1]); 
        }
      break;     
      case 3:                                                                 // pantalla test - escaneo
        if (ciclo !=3){
          buzzer_ON(100000);
          pausa_test =0;
          esc_test =2;
          ciclo=3;
        }
        nextion_Palabras =nextion_rx(0);
        if (nextion_Palabras !=0){
          if (pantalla !=3) break;
          if (registro ==0x44 || registro ==0x4a){
            (registro ==0x44) ? fin_reg =15: fin_reg =16;                     // registro indicacion fin escaneo voltaje o corriente
            esc_test =0;  
            pausa_test =0;
          }
          if (registro ==0x24){
            for(byte z=1; z <16; z++){                                        // copiamos las palabras recibidas
              datos_TX[15+z] =datos_TX[z];                                    // de la palabra 16 a la 30
            }
            datos_TX[31] =10;                                                 // la ultima palabra indica el número de pasos
            nextion_Palabras = 31;
            fin_reg =14;  
            esc_test =1;
            pausa_test =0;
          }
          if (esc_test <2){
            wire_DPS(esclavo, registro, nextion_Palabras);                    // enviamos a la fuente 1-2 la orden de escaneo o test  
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            do{
              read_DPS(esclavo, 2, 14);                                       // petición de lectura del registro 2 al 15 (leemos 14 registros)
              do{
                RX_serial_DPS(0);                                             // recepción de la respuesta y gestión
              }while (status_RX_DPS <2);
            }while (datos_RX_DPS[fin_reg] ==0);                               // esperamos indicacion en marcha test o escaneo (registro a 1->ON)
            buzzer_ON(200000);
            (esclavo ==1) ? fuente1_run =1: fuente2_run =1;
            digitalWrite(LED_F1_PIN, fuente1_run);
            digitalWrite(LED_F2_PIN, fuente2_run);
            do{
             nextion_Palabras =nextion_rx(0);
              if (nextion_Palabras !=0){
                if (pantalla !=3) break;
                if (registro ==0x4f || registro ==0x43 || registro ==0x49){    // comprobamos si tenemos orden de pausa
                  reg_DPS(esclavo, 1, registro, 1);                            // enviamos pausa
                  do{
                    RX_serial_DPS(1);                                          // recepción de la respuesta sin gestión
                  }while (status_RX_DPS <2);
                  pausa_test =1;
                  break;
                }else
                  pausa_test =0;
              }
              read_DPS(esclavo, 2, 14);                                        // ID, leemos 14 palabras comenzando el el registro 2 
              do{
                RX_serial_DPS(0);                                              // recepción de la respuesta y gestión
              }while (status_RX_DPS <2);
              data_nextion_TX(esclavo, bool (esc_test));                       // enviamos a la pantalla la Uout y Iout
            }while (datos_RX_DPS[fin_reg] ==1);                                // esperamos indicacion de fin test o escaneo
            esc_test =2;
            buzzer_ON(100000);
            (esclavo ==1) ? fuente1_run =0: fuente2_run =0;
            digitalWrite(LED_F1_PIN, fuente1_run);
            digitalWrite(LED_F2_PIN, fuente2_run);
            if (pausa_test ==0)
             num_nextion("0", 1, 0, 0);                                        // enviamos OK va0.val=1
          }
        }
      break;
      case 4:                                                                  // pantalla gráfica fuente 1
        if (ciclo !=4){
          if (fuente1_run ==0){
            reg_DPS (1, 1, 9, 1);                                             // salida de la fuente 1 en marcha
            buzzer_ON(200000);
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            fuente1_run =1;
          }
          ciclo =4;
        }
        wave_nextion_tx(1);
        nextion_rx(0);
      break;
      case 5:                                                                 // pantalla gráfica fuente 2
        if (ciclo !=5){
          if (fuente2_run ==0){
            reg_DPS (2, 1, 9, 1);                                             // salida de la fuente fuente 2 en marcha
            buzzer_ON(200000);
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            fuente2_run =1;
          }
          ciclo =5;
        }
        wave_nextion_tx(2);
        nextion_rx(0);
      break;
      case 6:                                                                 // pantalla memorias
        if (ciclo !=6){
          if (datos_DPS){                                                     // si estábamos esperando datos de la pantalla anterior (principal)
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            datos_DPS =0;
          }
          if (fuente1_run ==1){
            reg_DPS (1, 1, 9, 0);                                             // salida de la fuente fuente 1 parada
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            fuente1_run =0;
          }
          buzzer_ON(100000);
          if (fuente2_run ==1){
            reg_DPS (2, 1, 9, 0);                                             // salida de la fuente fuente 2 parada
            do{
              RX_serial_DPS(1);                                               // recepción de la respuesta sin gestión
            }while (status_RX_DPS <2);
            fuente2_run =0;
          }
          ciclo =6;
        }
        nextion_Palabras =nextion_rx(0);                                      // comprobamos si tenemos datos de la pantalla  
        if (nextion_Palabras !=0){
          if (registro ==0x23){                                               // petición desde la pantalla de leer memoria fuente 1 o 2
            read_DPS(esclavo, grupo_memoria, 8);                              // petición a la fuente de registros del grupo de memoria
            do{
              RX_serial_DPS(0);                                               // recepción de la respuesta 
            }while (status_RX_DPS <2);
            data_nextion_TX(esclavo, 0);                                      // enviamos a la pantalla los valores de la memoria indicada
            //wire_DPS(esclavo, 0x23, 1);                                     // acceso a memoria  
            reg_DPS (esclavo, 1, 0x23, datos_TX[1]);                          // ID 1-2, escribir 1, registro acceso a memoria, valor recibido 
            do{
                RX_serial_DPS(1);                                             // escribimos y NO gestionamos los datos recibidos
            }while (status_RX_DPS <2);
            buzzer_ON(100000);
            num_nextion("m6", 10, 0, 0);                                      // vam6.val=10, OK, leida i acceso a memoria
          }else{                                                              // cambio valor de memoria
            if (registro >0x4f && registro <0xe8){                            // es un registro de las memorias 0-9 (0x50 a 0xe7)
              reg_DPS (esclavo, 1, registro, datos_TX[1]);                    // ID 1-2, escribir 1, registro de memoria, valor recibido  
              buzzer_ON(100000);
              do{
                RX_serial_DPS(1);                                             // escribimos y NO gestionamos los datos recibidos
              }while (status_RX_DPS <2);
            }
          }
        }
      break;
      case 7:                                                                 // pantalla teclado
        if (ciclo ==0)                                                        // si venimos de la pantalla 0       
          stop_update =1;
        buzzer_ON(100000);
        pantalla =ciclo;                                                      // retornamos a la pagina anterior
      break;
    }
    digitalWrite(LED_F1_PIN, fuente1_run);
    digitalWrite(LED_F2_PIN, fuente2_run);
    if (analogRead(USB_PC_PIN) >500){ 
      if (pc_control ==0){
        digitalWrite(ON_5_3v3, LOW);
        dim_nextion();
        control_pc(1);                                                        // bloqueamos pantalla, control por PC remoto
        while (analogRead(USB_PC_PIN) >500){                                  // esperamos que se pase a control por nextion
          delay (250);
        }
        digitalWrite(ON_5_3v3, HIGH);
        reset_nextion();
        pantalla =0;   
        control_pc(0);                                                        // desbloqueamos pantalla, control por pantalla nextion
      }
    }else{
      if (nextion_Palabras !=0 || fuente1_run ==1 || fuente2_run ==1){        // activo
       iniTime = millis();
      }else{                                                                  // inactivo
        if (millis() >(iniTime +600000)){                                     // 10 minutos
          buzzer_ON(300000);
          iniTime = millis();
          if (reposo ==0){
            sleep_nextion(1);
            reposo =1;
          }
        }
        if (reposo ==1 && !digitalRead(PULSADOR_PIN)){                        // salimos de reposo pantalla
          sleep_nextion(0);
          while(!digitalRead(PULSADOR_PIN)){}
          iniTime = millis();
          reposo =0;
        }
      }
    }
}
//****************************************************************************************************************************************************
// actualizamos la pantalla con los datos de las fuentes, indicando si son datos de memoria o de control
void update_pantalla(bool data_mem, uint8_t up_fuente){
  if (data_mem){                                                              // registros de memoria
    uint8_t p_memoria =grupo_memoria;
    p_memoria =grupo_memoria+2;                                               // obtenemos la primera posición del la memoria a leer
    if (up_fuente ==1 || up_fuente ==3){                                      // actualizamos memoria fuente 1 si es 1 o 3
      read_DPS(1, p_memoria, 3);                                              // ID 1, S-OVP, S-OCP Y S-OPP del grupo de memoria
      do{
        RX_serial_DPS(0);                                                     // comprobamos si tenemos los datos de la fuente 1 
        pulsador();                                 
      }while (status_RX_DPS <2);  
      data_nextion_TX(1, data_mem);                                           // enviamos los valores S-OVP, S-OCP Y S-OPP de la fuente 1 a nextion
    }
    if (up_fuente ==2 || up_fuente ==3){                                      // actualizamos fuente 1 si es 2 o 3
      read_DPS(2, p_memoria, 3);                                              // ID 2, del registro S-OVP, S-OCP Y S-OPP del grupo de memoria
        do{
          RX_serial_DPS(0);                                                   // comprobamos si tenemos los datos de la fuente 2  
          pulsador();                                 
        }while (status_RX_DPS <2);  
        data_nextion_TX(2, data_mem);                                         // enviamos los valores U-set-, I-set, S-OVP, S-OCP Y S-OPP de la fuente 2 a nextion
    }
  }else{                                                                      // registros de datos
    if (up_fuente ==1 || up_fuente ==3){                                      // actualizamos datos fuente 1 si es 1 o 3
      read_DPS(1, 0, 10);                                                     // pedimos datos de la fuente->ID fuente, leemos U-SET, I-SET, UOUT, IOUT, POUT, UINT, LOCK, PROTECT, CV_CC y ON_OFF  
      do{
        RX_serial_DPS(0);                                                     // comprobamos si tenemos los datos de la fuente 1   
        pulsador();                               
      }while (status_RX_DPS <2);  
      data_nextion_TX(1, data_mem);                                           // enviamos los valores U-SET, I-SET, UOUT, IOUT, POUT, UINT, LOCK, PROTECT, CV_CC y ON_OFF de la fuente 1 a nextion
    }
    if (up_fuente ==2 || up_fuente ==3){                                      // actualizamos fuente 2 si es 2 o 3
      read_DPS(2, 0, 10);                                                     // pedimos datos de la fuente 2->ID fuente, leemos U-SET, I-SET, UOUT, IOUT, POUT, UINT, LOCK, PROTECT, CV_CC y ON_OFF  
      do{
        RX_serial_DPS(0);                                                     // comprobamos si tenemos los datos de la fuente 2   
        pulsador();                               
      }while (status_RX_DPS <2);  
      data_nextion_TX(2, data_mem);                                           // enviamos los valores U-SET, I-SET, UOUT, IOUT, POUT, UINT, LOCK, PROTECT, CV_CC y ON_OFF de la fuente 2 a nextion
    }
  }
}
 // Gestión recepción de  datos de la pantalla
void data_nextion_RX(bool espera_tx){
  nextion_Palabras =nextion_rx(0);                                            // comprobamos si tenemos datos de la pantalla
  if (nextion_Palabras !=0){                                                  // si tenemos datos de nextion para las fuentes
    if (espera_tx)
      espera_TX_DPS =1;                                                       // indicamos que tenemos datos para enviar a las fuentes
    else
      TX_data_DPS();                                                          // enviamos a la fuente 1 o 2  
      if (stop_update ==1) stop_update =0;   
  }else{
    if (registro ==255 && datos_DPS ==1){                                     // si es un cambio de pagina y estamos esperando respuesta de la fuente
      do{
        RX_serial_DPS(1);                                                     // recepción de la respuesta, SIN gestión
      }while (status_RX_DPS <2);
      datos_DPS =0;
    }
  }
}
// Enviamos lo recibido de la pantalla a las fuentes
void TX_data_DPS(){ 
long tone_on_off =25000; 
  if (nextion_Palabras ==1){                                              
    reg_DPS (esclavo, 1, registro, datos_TX[1]);                              // enviamos una palabra al registro correspondiente de la fuente 1-2
    if (registro ==0x09 && datos_TX[1] ==1) tone_on_off =175000;              // fuente en marcha, tono largo
  }else{                                                                      // si tenemos varias palabras 
    wire_DPS (esclavo, registro, nextion_Palabras);                           // enviamos las palabras a los registros correspondiente de la fuente 1-2
  }
  buzzer_ON(tone_on_off);  
  do{
    if (pantalla ==0) pulsador();
    RX_serial_DPS(1);                                                         // recepción de la respuesta sin gestión
  }while (status_RX_DPS <2);                                                  // esperamos la respuesta, sin gestionar
  if (registro <24)                                                           // actualizamos
    update_pantalla(0, esclavo);
  else
    update_pantalla(1, esclavo);
  espera_TX_DPS =0;                                                           // indicamos sin datos para enviar (enviados)
}
// Gestión de control por USB o pantalla
void control_pc(bool on_off){
  pc_control=on_off;
  buzzer_ON(200000);
  if (pc_control){
    reg_DPS (1, 1, 9, 0);                                                     // salida de la fuente fuente 1 parada, escribir en registro 9 el valor 0
    do{
      RX_serial_DPS(1);                                                       // recepción de la respuesta sin gestión
    }while (status_RX_DPS <2);
    data_nextion_TX(1, 0);
    reg_DPS (2, 1, 9, 0);                                                     // salida de la fuente fuente 2 parada, escribir en registro 9 el valor 0
    do{
      RX_serial_DPS(1);                                                       // recepción de la respuesta sin gestión
    }while (status_RX_DPS <2);
    data_nextion_TX(2, 0);
    txt_nextion("32", "Control  USB");
    cmd_nextion("tsw 255,0");
    digitalWrite(ON_DPS2, LOW);
    digitalWrite(ON_DPS1, LOW);
  }else{
    selec_esclavo(esclavo);
    txt_nextion("32", "");
    cmd_nextion("tsw 255,1");
  }
}
// Comprobamos si se está pulsado el encoder
void pulsador(){
  if (selec_pulsador_tx ==1 || reposo ==1){                                                 // solo se gestiona si se envió a la pantalla el valor de la última pulsación
    return;
  }
  if (digitalRead(PULSADOR_PIN)){                                             // obtenemos el valor del pulsador del encoder
    pulsador_encoder =0;                             
  }else{
    pulsador_encoder =1;                                                      // se esta pulsando
    buzzer_ON(25000);
    selec_pulsador_tx =1;                                                     // indicamos que tenemos que enviar el estado del pulsador
    while(!digitalRead(PULSADOR_PIN)){}
  }
}
// En cada pulsación se selecciona la opción correspondiente
void pulsador_selec(){  
uint8_t x =0;
  estado_encoder =0;
  pulsador_encoder =0; 
  switch (selec_pulsador){
    case 0:                                                                   // primera pulsación
      if (on_off_encoder ==1)
        EIMSK = 0b00000011;                                                   // habilitamos interrupciones int0 y int1 en la primera pulsación
      else
        buzzer_ON(200000);
      x =1;                                                                   // V-set fuente 1 
    break;
    case 1:                                                          
      x =20;                                                                  // botón ON/OFF fuente 1
    break;
    case 20:
      x =6;                                                                   // V-set fuente 2
    break;
    case 6:
      x =30;                                                                  // botón ON/OFF fuente 2
    break;
    case 30:
      x =1;                                                                   // V-set fuente 1
    break; 
  }
  selec_pulsador =x;
  num_nextion("16", selec_pulsador, 0, 0);                                    // enviamos estado pulsador encoder, la pantalla responde con el valor actual
  while (!nextion_rx(0)){                                                     // esperamos la respuesta de la pantalla
    delay(2);                                                                 // rx nextion +-5ms
  }                                                   
  selec_pulsador_tx =0;
}
void encoder(){
long on_off_tone =0;
bool tx_DPS =0;
  fuente =1;
  if (selec_pulsador ==1 || selec_pulsador ==6){                              // V-set fuente 1 o 2
    buzzer_ON(25000);
    if (estado_encoder ==1)
      data_encoder =data_encoder +pasos_enc;                                  // incremento
    if (estado_encoder ==2)
      data_encoder =data_encoder -pasos_enc;                                  // decremento
    if (selec_pulsador ==6) fuente =2;
    if (data_encoder >0 && data_encoder<5001){
      reg_DPS (fuente, 1, 0, data_encoder);                                   // si son valores validos enviamos V-set
      tx_DPS =1;
    }
  }else{
   if (estado_encoder ==1){                                                         
    data_encoder =1;                                                          // ON fuente 1 o 2
    on_off_tone =175000;
   }
   if (estado_encoder ==2){                                                         
    data_encoder =0;                                                          // OFF fuente 1 o 2
    on_off_tone =25000;
   }
   if (selec_pulsador ==30){
    if (fuente2_run !=data_encoder) tx_DPS =1;
    fuente =2;
   }else
    if (fuente1_run !=data_encoder) tx_DPS =1;
   if (tx_DPS ==1){
    reg_DPS (fuente, 1, 9, data_encoder);                                     // enviamos ON/OFF
   }
  }
  if (on_off_tone !=0) buzzer_ON(on_off_tone); 
  estado_encoder =0;
  if (tx_DPS){
    do{
      RX_serial_DPS(1);                                                       // comprobamos si tenemos la respuesta                    
    }while (status_RX_DPS <2);   
  }
}
// -----------------------------------ISR-------------------------------
// Encoder pin A
ISR (INT0_vect){ 
  if (digitalRead(ENCODE_B) ==HIGH) estado_encoder =2;                        // si el pin B es 1, es una rotación a izquierda
}
// Encoder pin B
ISR (INT1_vect){                      
  if (digitalRead(ENCODE_A) ==HIGH) estado_encoder =1;                        // si el pin A es 1, es una rotación a derecha
}

// OFF zumbador
void buzzer_OFF(){
  digitalWrite(BUZZER_PIN, LOW);
  Timer1.stop();
}
// ON zumbador
void buzzer_ON(unsigned long run_buzzer){
  if (!on_off_buzzer) return;
  digitalWrite(BUZZER_PIN, HIGH);
  Timer1.setPeriod(run_buzzer);                                               // ajuntamos en microsegundos 
}
