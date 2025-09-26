// --- LIBRERÍAS: LAS CAJAS DE HERRAMIENTAS ---
#include "VoiceRecognitionV3.h"           ///< Herramienta principal para el módulo de reconocimiento de voz.
#include "AudioFileSourceSD.h"            ///< Herramienta para encontrar y abrir archivos de música en la tarjeta SD.
#include "AudioGeneratorMP3.h"            ///< Herramienta que sabe cómo "leer" y decodificar archivos MP3.
#include "AudioOutputI2SNoDAC.h"          ///< Herramienta para enviar el sonido a los altavoces.
#include "FS.h"                           ///< Herramienta básica para manejar archivos.
#include "SD.h"                           ///< Herramienta específica para la tarjeta SD.
#include "SPI.h"                          ///< Herramienta para comunicación rápida con la SD.
#include <ESP32Servo.h>                   ///< Herramienta para controlar servomotores.

// --- CONFIGURACIÓN GENERAL ---
#define Version "1.3"                     ///< Versión mejorada del código.

// --- CONFIGURACIÓN DE PINES ---
// Módulo de voz
#define PIN_TX 17                         ///< Pin para transmitir (TX) al módulo de voz.
#define PIN_RX 16                         ///< Pin para recibir (RX) desde el módulo de voz.
// I/O
#define LED_INTERNO 2                     ///< Pin del LED integrado en la placa ESP32.
#define NUM_REGISTROS 11                  ///< Total de comandos de voz (0 al 10).
// Salidas
#define SALIDA_0 25                       ///< Pin para el comando "Jarvis".
#define SALIDA_1 26                       ///< Pin para el comando "Abrir".
#define SALIDA_2 27                       ///< Pin para el comando "Cerrar".
#define SALIDA_REACTOR_ON 33              ///< Pin para el comando "Reactor On".
// Tarjeta SD
#define CS 5                              ///< Pin "Chip Select" para la tarjeta SD.
// Servomotor
#define SERVO_PIN 32                      ///< Pin de señales para el servomotor.
#define SERVO_NEUTRAL 90                  ///< Posición de reposo del servo (90 grados).
#define SERVO_STEP_DEG 1                  ///< Movimiento suave, de 1 grado a la vez.
#define SERVO_STEP_MS 15                  ///< Pausa de 15 ms entre cada grado de movimiento.

// --- VARIABLES GLOBALES Y OBJETOS ---
// Reconocimiento de voz
VR miReconocedor(PIN_RX, PIN_TX);         ///< Objeto que gestiona la escucha de voz.
uint8_t registros[NUM_REGISTROS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; ///< Lista de comandos a cargar.
uint8_t buf[65];                          ///< Buffer para mensajes del módulo de voz.

// Reproducción de audio
AudioGeneratorMP3 decodificadorMp3;       ///< Objeto que convierte MP3 en sonido.
AudioFileSourceSD fuenteAudio;            ///< Objeto que lee el archivo desde la SD.
AudioOutputI2SNoDAC salidaI2S;            ///< Objeto que envía el sonido a los altavoces.
String archivoSiguiente = "";             ///< Almacena el siguiente archivo en una secuencia (para música).

// Parpadeo de LED
bool parpadeando = false;                 ///< Bandera que indica si el LED debe parpadear.
int parpadeosObjetivo = 0;                ///< Número de parpadeos deseados.
int parpadeosRealizados = 0;              ///< Contador de parpadeos.
bool estadoLed = false;                   ///< Estado actual del LED (encendido/apagado).
unsigned long ultimoParpadeoMillis = 0;   ///< Momento del último cambio de estado del LED.
unsigned long intervaloParpadeo = 200;    ///< Tiempo entre parpadeos.

// Salidas por pulso
unsigned long salidaOffMillis[NUM_REGISTROS] = {0}; ///< Temporizadores para apagar cada salida.

// Movimiento del servo
Servo servo;                              ///< Objeto que controla el servomotor.
int servoAnguloActual = SERVO_NEUTRAL;    ///< Posición actual del servo.
int servoAnguloObjetivo = SERVO_NEUTRAL;  ///< Posición a la que debe llegar el servo.
unsigned long ultimoMovimientoServoMillis = 0; ///< Momento del último movimiento del servo.
bool servoMoviendose = false;             ///< Bandera que indica si el servo está en movimiento.

// Modo de entrenamiento
#define TIEMPO_ESPERA_ENTRENAMIENTO 8000  ///< 8 segundos para decir la palabra a grabar.
#define TIEMPO_CONFIRMACION 5000          ///< Duración del parpadeo rápido inicial.
#define INTERVALO_PARPADEO_RAPIDO 100     ///< Intervalo del parpadeo de confirmación.

bool modoEntrenamiento = false;           ///< Bandera para el modo entrenamiento.
int registroActualEntrenamiento = 0;      ///< Comando que se está grabando actualmente.
unsigned long inicioEsperaEntrenamiento = 0; ///< Momento de inicio de la cuenta atrás para grabar.
bool confirmacionVisual = false;          ///< Bandera para el parpadeo de confirmación.
String comandoSerial = "";                ///< Almacena texto del monitor serial para iniciar el entrenamiento.


// --- DECLARACIÓN DE FUNCIONES ---
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


// ... (El resto de funciones como obtenerSignaturaRegistro, cargarRegistros, leerRespuestaModulo, etc., permanecen sin cambios) ...
// ... (Se omiten por brevedad, ya que no fueron modificadas) ...

void setup() {
  Serial.begin(115200);
  pinMode(LED_INTERNO, OUTPUT);
  pinMode(SALIDA_0, OUTPUT);
  pinMode(SALIDA_1, OUTPUT);
  pinMode(SALIDA_2, OUTPUT);
  pinMode(SALIDA_REACTOR_ON, OUTPUT);
  digitalWrite(LED_INTERNO, LOW);
  digitalWrite(SALIDA_0, LOW);
  digitalWrite(SALIDA_1, LOW);
  digitalWrite(SALIDA_2, LOW);
  digitalWrite(SALIDA_REACTOR_ON, LOW);

  Serial.println("Inicializando modulo de reconocimiento de voz...");
  miReconocedor.begin(9600);
  Serial.println("Modulo de voz inicializado.");

  Serial.println("Inicializando tarjeta SD...");
  if (!SD.begin(CS)) {
    Serial.println("Error: Tarjeta SD no encontrada o inicializacion fallida");
  } else {
    Serial.println("Tarjeta SD inicializada correctamente.");
  }
  salidaI2S.SetOutputModeMono(true);
  servo.attach(SERVO_PIN);
  servo.write(servoAnguloActual);
  ultimoMovimientoServoMillis = millis();

  Serial.println("Cargando todos los registros de voz...");
  if (cargarRegistros()) {
     Serial.println("Comandos de carga de registros enviados.");
  } else {
     Serial.println("Error al enviar comandos de carga.");
  }

  Serial.print("Version del firmware: "); Serial.println(Version);
  Serial.println("Dispositivo listo. Esperando comandos...");
}

/**
 * @brief La función LOOP es el corazón del programa. Se repite sin parar.
 * Se ha reestructurado para gestionar los estados:
 * 1. Modo Entrenamiento.
 * 2. Reproduciendo Audio (ignora nuevos comandos).
 * 3. Esperando Comando (solo si no hay audio en curso).
 */
void loop() {
  // Tareas de fondo que siempre deben ejecutarse
  actualizarParpadeo();
  actualizarSalidas();
  actualizarServo();
  
  // Revisa si se ha escrito "entrenar" en el monitor serial
  if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') {
          String comando = comandoSerial;
          comandoSerial = "";
          comando.trim();
          if (comando.equalsIgnoreCase("entrenar")) {
              iniciarModoEntrenamiento();
          }
      } else {
          comandoSerial += c;
      }
  }

  // Controlador de Estado Principal
  if (modoEntrenamiento) {
    // ESTADO 1: Grabando nuevos comandos de voz
    manejarModoEntrenamiento();
  } else if (decodificadorMp3.isRunning()) {
    // ESTADO 2: Un audio se está reproduciendo
    // Le damos prioridad a la tarea de audio y gestionamos su finalización
    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);
    yield();
    if (!decodificadorMp3.loop()) {
      // El audio ha terminado
      vTaskPrioritySet(NULL, tskIDLE_PRIORITY);
      decodificadorMp3.stop();
      fuenteAudio.close();
      Serial.println("Audio finalizado.");
      
      // Si el servo se movió, lo devolvemos a su posición neutral.
      if(servoAnguloActual != SERVO_NEUTRAL){
        servoAnguloObjetivo = SERVO_NEUTRAL;
        servoMoviendose = true;
      }

      // Si había una canción en cola (secuencia de música), la reproduce
      if (archivoSiguiente.length() > 0) {
          reproducirAudio(archivoSiguiente.c_str());
          archivoSiguiente = "";
      }
    }
  } else {
    // ESTADO 3: No hay audio en curso, estamos listos para escuchar un comando
    manejarModoOperacionNormal();
  }
}

/**
 * @brief Escucha y reacciona a los comandos de voz.
 * Esta función ahora es llamada ÚNICAMENTE cuando no hay audio en reproducción.
 */
void manejarModoOperacionNormal() {
  int ret = miReconocedor.recognize(buf, 50);
  if (ret > 0) {
    int registro = buf[1];
    iniciarParpadeo(registro);
    switch (registro) {
      case 0: // "Jarvis"
        Serial.println("Comando reconocido: Jarvis");
        ejecutarSalidaPulsada(registro);
        reproducirAudio("/jarvis.mp3");
        break;
      case 1: // "Abrir"
        Serial.println("Comando reconocido: Abrir mascara");
        ejecutarSalidaPulsada(registro);
        iniciarMovimientoServoAbrir();
        reproducirAudio("/abrir.mp3");
        break;
      case 2: // "Cerrar"
        Serial.println("Comando reconocido: Cerrar mascara");
        ejecutarSalidaPulsada(registro);
        iniciarMovimientoServoCerrar();
        reproducirAudio("/cerrar.mp3");
        break;
      case 3: // "Diagnostico"
        Serial.println("Comando reconocido: Diagnostico");
        reproducirAudio("/audio_diagnostico.mp3");
        break;
      case 4: // "Me Amas"
        Serial.println("Comando reconocido: Me Amas");
        reproducirAudio("/audio_me_amas.mp3");
        break;
      case 5: // "Modo Ataque"
        Serial.println("Comando reconocido: Modo Ataque");
        reproducirAudio("/audio_modo_ataque.mp3");
        break;
      case 6: // "Musica"
        Serial.println("Comando reconocido: Musica");
        archivoSiguiente = "/musica_1.mp3"; // Prepara la canción para después
        reproducirAudio("/audio_pre_musica.mp3"); // Reproduce la introducción
        break;
      case 7: // "Musica2"
        Serial.println("Comando reconocido: Musica2");
        archivoSiguiente = "/musica_2.mp3"; // Prepara la segunda canción
        reproducirAudio("/audio_pre_musica.mp3"); // Reproduce la introducción
        break;
      case 8: // "Reactor On"
        Serial.println("Comando reconocido: Reactor On");
        ejecutarSalidaPulsada(registro);
        reproducirAudio("/audio_reactor_on.mp3");
        break;
      // ... otros casos
      default:
        Serial.println("Registro de comando no reconocido");
    }
  }
}


/**
 * @brief Abre un archivo MP3 y lo reproduce.
 * Función simplificada: ya no necesita comprobar si algo más se está reproduciendo.
 * @param ruta El nombre del archivo a reproducir (ej: "/jarvis.mp3").
 */
void reproducirAudio(const char *ruta) {
  if (!SD.exists(ruta)) {
    Serial.print("Archivo no encontrado: ");
    Serial.println(ruta);
    return;
  }

  if (!fuenteAudio.open(ruta)) {
    Serial.print("Error al abrir el archivo: ");
    Serial.println(ruta);
    return;
  }

  Serial.print("Iniciando reproduccion de: ");
  Serial.println(ruta);
  decodificadorMp3.begin(&fuenteAudio, &salidaI2S);
}