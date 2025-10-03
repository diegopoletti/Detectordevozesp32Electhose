/**
 * @file MAscara de Ironman
 * @author Código Diego Poletti
 * @brief Programa de control para un sistema interactivo basado en ESP32 que utiliza reconocimiento de voz, reproducción de audio MP3 y control de servomotores.
 * @version 1.4
 * @date 2025-10-2
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este firmware está diseñado para un microcontrolador ESP32. Su función principal es escuchar comandos de voz a través de un módulo externo
 * (Voice Recognition V3), procesarlos y ejecutar acciones en consecuencia. Estas acciones incluyen la reproducción de archivos de audio desde una
 * tarjeta SD, la activación de salidas digitales (como LEDs o relés) y el movimiento preciso de un servomotor.
 * Se implementa una máquina de estados simple en el bucle principal para garantizar que la reproducción de audio no sea interrumpida por nuevos comandos.
 * Además, incluye un modo de entrenamiento que permite al usuario grabar sus propios comandos de voz, asociándolos a acciones predefinidas.
 */

// --- SECCIÓN DE INCLUSIÓN DE LIBRERÍAS ---
// Una librería es un conjunto de código preescrito que nos proporciona herramientas para realizar tareas complejas sin tener que programarlas desde cero.
// La directiva '#include' le indica al compilador que debe "importar" estas herramientas para que podamos usarlas en nuestro programa.
#include "VoiceRecognitionV3.h"           ///< Importa la librería específica para interactuar con el módulo de reconocimiento de voz. Contiene las funciones para enviar comandos y recibir respuestas.
#include "AudioFileSourceSD.h"            ///< Proporciona la capacidad de utilizar un archivo almacenado en una tarjeta SD como fuente de datos de audio.
#include "AudioGeneratorMP3.h"            ///< Contiene el algoritmo necesario para decodificar el formato de compresión MP3 y convertirlo en datos de audio sin procesar (PCM).
#include "AudioOutputI2SNoDAC.h"          ///< Gestiona el envío de la señal de audio digital a los pines del ESP32 utilizando el protocolo I2S, una interfaz estándar para audio.
#include "FS.h"                           ///< Incluye el "File System" (Sistema de Archivos) base de ESP32, necesario para interactuar con cualquier tipo de almacenamiento.
#include "SD.h"                           ///< Librería específica que implementa los comandos para comunicarse con una tarjeta de memoria SD sobre el sistema de archivos.
#include "SPI.h"                          ///< Proporciona las funciones para la comunicación "Serial Peripheral Interface", el protocolo de alta velocidad que usa el ESP32 para hablar con la tarjeta SD.
#include <ESP32Servo.h>                   ///< Importa la librería optimizada para el control de servomotores en el ESP32, gestionando los timers de hardware para generar las señales PWM necesarias.

// --- SECCIÓN DE DEFINICIONES Y CONSTANTES GLOBALES ---
// Usamos la directiva '#define' para crear "macros". Esto sustituye cada aparición del nombre (ej. 'Version') por su valor (ej. "1.4") antes de la compilación.
// Es una buena práctica para asignar nombres legibles a valores fijos, facilitando la lectura y el mantenimiento del código.
#define Version "1.4"                     ///< Define la versión del firmware. Es útil para el control de cambios y depuración.

// --- CONFIGURACIÓN DE PINES (HARDWARE MAPPING) ---
// Aquí se asignan nombres descriptivos a los números de los pines físicos (GPIO) del ESP32. Esto hace que el código sea más legible y fácil de modificar si el hardware cambia.
// Módulo de voz
#define PIN_TX 17                         ///< Asigna el pin GPIO 17 para la función de Transmisión (TX), enviando datos desde el ESP32 hacia el módulo de voz.
#define PIN_RX 16                         ///< Asigna el pin GPIO 16 para la función de Recepción (RX), recibiendo datos desde el módulo de voz hacia el ESP32.
// I/O (Entrada/Salida)
#define LED_INTERNO 2                     ///< Define el pin GPIO 2 para controlar el LED integrado en la placa ESP32, usado para feedback visual.
#define NUM_REGISTROS 11                  ///< Define una constante para el número total de comandos de voz que el sistema manejará.
// Salidas Digitales
#define SALIDA_0 25                       ///< Asigna el pin GPIO 25 a una salida digital, asociada lógicamente al comando "Jarvis".
#define SALIDA_1 26                       ///< Asigna el pin GPIO 26 a una salida digital, asociada lógicamente al comando "Abrir".
#define SALIDA_2 27                       ///< Asigna el pin GPIO 27 a una salida digital, asociada lógicamente al comando "Cerrar".
#define SALIDA_REACTOR_ON 33              ///< Asigna el pin GPIO 33 a una salida digital, asociada lógicamente al comando "Reactor On".
// Tarjeta SD
#define CS 5                              ///< Asigna el pin GPIO 5 como "Chip Select" (CS) para la tarjeta SD. Este pin se usa para activar la comunicación con la tarjeta.
// Servomotor
#define SERVO_PIN 32                      ///< Define el pin GPIO 32 como el pin de señal para el servomotor.
#define SERVO_CERRADO 0                   ///< Constante que define la posición angular de reposo o "cerrado" del servo (0 grados).
#define SERVO_ABIERTO 90                  ///< Constante que define la posición angular de trabajo o "abierto" del servo (90 grados).
#define SERVO_STEP_DEG 1                  ///< Define el incremento angular (1 grado) para cada paso del movimiento del servo, permitiendo un desplazamiento suave.
#define SERVO_STEP_MS 15                  ///< Define la pausa en milisegundos entre cada paso de movimiento del servo, controlando la velocidad del desplazamiento.

// --- DECLARACIÓN DE VARIABLES GLOBALES Y OBJETOS ---
// En esta sección se "instancian" los objetos de las clases proporcionadas por las librerías y se declaran las variables que necesitan ser accesibles desde cualquier parte del programa.
// Reconocimiento de voz
VR miReconocedor(PIN_RX, PIN_TX);         ///< Crea un objeto llamado 'miReconocedor' de la clase 'VR'. Se le pasan los pines RX y TX como parámetros a su constructor para inicializar la comunicación.
uint8_t registros[NUM_REGISTROS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; ///< Declara un array (lista) de enteros de 8 bits sin signo para almacenar los índices de los comandos que se cargarán en el módulo.
uint8_t buf[65];                          ///< Declara un array de bytes que funcionará como "buffer" (almacén temporal) para guardar los datos recibidos del módulo de voz.

// Reproducción de audio
AudioGeneratorMP3 decodificadorMp3;       ///< Crea un objeto de la clase 'AudioGeneratorMP3', que será el encargado de la lógica de decodificación.
AudioFileSourceSD fuenteAudio;            ///< Crea un objeto de la clase 'AudioFileSourceSD', que se usará para acceder al archivo MP3 en la tarjeta SD.
AudioOutputI2SNoDAC salidaI2S;            ///< Crea un objeto de la clase 'AudioOutputI2SNoDAC', responsable de enviar el audio procesado a los pines físicos.
String archivoSiguiente = "";             ///< Declara una variable de tipo String para almacenar la ruta del siguiente archivo de audio a reproducir en una secuencia.

// Parpadeo de LED (Control asíncrono)
bool parpadeando = false;                 ///< Variable booleana (true/false) que actúa como bandera para activar o desactivar la lógica de parpadeo.
int parpadeosObjetivo = 0;                ///< Almacena cuántos ciclos de encendido/apagado debe realizar el LED.
int parpadeosRealizados = 0;              ///< Contador para llevar la cuenta de los parpadeos ya efectuados.
bool estadoLed = false;                   ///< Almacena el estado actual del LED (HIGH o LOW) para poder invertirlo.
unsigned long ultimoParpadeoMillis = 0;   ///< Almacena el tiempo (en milisegundos desde el inicio) del último cambio de estado del LED, usando la función millis().
unsigned long intervaloParpadeo = 200;    ///< Define la duración en milisegundos de cada estado (encendido o apagado) del LED.

// Salidas por pulso
unsigned long salidaOffMillis[NUM_REGISTROS] = {0}; ///< Un array para almacenar el "timestamp" futuro en el que cada salida digital debe apagarse, permitiendo crear pulsos de duración controlada.

// Movimiento del servo
Servo servo;                              ///< Crea un objeto de la clase 'Servo', que nos dará una interfaz de alto nivel para controlar el motor.
int servoAnguloActual = SERVO_CERRADO;    ///< Variable para almacenar la posición angular actual del servomotor. Se inicializa en la posición cerrada.
int servoAnguloObjetivo = SERVO_CERRADO;  ///< Variable para almacenar la posición angular a la que deseamos que el servo se mueva. Se inicializa en la posición cerrada.
unsigned long ultimoMovimientoServoMillis = 0; ///< Almacena el tiempo del último micropaso del servo para controlar su velocidad.
bool servoMoviendose = false;             ///< Bandera booleana que indica si el servo está actualmente en proceso de moverse hacia un nuevo ángulo objetivo.

// Modo de entrenamiento de voz
#define TIEMPO_ESPERA_ENTRENAMIENTO 8000  ///< Tiempo máximo en milisegundos que el sistema esperará a que el usuario diga una palabra para grabar.
#define TIEMPO_CONFIRMACION 5000          ///< Duración del parpadeo rápido inicial que confirma la entrada al modo entrenamiento.
#define INTERVALO_PARPADEO_RAPIDO 100     ///< Define el intervalo para el parpadeo rápido de confirmación.

bool modoEntrenamiento = false;           ///< Bandera booleana que controla si el sistema está en modo de operación normal o en modo de entrenamiento.
int registroActualEntrenamiento = 0;      ///< Variable que lleva la cuenta de cuál de los comandos se está entrenando en el momento actual.
unsigned long inicioEsperaEntrenamiento = 0; ///< Almacena el tiempo en que comienza la espera para que el usuario hable durante el entrenamiento.
bool confirmacionVisual = false;          ///< Bandera para controlar la fase inicial de parpadeo de confirmación.
String comandoSerial = "";                ///< Variable tipo String para acumular los caracteres recibidos desde el monitor serial.

// --- DECLARACIÓN DE FUNCIONES (PROTOTIPOS) ---
// Aquí se listan las "firmas" de todas las funciones que definiremos más adelante en el código.
// Esto permite que una función pueda llamar a otra aunque su definición completa se encuentre más abajo en el archivo. Es una práctica estándar en C++.
bool cargarRegistros();
void leerRespuestaModulo();
String obtenerSignaturaRegistro(int registro);
void manejarModoOperacionNormal();
void reproducirAudio(const char *ruta);
void iniciarModoEntrenamiento();
void manejarModoEntrenamiento();
void entrenarSiguienteRegistro(bool exito);
void finalizarModoEntrenamiento();
void actualizarParpadeo();
void ejecutarSalidaPulsada(int registro);
void actualizarSalidas();
void actualizarServo();
void iniciarMovimientoServoAbrir();
void iniciarMovimientoServoCerrar();
void mostrarInstruccionEntrenamiento();
void iniciarParpadeoEntrenamiento(int registro);
void iniciarParpadeo(int registro);

/**
 * @brief Devuelve la palabra (signatura) asociada a un número de registro. Funciona como un diccionario.
 * @param registro El índice numérico del comando de voz.
 * @retval La cadena de texto (String) asociada a ese comando.
 */
String obtenerSignaturaRegistro(int registro) {
  // La estructura 'switch' es una forma eficiente de comparar una variable con múltiples valores posibles.
  switch(registro) {
    case 0: return "Jarvis";
    case 1: return "Abrir";
    case 2: return "Cerrar";
    case 3: return "Diagnostico";
    case 4: return "Me Amas";
    case 5: return "Modo Ataque";
    case 6: return "Musica";
    case 7: return "Musica2";
    case 8: return "Reactor On";
    case 9: return "Comando Extra 1";
    case 10: return "Comando Extra 2";
    default: return ""; // Si el número de registro no corresponde a ningún caso, devuelve una cadena vacía.
  }
}

/**
 * @brief Envía los comandos de carga de registros al módulo de voz.
 * @details El módulo solo puede cargar 7 registros a la vez, por lo que se divide la carga en dos paquetes.
 * @retval Devuelve 'true', indicando que los comandos de envío fueron ejecutados.
 */
bool cargarRegistros() {
    // Se define el primer paquete de datos. El primer byte (0x30) es el código de comando para "cargar". Los siguientes son los índices a cargar.
    uint8_t bufferComando1[8] = {0x30, 0, 1, 2, 3, 4, 5, 6};
    Serial.println("Enviando paquete 1 para cargar registros 0-6..."); // Mensaje de depuración para el monitor serial.
    miReconocedor.send_pkt(bufferComando1, 8); // Se envía el paquete de 8 bytes al módulo de voz.
    delay(100); // Pequeña pausa para dar tiempo al módulo a procesar el comando.
    leerRespuestaModulo(); // Se verifica si el módulo respondió algo.

    // Se define y envía el segundo paquete con los registros restantes.
    uint8_t bufferComando2[5] = {0x30, 7, 8, 9, 10};
    Serial.println("Enviando paquete 2 para cargar registros 7-10...");
    miReconocedor.send_pkt(bufferComando2, 5);
    delay(100);
    leerRespuestaModulo();

    Serial.println("Carga de todos los registros enviada.");
    return true; // La función finaliza indicando éxito.
}

/**
 * @brief Lee e imprime cualquier respuesta proveniente del módulo de voz.
 * @details Se mantiene en un bucle 'while' para asegurarse de capturar respuestas fragmentadas en múltiples paquetes.
 */
void leerRespuestaModulo() {
  uint8_t bufferRespuesta[400]; // Buffer grande para almacenar la respuesta.
  uint8_t longitudesPaquetes[32]; // Array para guardar el tamaño de cada paquete recibido.
  int longitudTotal = 0; // Acumulador para el tamaño total de la respuesta.
  int indicePaquete = 0; // Contador de paquetes recibidos.
  int resultadoRecepcion; // Variable para almacenar el resultado de la función de recepción.
  
  // Bucle que intenta leer paquetes hasta que no haya más datos disponibles.
  while(true) {
    resultadoRecepcion = miReconocedor.receive_pkt(bufferRespuesta + longitudTotal, 50); // Intenta leer hasta 50 bytes.
    if(resultadoRecepcion > 0) { // Si se recibieron datos...
      longitudTotal += resultadoRecepcion; // Se actualiza la longitud total.
      longitudesPaquetes[indicePaquete] = resultadoRecepcion; // Se guarda el tamaño del paquete actual.
      indicePaquete++; // Se incrementa el contador de paquetes.
    } else { // Si no se recibió nada...
      break; // Se rompe el bucle.
    }
  }
  
  // Si se recibió al menos un paquete, se imprime su contenido.
  if(indicePaquete > 0) {
    longitudTotal = 0;
    // Bucle para recorrer e imprimir cada paquete individualmente.
    for(int i = 0; i < indicePaquete; i++) {
      Serial.print("< "); // Prefijo para indicar que es un dato de entrada.
      miReconocedor.writehex(bufferRespuesta + longitudTotal, longitudesPaquetes[i]); // Imprime el paquete en formato hexadecimal.
      longitudTotal += longitudesPaquetes[i];
      Serial.println(); // Salto de línea para mayor claridad.
    }
  } else {
    Serial.println("No se recibio respuesta del modulo."); // Mensaje si no hubo respuesta.
  }
}

/**
 * @brief Función de configuración principal. Se ejecuta una única vez al encender o reiniciar el ESP32.
 * @details Aquí se inicializan todas las comunicaciones, se configuran los pines y se preparan los componentes para la operación.
 */
void setup() {
  // Inicializa la comunicación serial con el ordenador a una velocidad de 115200 baudios para depuración.
  Serial.begin(115200);

  // Configura los pines digitales como SALIDAS (OUTPUT).
  pinMode(LED_INTERNO, OUTPUT);
  pinMode(SALIDA_0, OUTPUT);
  pinMode(SALIDA_1, OUTPUT);
  pinMode(SALIDA_2, OUTPUT);
  pinMode(SALIDA_REACTOR_ON, OUTPUT);

  // Establece el estado inicial de todas las salidas a BAJO (LOW), es decir, apagadas.
  digitalWrite(LED_INTERNO, LOW);
  digitalWrite(SALIDA_0, LOW);
  digitalWrite(SALIDA_1, LOW);
  digitalWrite(SALIDA_2, LOW);
  digitalWrite(SALIDA_REACTOR_ON, LOW);

  // Inicializa la comunicación serial con el módulo de voz.
  Serial.println("Inicializando modulo de reconocimiento de voz...");
  miReconocedor.begin(9600);
  Serial.println("Modulo de voz inicializado.");

  // Inicializa la comunicación con la tarjeta SD.
  Serial.println("Inicializando tarjeta SD...");
  if (!SD.begin(CS)) { // Intenta inicializar la SD usando el pin CS definido.
    Serial.println("Error: Tarjeta SD no encontrada o inicializacion fallida");
  } else {
    Serial.println("Tarjeta SD inicializada correctamente.");
  }

  // Configura la salida de audio I2S en modo monofónico.
  salidaI2S.SetOutputModeMono(true);
  
  // Asocia el objeto 'servo' con su pin físico.
  servo.attach(SERVO_PIN);
  // Mueve el servo a su posición cerrada inicial.
  servo.write(servoAnguloActual);
  // Inicializa el temporizador del servo.
  ultimoMovimientoServoMillis = millis();

  // Carga los registros de voz en el módulo.
  Serial.println("Cargando todos los registros de voz...");
  if (cargarRegistros()) {
     Serial.println("Comandos de carga de registros enviados.");
  } else {
     Serial.println("Error al enviar comandos de carga.");
  }

  // Imprime información de la versión y estado final en el monitor serial.
  Serial.print("Version del firmware: "); Serial.println(Version);
  Serial.println("Dispositivo listo. Esperando comandos...");
}

/**
 * @brief Bucle principal del programa. Se ejecuta continuamente después de la función setup().
 * @details Implementa una máquina de estados para gestionar las diferentes fases de operación:
 * entrenamiento, reproducción de audio, y escucha de comandos.
 */
void loop() {
  // --- TAREAS DE FONDO (ASÍNCRONAS) ---
  // Estas funciones se llaman en cada ciclo para actualizar estados que dependen del tiempo,
  // como parpadeos o movimientos suaves, sin bloquear la ejecución del resto del código.
  actualizarParpadeo();
  actualizarSalidas();
  actualizarServo();
  
  // --- GESTIÓN DE ENTRADA SERIAL ---
  // Revisa si hay datos disponibles en el puerto serial (enviados desde el monitor del PC).
  if (Serial.available()) {
      char c = Serial.read(); // Lee un caracter.
      if (c == '\n') { // Si el caracter es un salto de línea (se presionó Enter)...
          String comando = comandoSerial; // Se guarda el comando acumulado.
          comandoSerial = ""; // Se limpia la variable para el próximo comando.
          comando.trim(); // Se eliminan espacios en blanco al inicio y al final.
          if (comando.equalsIgnoreCase("entrenar")) { // Se compara el comando (ignorando mayúsculas/minúsculas).
              iniciarModoEntrenamiento(); // Si es "entrenar", se llama a la función correspondiente.
          }
      } else {
          comandoSerial += c; // Si no es Enter, se añade el caracter al comando.
      }
  }

  // --- CONTROLADOR DE ESTADO PRINCIPAL ---
  // Esta estructura 'if-else if-else' asegura que el programa solo esté en un estado a la vez.
  if (modoEntrenamiento) {
    // ESTADO 1: Grabando nuevos comandos de voz.
    manejarModoEntrenamiento();
  } else if (decodificadorMp3.isRunning()) {
    // ESTADO 2: Un audio se está reproduciendo. El programa se enfoca en esta tarea.
    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1); // Aumenta la prioridad de esta tarea para asegurar una reproducción fluida.
    yield(); // Cede tiempo de procesador a otras tareas de fondo (si las hubiera).
    if (!decodificadorMp3.loop()) { // Llama a la función que procesa un trozo del audio y devuelve 'false' si terminó.
      // El audio ha terminado.
      vTaskPrioritySet(NULL, tskIDLE_PRIORITY); // Restaura la prioridad normal de la tarea.
      decodificadorMp3.stop(); // Detiene el decodificador.
      fuenteAudio.close(); // Cierra el archivo en la tarjeta SD.
      Serial.println("Audio finalizado.");
      
      // Si había una canción en cola, la reproduce ahora.
      if (archivoSiguiente.length() > 0) {
          reproducirAudio(archivoSiguiente.c_str());
          archivoSiguiente = ""; // Limpia la cola.
      }
    }
  } else {
    // ESTADO 3: No hay audio en curso y no se está entrenando. El sistema está listo para escuchar un nuevo comando.
    manejarModoOperacionNormal();
  }
}

/**
 * @brief Gestiona la lógica de reconocer un comando y ejecutar la acción correspondiente.
 * @details Esta función es llamada únicamente cuando el sistema está en el estado de escucha.
 */
void manejarModoOperacionNormal() {
  // Pide al módulo de voz que intente reconocer un comando y guarde el resultado en 'buf'.
  int ret = miReconocedor.recognize(buf, 50);
  if (ret > 0) { // Si 'ret' es mayor que 0, significa que se reconoció un comando.
    int registro = buf[1]; // El segundo byte de la respuesta ('buf[1]') contiene el índice del comando reconocido.
    iniciarParpadeo(registro); // Inicia un parpadeo para dar feedback visual.
    
    // Estructura 'switch' para ejecutar un bloque de código diferente según el comando reconocido.
    switch (registro) {
      case 0: // Comando "Jarvis"
        Serial.println("Comando reconocido: Jarvis");
        ejecutarSalidaPulsada(registro); // Activa la salida digital correspondiente.
        reproducirAudio("/jarvis.mp3"); // Inicia la reproducción del audio de respuesta.
        break;
      case 1: // Comando "Abrir"
        Serial.println("Comando reconocido: Abrir mascara");
        ejecutarSalidaPulsada(registro);
        iniciarMovimientoServoAbrir(); // Inicia el movimiento del servo hacia la posición abierta.
        reproducirAudio("/abrir.mp3");
        break;
      case 2: // Comando "Cerrar"
        Serial.println("Comando reconocido: Cerrar mascara");
        ejecutarSalidaPulsada(registro);
        iniciarMovimientoServoCerrar(); // Inicia el movimiento del servo hacia la posición cerrada.
        reproducirAudio("/cerrar.mp3");
        break;
      case 3: // Comando "Diagnostico"
        Serial.println("Comando reconocido: Diagnostico");
        reproducirAudio("/audio_diagnostico.mp3");
        break;
      case 4: // Comando "Me Amas"
        Serial.println("Comando reconocido: Me Amas");
        reproducirAudio("/audio_me_amas.mp3");
        break;
      case 5: // Comando "Modo Ataque"
        Serial.println("Comando reconocido: Modo Ataque");
        reproducirAudio("/audio_modo_ataque.mp3");
        break;
      case 6: // Comando "Musica"
        Serial.println("Comando reconocido: Musica");
        archivoSiguiente = "/musica_1.mp3"; // Almacena la canción principal en la cola.
        reproducirAudio("/audio_pre_musica.mp3"); // Reproduce primero el audio de introducción.
        break;
      case 7: // Comando "Musica2"
        Serial.println("Comando reconocido: Musica2");
        archivoSiguiente = "/musica_2.mp3"; // Almacena la segunda canción en la cola.
        reproducirAudio("/audio_pre_musica.mp3"); // Reproduce la misma introducción.
        break;
      case 8: // Comando "Reactor On"
        Serial.println("Comando reconocido: Reactor On");
        ejecutarSalidaPulsada(registro);
        reproducirAudio("/audio_reactor_on.mp3");
        break;
      case 9:
        Serial.println("Comando reconocido: Comando Extra 1");
        break;
      case 10:
        Serial.println("Comando reconocido: Comando Extra 2");
        break;
      default:
        Serial.println("Registro de comando no reconocido");
    }
  }
}

/**
 * @brief Prepara el sistema para entrar en el modo de entrenamiento de voz.
 */
void iniciarModoEntrenamiento() {
  Serial.println(">>> Modo de entrenamiento activado <<<");
  modoEntrenamiento = true; // Activa la bandera principal del modo entrenamiento.
  registroActualEntrenamiento = 0; // Comienza el entrenamiento desde el primer registro (índice 0).
  confirmacionVisual = true; // Activa la fase de parpadeo de confirmación.
  parpadeando = false; // Se asegura de que no haya otro parpadeo en curso.
  ultimoParpadeoMillis = millis(); // Inicializa el temporizador para el parpadeo de confirmación.
  intervaloParpadeo = INTERVALO_PARPADEO_RAPIDO; // Establece el intervalo para un parpadeo rápido.
  Serial.println("Preparando para el entrenamiento...");
  Serial.println("Por favor, espere el parpadeo rapido del LED interno...");
}

/**
 * @brief Gestiona la lógica secuencial del modo entrenamiento.
 */
void manejarModoEntrenamiento() {
  // Fase 1: Parpadeo rápido de confirmación.
  if (confirmacionVisual) {
    if (millis() - ultimoParpadeoMillis >= TIEMPO_CONFIRMACION) { // Espera a que termine el tiempo de confirmación.
      confirmacionVisual = false; // Finaliza la fase de confirmación.
      Serial.println("Entrenamiento comenzando...");
      mostrarInstruccionEntrenamiento(); // Muestra la primera instrucción.
      inicioEsperaEntrenamiento = millis(); // Inicia el temporizador de espera para la voz del usuario.
      iniciarParpadeoEntrenamiento(registroActualEntrenamiento); // Inicia el parpadeo que indica que está escuchando.
    }
    return; // Sale de la función para esperar a que termine la confirmación.
  }
  
  // Fase 2: Espera y grabación.
  // Comprueba si se ha agotado el tiempo de espera.
  if (millis() - inicioEsperaEntrenamiento >= TIEMPO_ESPERA_ENTRENAMIENTO) {
    Serial.println("Tiempo de espera agotado. Intentelo de nuevo.");
    entrenarSiguienteRegistro(false); // Indica que el intento falló y reinicia para el mismo registro.
    return;
  }

  // Intenta entrenar el registro actual.
  String signaturaActual = obtenerSignaturaRegistro(registroActualEntrenamiento);
  int ret = miReconocedor.trainWithSignature(registroActualEntrenamiento, 
                                   (uint8_t*)signaturaActual.c_str(), 
                                   signaturaActual.length(), 
                                   buf);
  
  if (ret >= 0) { // Si el entrenamiento fue exitoso...
    Serial.print("Entrenamiento del registro ");
    Serial.print(registroActualEntrenamiento);
    Serial.print(" ('");
    Serial.print(signaturaActual);
    Serial.println("') exitoso!");
    entrenarSiguienteRegistro(true); // Pasa al siguiente registro.
  } else if (ret == -1) { // Si el entrenamiento falló...
    Serial.print("Fallo en el entrenamiento del registro "); 
    Serial.print(registroActualEntrenamiento);
    Serial.print(" ('");
    Serial.print(signaturaActual);
    Serial.println("'). Intentelo de nuevo.");
    entrenarSiguienteRegistro(false); // Reinicia para el mismo registro.
  }
}

/**
 * @brief Muestra en el monitor serial la instrucción para el usuario sobre qué palabra decir.
 */
void mostrarInstruccionEntrenamiento() {
  String signatura = obtenerSignaturaRegistro(registroActualEntrenamiento);
  Serial.print("Registro ");
  Serial.print(registroActualEntrenamiento);
  Serial.print(": Diga la palabra '");
  Serial.print(signatura);
  Serial.println("' cuando el LED se encienda.");
}

/**
 * @brief Decide si avanzar al siguiente registro a entrenar, repetir el actual o finalizar.
 * @param exito Booleano que indica si el entrenamiento anterior fue exitoso.
 */
void entrenarSiguienteRegistro(bool exito) {
  if (exito) { // Si el entrenamiento anterior fue exitoso...
      if (registroActualEntrenamiento < (NUM_REGISTROS - 1)) { // ...y si aún no hemos llegado al último registro...
          registroActualEntrenamiento++; // ...incrementamos el índice para pasar al siguiente.
          Serial.print("Preparando para entrenar el siguiente registro: ");
          Serial.println(registroActualEntrenamiento);
          delay(2000); // Pausa para que el usuario se prepare.
          iniciarParpadeoEntrenamiento(registroActualEntrenamiento);
          mostrarInstruccionEntrenamiento();
      } else { // ...si ya era el último registro...
          Serial.println("--- Entrenamiento de todos los registros finalizado con exito. ---");
          for(int i=0; i<NUM_REGISTROS; i++){ // Imprime un resumen de los comandos entrenados.
            Serial.print("- Registro "); Serial.print(i); Serial.print(": '");
            Serial.print(obtenerSignaturaRegistro(i)); Serial.println("'");
          }
          finalizarModoEntrenamiento(); // Finaliza el proceso.
          return;
      }
  } else { // Si el entrenamiento anterior falló...
      Serial.print("Reiniciando entrenamiento del registro ");
      Serial.println(registroActualEntrenamiento);
      delay(1000);
      iniciarParpadeoEntrenamiento(registroActualEntrenamiento); // ...se reinicia el proceso para el mismo registro.
      mostrarInstruccionEntrenamiento();
  }
  
  inicioEsperaEntrenamiento = millis(); // En ambos casos (exitoso o fallido), se reinicia el temporizador de espera.
}

/**
 * @brief Finaliza el modo entrenamiento y devuelve el sistema a operación normal.
 */
void finalizarModoEntrenamiento() {
  modoEntrenamiento = false; // Desactiva la bandera de entrenamiento.
  parpadeando = false; // Detiene cualquier parpadeo.
  digitalWrite(LED_INTERNO, LOW); // Asegura que el LED quede apagado.
  Serial.println("Saliendo del modo de entrenamiento. Volviendo al modo de reconocimiento.");
  delay(1000);
  
  Serial.println("Cargando registros entrenados para reconocimiento...");
  // Vuelve a cargar los registros en el módulo para que reconozca las nuevas voces grabadas.
  if (cargarRegistros()) {
    Serial.println("Registros cargados. El sistema esta listo.");
  } else {
    Serial.println("Error al cargar registros.");
  }
}

/**
 * @brief Configura e inicia el parpadeo del LED para la fase de entrenamiento.
 * @param registro El número de registro actual, usado para determinar el número de parpadeos.
 */
void iniciarParpadeoEntrenamiento(int registro) {
  parpadeosObjetivo = (registro + 1) * 2; // El número de parpadeos indica el número de registro.
  parpadeosRealizados = 0;
  estadoLed = false;
  digitalWrite(LED_INTERNO, LOW);
  parpadeando = true; // Activa la bandera para que 'actualizarParpadeo' haga su trabajo.
  ultimoParpadeoMillis = millis();
  intervaloParpadeo = 1000; // Intervalo lento para que sea fácil de contar.
}

/**
 * @brief Configura e inicia un parpadeo rápido como feedback de un comando reconocido.
 * @param registro El número de registro reconocido.
 */
void iniciarParpadeo(int registro) {
  parpadeosObjetivo = (registro + 1) * 2;
  parpadeosRealizados = 0;
  estadoLed = true;
  digitalWrite(LED_INTERNO, HIGH);
  parpadeando = true;
  ultimoParpadeoMillis = millis();
  intervaloParpadeo = 200; // Intervalo rápido.
}

/**
 * @brief Gestiona el parpadeo del LED de forma asíncrona (no bloqueante).
 * @details Se llama en cada ciclo del loop. Solo actúa si la bandera 'parpadeando' está activa.
 */
void actualizarParpadeo() {
  if (!parpadeando) return; // Si no hay que parpadear, la función termina inmediatamente.

  unsigned long ahora = millis(); // Obtiene el tiempo actual.
  if (ahora - ultimoParpadeoMillis >= intervaloParpadeo) { // Comprueba si ha pasado el tiempo definido para el intervalo.
    ultimoParpadeoMillis = ahora; // Actualiza el tiempo del último cambio.
    estadoLed = !estadoLed; // Invierte el estado del LED (de true a false o viceversa).
    digitalWrite(LED_INTERNO, estadoLed ? HIGH : LOW); // Aplica el nuevo estado al pin físico.
    parpadeosRealizados++; // Incrementa el contador.
    if (parpadeosRealizados >= parpadeosObjetivo) { // Si se alcanzó el número de parpadeos deseado...
        parpadeando = false; // ...se desactiva la bandera.
        digitalWrite(LED_INTERNO, LOW); // Se deja el LED apagado por seguridad.
    }
  }
}

/**
 * @brief Activa una salida digital por un breve período de tiempo (pulso).
 * @param registro El índice del registro que determina qué pin de salida activar.
 */
void ejecutarSalidaPulsada(int registro) {
  unsigned long ahora = millis();
  unsigned long tiempoApagado = ahora + 500; // Calcula el tiempo futuro (dentro de 500 ms) en el que se deberá apagar.

  switch (registro) {
    case 0:
      digitalWrite(SALIDA_0, HIGH); // Enciende la salida.
      salidaOffMillis[0] = tiempoApagado; // Almacena el tiempo de apagado en el array.
      break;
    case 1:
      digitalWrite(SALIDA_1, HIGH);
      salidaOffMillis[1] = tiempoApagado;
      break;
    case 2:
      digitalWrite(SALIDA_2, HIGH);
      salidaOffMillis[2] = tiempoApagado;
      break;
    case 8: // Reactor On
      digitalWrite(SALIDA_REACTOR_ON, HIGH);
      salidaOffMillis[8] = tiempoApagado;
      break;
  }
}

/**
 * @brief Revisa si alguna de las salidas activadas por pulso debe ser apagada.
 * @details Se llama en cada ciclo del loop para un control temporal preciso y no bloqueante.
 */
void actualizarSalidas() {
  unsigned long ahora = millis(); // Obtiene el tiempo actual.

  // Recorre el array de temporizadores.
  for(int i=0; i < NUM_REGISTROS; i++){
    // Si hay un tiempo de apagado programado (es decir, no es cero) y el tiempo actual ya lo ha superado...
    if (salidaOffMillis[i] && ahora >= salidaOffMillis[i]) {
      switch(i){ // ...apaga la salida correspondiente.
        case 0: digitalWrite(SALIDA_0, LOW); break;
        case 1: digitalWrite(SALIDA_1, LOW); break;
        case 2: digitalWrite(SALIDA_2, LOW); break;
        case 8: digitalWrite(SALIDA_REACTOR_ON, LOW); break;
      }
      salidaOffMillis[i] = 0; // Pone a cero el temporizador para que no se vuelva a apagar.
    }
  }
}

/**
 * @brief Establece el ángulo objetivo del servo a la posición de "abierto".
 */
void iniciarMovimientoServoAbrir() {
  servoAnguloObjetivo = SERVO_ABIERTO; // Establece el destino del movimiento a 90 grados.
  servoMoviendose = true; // Activa la bandera para que la función 'actualizarServo' comience a trabajar.
}

/**
 * @brief Establece el ángulo objetivo del servo a la posición de "cerrado".
 */
void iniciarMovimientoServoCerrar() {
  servoAnguloObjetivo = SERVO_CERRADO; // Establece el destino del movimiento a 0 grados.
  servoMoviendose = true; // Activa la bandera para que la función 'actualizarServo' comience a trabajar.
}

/**
 * @brief Mueve el servo un pequeño paso hacia su ángulo objetivo.
 * @details Se llama en cada ciclo del loop para crear un movimiento suave y no bloqueante.
 */
void actualizarServo() {
  if (!servoMoviendose) return; // Si no hay que moverse, no hace nada.

  unsigned long ahora = millis();
  // Comprueba si ya pasó el tiempo de espera mínimo entre pasos.
  if (ahora - ultimoMovimientoServoMillis < SERVO_STEP_MS) return;

  ultimoMovimientoServoMillis = ahora; // Actualiza el tiempo del último movimiento.
  
  // Compara la posición actual con la objetivo y se mueve un paso en la dirección correcta.
  if (servoAnguloActual < servoAnguloObjetivo) {
    servoAnguloActual = min(servoAnguloObjetivo, servoAnguloActual + SERVO_STEP_DEG); // 'min' evita pasarse del objetivo.
    servo.write(servoAnguloActual); // Envía el comando de nueva posición al servo.
  } else if (servoAnguloActual > servoAnguloObjetivo) {
    servoAnguloActual = max(servoAnguloObjetivo, servoAnguloActual - SERVO_STEP_DEG); // 'max' evita pasarse del objetivo.
    servo.write(servoAnguloActual);
  }

  // Si ya se ha alcanzado el objetivo, detiene el movimiento.
  if (servoAnguloActual == servoAnguloObjetivo) {
    servoMoviendose = false;
  }
}

/**
 * @brief Abre un archivo MP3 de la tarjeta SD y comienza su reproducción.
 * @param ruta La ruta completa del archivo en la tarjeta SD (ej: "/jarvis.mp3").
 */
void reproducirAudio(const char *ruta) {
  // Verifica si el archivo especificado realmente existe en la tarjeta SD.
  if (!SD.exists(ruta)) {
    Serial.print("Archivo no encontrado: ");
    Serial.println(ruta);
    return; // Termina la función si no existe.
  }

  // Intenta abrir el archivo.
  if (!fuenteAudio.open(ruta)) {
    Serial.print("Error al abrir el archivo: ");
    Serial.println(ruta);
    return; // Termina la función si no se pudo abrir.
  }

  Serial.print("Iniciando reproduccion de: ");
  Serial.println(ruta);
  // Inicia el decodificador, pasándole la fuente de datos (el archivo) y el objeto de salida (los pines I2S).
  decodificadorMp3.begin(&fuenteAudio, &salidaI2S);
}
