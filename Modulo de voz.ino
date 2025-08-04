// Versión para ESP32

/**
  ******************************************************************************
  * @file    vr_sample_bridge.ino
  * @author  JiapengLi
  * @brief   Este archivo proporciona una demostración de cómo controlar LEDs
              utilizando el módulo de reconocimiento de voz
  ******************************************************************************
  * @nota:
        Use este ejemplo para probar los comandos del módulo de reconocimiento de voz.
    Ejemplo:
       1. Habilitar "Enviar con nueva línea" en el monitor serial, Baud rate 115200.
       2. Ingresar "01" para "verificar reconocedor"
       3. Ingresar "31" para "limpiar reconocedor"
       4. Ingresar "30 00 02 04" para "cargar registro 0, registro 2, registro 4"
  ******************************************************************************
  * @sección  HISTORIA
    
    2013/06/17    Versión inicial.
    2023/08/04    Modificado para ESP32 con control de salidas
  */
  

#include "VoiceRecognitionV3.h"

/**        
  Conexiones
  ESP32       Módulo de Reconocimiento de Voz
   RX2 (GPIO16) -------> TX
   TX2 (GPIO17) -------> RX
   
  Salidas:
   GPIO18 -------> Salida para Comando 0
   GPIO19 -------> Salida para Comando 2
   GPIO21 -------> Salida para Comando 4
*/
VR miVR(16, 17);  // UART2: 16=RX, 17=TX

// Definición de pines de salida para cada comando
#define PIN_COMANDO0  18  // GPIO18 para Comando 0
#define PIN_COMANDO1  19  // GPIO19 para Comando 2
#define PIN_COMANDO2  21  // GPIO21 para Comando 4

/***************************************************************************/
// Parte de análisis de comandos
#define LONGITUD_BUFFER_COMANDO      64+1
uint8_t comando[LONGITUD_BUFFER_COMANDO];
uint8_t contador_comando;
uint8_t *direccion_parametro;

uint8_t buffer[400];
uint8_t longitud_buffer[32];

void setup(void)
{
  miVR.begin(9600);
  
  // Configurar pines de salida como digitales
  pinMode(PIN_COMANDO0, OUTPUT);
  pinMode(PIN_COMANDO1, OUTPUT);
  pinMode(PIN_COMANDO2, OUTPUT);
  
  // Inicializar todas las salidas en estado bajo
  digitalWrite(PIN_COMANDO0, LOW);
  digitalWrite(PIN_COMANDO1, LOW);
  digitalWrite(PIN_COMANDO2, LOW);
  
  /** Inicialización */
  Serial.begin(115200);
  Serial.println(F("Módulo de Reconocimiento de Voz V3 Elechouse - Ejemplo \"bridge\""));
  Serial.println(F("Ejemplo de uso:\r\n1. Habilitar \"Enviar con nueva línea\" en el monitor serial\r\n2. Baud rate 115200\r\n3. Ingresar \"01\" para verificar el reconocedor\r\n4. Ingresar \"31\" para limpiar el reconocedor\r\n5. Ingresar \"30 00 02 04\" para cargar registros 0, 2 y 4"));
}

void loop(void)
{
  int longitud, i, retorno, indice;
  
  /** Recibir comando desde el Serial */
  longitud = recibirComando();
  if(longitud > 0){
    imprimirSeparador();  
    if(!verificarComando(longitud)){    
      retorno = convertirComando(buffer, longitud);
      if(retorno>0){
        Serial.print("> ");
        miVR.writehex(buffer, retorno);
        miVR.send_pkt(buffer, retorno);
        Serial.println();
      }else{
        /** El comando recibido es inválido */
        Serial.write(comando, longitud);
        Serial.println(F("Error: Entrada no está en formato hexadecimal."));
      }    
    }else{
      /** El comando recibido es inválido */
      Serial.print("> ");
      Serial.write(comando, longitud); 
      Serial.println(F("Error: Entrada no está en formato hexadecimal."));
    }
  }
  
  /** Recibir todos los paquetes disponibles */
  longitud = 0;
  indice = 0;
  while(1){
    retorno = miVR.receive_pkt(buffer+longitud, 50);
    if(retorno>0){
      longitud += retorno;
      longitud_buffer[indice] = retorno;
      indice++;
    }else{
      break;
    }
  }
  
  // Procesar paquetes recibidos
  if(indice > 0){
    longitud = 0;
    for(i=0; i<indice; i++){
      Serial.print("< ");
      miVR.writehex(buffer+longitud, longitud_buffer[i]);
      
      // Procesar paquete de reconocimiento
      procesarPaqueteVR(buffer+longitud, longitud_buffer[i]);
      
      longitud += longitud_buffer[i];
      Serial.println();
    }
  }
}

/**
  @brief   Procesa los paquetes de reconocimiento de voz
  @param   buffer    -> Puntero al buffer de datos
  @param   longitud  -> Longitud del paquete recibido
  @retval  Ninguno
*/
void procesarPaqueteVR(uint8_t *buffer, uint8_t longitud) {
  // El paquete de reconocimiento válido debe tener al menos 3 bytes
  if (longitud < 3) return;
  
  // Verificar si es un paquete de reconocimiento válido (cabecera 0xAA)
  if (buffer[0] == 0xAA) {
    uint8_t numero_comandos = buffer[1];
    
    // Verificar integridad del paquete
    if (longitud < (2 + numero_comandos)) return;
    
    // Procesar cada comando detectado en el paquete
    for (int i = 0; i < numero_comandos; i++) {
      uint8_t comando_detectado = buffer[2 + i];
      
      // Apagar todas las salidas primero
      digitalWrite(PIN_COMANDO0, LOW);
      digitalWrite(PIN_COMANDO1, LOW);
      digitalWrite(PIN_COMANDO2, LOW);
      
      // Activar la salida correspondiente al comando detectado
      switch (comando_detectado) {
        case 0:  // Registro 0
          Serial.println("Comando 0 detectado - Activando GPIO18");
          digitalWrite(PIN_COMANDO0, HIGH);
          break;
        case 2:  // Registro 2
          Serial.println("Comando 2 detectado - Activando GPIO19");
          digitalWrite(PIN_COMANDO1, HIGH);
          break;
        case 4:  // Registro 4
          Serial.println("Comando 4 detectado - Activando GPIO21");
          digitalWrite(PIN_COMANDO2, HIGH);
          break;
        default:  // Comando no reconocido
          Serial.print("Comando desconocido detectado: 0x");
          Serial.println(comando_detectado, HEX);
          break;
      }
    }
  }
}

/**
  @brief   Recibe comandos desde el puerto serial
  @param   Ninguno
  @retval  Longitud del comando, -1 si no se recibió comando
*/
int recibirComando()
{
  int retorno;
  int longitud;
  unsigned long inicio_milis = millis();
  
  if(!Serial.available()){
    return -1;
  }
  
  while(1){
    retorno = Serial.read();
    if(retorno > 0){
      inicio_milis = millis();
      comando[contador_comando] = retorno;
      
      // Final de comando (nueva línea)
      if(comando[contador_comando] == '\n'){
        longitud = contador_comando + 1;
        contador_comando = 0;
        return longitud;
      }
      
      contador_comando++;
      
      // Prevención de desbordamiento del buffer
      if(contador_comando == LONGITUD_BUFFER_COMANDO){
        contador_comando = 0;
        return -1;
      }
    }
    
    // Timeout después de 100ms
    if(millis() - inicio_milis > 100){
      contador_comando = 0;
      return -1;
    }
  }
}

/**
  @brief   Verifica el formato del comando
  @param   longitud  -> Longitud del comando
  @retval  0  -> Comando válido
          -1  -> Comando inválido
*/
int verificarComando(int longitud)
{
  for(int i=0; i<longitud; i++){
    if(comando[i] >= '0' && comando[i] <= '9') continue;
    if(comando[i] >= 'a' && comando[i] <= 'f') continue;
    if(comando[i] >= 'A' && comando[i] <= 'Z') continue;
    if(comando[i] == '\t' || comando[i] == ' ' || 
       comando[i] == '\r' || comando[i] == '\n') continue;
    
    return -1;
  }
  return 0;
}

/**
  @brief   Verifica el número de parámetros en el comando
  @param   longitud  -> Longitud del comando
  @retval  Número de parámetros
*/
int verificarNumeroParametros(int longitud)
{
  int contador = 0;
  for(int i=0; i<longitud; ){
    if(comando[i] != '\t' && comando[i] != ' ' && 
       comando[i] != '\r' && comando[i] != '\n'){
      contador++;
      while(i < longitud && comando[i] != '\t' && comando[i] != ' ' && 
            comando[i] != '\r' && comando[i] != '\n'){
        i++;
      }
    }else{
      i++;
    }
  }
  return contador;
}

/**
  @brief   Busca un parámetro específico en el comando
  @param   longitud          -> Longitud del comando
           indice_parametro  -> Índice del parámetro a buscar
           direccion         -> Dirección del parámetro encontrado
  @retval  Longitud del parámetro encontrado
*/
int buscarParametro(int longitud, int indice_parametro, uint8_t **direccion)
{
  int contador = 0;
  for(int i=0; i<longitud; ){
    uint8_t dato = comando[i];
    if(dato != '\t' && dato != ' '){
      contador++;
      if(contador == indice_parametro){
        *direccion = comando + i;
        int longitud_parametro = 0;
        
        // Calcular longitud del parámetro
        while(i < longitud && comando[i] != '\t' && comando[i] != ' ' && 
              comando[i] != '\r' && comando[i] != '\n'){
          i++;
          longitud_parametro++;
        }
        return longitud_parametro;
      }else{
        // Saltar al siguiente parámetro
        while(i < longitud && comando[i] != '\t' && comando[i] != ' ' && 
              comando[i] != '\r' && comando[i] != '\n'){
          i++;
        }
      }
    }else{
      i++;
    }
  }
  return -1;
}

/**
  @brief   Convierte una cadena hexadecimal a decimal
  @param   cadena  -> Cadena hexadecimal
  @retval  Valor decimal
*/
uint32_t hexadecimal_a_decimal(uint8_t *cadena)
{
  uint32_t retorno = 0;
  for(int i=0; i<8; i++){  // Máximo 8 caracteres
    if(cadena[i] >= '0' && cadena[i] <= '9'){
      retorno = (retorno << 4) + (cadena[i] - '0');
    }else if(cadena[i] >= 'A' && cadena[i] <= 'F'){
      retorno = (retorno << 4) + (cadena[i] - 'A' + 10);
    }else if(cadena[i] >= 'a' && cadena[i] <= 'f'){
      retorno = (retorno << 4) + (cadena[i] - 'a' + 10);
    }else{
      break;
    }
  }
  return retorno;
}

/**
  @brief   Convierte el comando a formato binario
  @param   destino   -> Buffer de destino
           longitud  -> Longitud del comando
  @retval  Número de bytes convertidos
*/
int convertirComando(uint8_t *destino, int longitud)
{
  int numero_parametros = verificarNumeroParametros(longitud);
  for(int i=0; i<numero_parametros; i++){
    int longitud_parametro = buscarParametro(longitud, i+1, &direccion_parametro);
    
    // Validar longitud del parámetro
    if(longitud_parametro > 2 || longitud_parametro <= 0) return -1;
    
    destino[i] = hexadecimal_a_decimal(direccion_parametro);
    
    // Validar conversión
    if(destino[i] == 0){
      if(*direccion_parametro != '0') return -1;
      if(longitud_parametro == 2 && *(direccion_parametro+1) != '0') return -1;
    }
  }
  return numero_parametros;
}

/**
  @brief   Imprime un separador en el monitor serial
  @param   Ninguno
  @retval  Ninguno
*/
void imprimirSeparador()
{
  for(int i=0; i<80; i++){
    Serial.write('-');
  }
  Serial.println();
}