// ESP32 version mejorado en español con documentación Doxygen
#include "VoiceRecognitionV3.h"
#define Version "0.2"
// Definiciones de pines de comunicación con el módulo de voz
#define PIN_TX 17           ///< TX2 del ESP32 conectado al RX del módulo de voz
#define PIN_RX 16           ///< RX2 del ESP32 conectado al TX del módulo de voz
#define LED_INTERNO 2       ///< LED interno del ESP32 (GPIO2)
#define SALIDA_0 18         ///< Salida correspondiente al comando 'Jarvis'
#define SALIDA_1 19         ///< Salida correspondiente al comando 'Abrir'
#define SALIDA_2 20         ///< Salida correspondiente al comando 'Cerrar'

VR myVR(PIN_RX, PIN_TX);    ///< Instancia del módulo de reconocimiento de voz
uint8_t records[] = {0, 1, 2}; ///< Registros que se van a cargar
uint8_t buf[65];              ///< Buffer para almacenar datos recibidos

/**
 * @brief Configuración inicial del ESP32
 * Inicializa la comunicación serial, el módulo de voz, pines y carga los comandos.
 */
void setup() {
  Serial.begin(115200); ///< Inicializa la consola serial a 115200 baudios
  pinMode(LED_INTERNO, OUTPUT); ///< Configura el LED interno como salida
  pinMode(SALIDA_0, OUTPUT); ///< Configura la salida 0 como salida
  pinMode(SALIDA_1, OUTPUT); ///< Configura la salida 1 como salida
  pinMode(SALIDA_2, OUTPUT); ///< Configura la salida 2 como salida

  Serial.println("Inicializando módulo de reconocimiento de voz...");
  myVR.begin(9600);
    Serial.println("Módulo de voz inicializado: /n Verificar que parpadee el led amarillo del modulo.");


  // Cargar registros 0, 1 y 2
  myVR.load(records, 3);
  Serial.println("Registros cargados correctamente.");
 }
/**
 * @brief Bucle principal del programa
 * Escucha comandos de voz y ejecuta acciones según el comando reconocido.
 */
void loop() {
  int ret = myVR.recognize(buf, 50); ///< Intenta reconocer un comando con timeout 50 ms
  if (ret > 0) {
    int registro = buf[1]; ///< Extrae el número de registro reconocido
    switch (registro) {
      case 0:
        Serial.println("Comando reconocido: Jarvis");
        parpadearLed(registro);
        reproducir(registro);
        break;
      case 1:
        Serial.println("Comando reconocido: Abrir mascara");
        parpadearLed(registro);
        reproducir(registro);
        break;
      case 2:
        Serial.println("Comando reconocido: Cerrar mascara");
        parpadearLed(registro);
        reproducir(registro);
        break;
      default:
        Serial.println("Comando no reconocido");
    }
  }
}

/**
 * @brief Parpadea el LED interno del ESP32
 * @param veces Cantidad de veces que parpadeará el LED (igual al número de registro + 1)
 */
void parpadearLed(int veces) {
  for (int i = 0; i < veces + 1; i++) {
    digitalWrite(LED_INTERNO, HIGH); ///< Enciende el LED
    delay(200);                      ///< Espera 200 milisegundos
    digitalWrite(LED_INTERNO, LOW);  ///< Apaga el LED
    delay(200);                      ///< Espera 200 milisegundos
  }
}

/**
 * @brief Ejecuta la acción correspondiente al comando reconocido
 * @param registro Número del registro reconocido (0 = Jarvis, 1 = Abrir, 2 = Cerrar)
 */
void reproducir(int registro) {
  Serial.print("Ejecutando acción para registro: ");
  Serial.println(registro);

  // Apagar todas las salidas primero
  digitalWrite(SALIDA_0, LOW);
  digitalWrite(SALIDA_1, LOW);
  digitalWrite(SALIDA_2, LOW);

  // Activar la salida correspondiente
  switch (registro) {
    case 0:
      digitalWrite(SALIDA_0, HIGH); ///< Activa la salida para "Jarvis"
      break;
    case 1:
      digitalWrite(SALIDA_1, HIGH); ///< Activa la salida para "Abrir"
      break;
    case 2:
      digitalWrite(SALIDA_2, HIGH); ///< Activa la salida para "Cerrar"
      break;
  }

  delay(500); ///< Mantiene la salida activa durante medio segundo

  // Apagar nuevamente todas las salidas
  digitalWrite(SALIDA_0, LOW);
  digitalWrite(SALIDA_1, LOW);
  digitalWrite(SALIDA_2, LOW);
}