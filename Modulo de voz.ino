// --- LIBRERÍAS: LAS CAJAS DE HERRAMIENTAS ---
// Aquí le decimos a nuestro programa qué herramientas necesita para funcionar.
// Cada "#include" es como pedir prestada una caja de herramientas especializada.

#include "VoiceRecognitionV3.h"           ///< Esta es la herramienta principal para hablar con el módulo de reconocimiento de voz.
#include "AudioFileSourceSD.h"            ///< Herramienta para encontrar y abrir archivos de música en la tarjeta SD.
#include "AudioGeneratorMP3.h"            ///< Herramienta que sabe cómo "leer" y decodificar archivos MP3.
#include "AudioOutputI2SNoDAC.h"          ///< Herramienta para enviar el sonido a los altavoces a través de pines específicos del ESP32.
#include "FS.h"                           ///< Herramienta básica para que el ESP32 pueda manejar archivos (como en una computadora).
#include "SD.h"                           ///< Herramienta específica para comunicarse con la tarjeta de memoria SD.
#include "SPI.h"                          ///< Herramienta para un tipo de comunicación muy rápida que usa la tarjeta SD.
#include <ESP32Servo.h>                   ///< Herramienta para controlar servomotores, como el que mueve la máscara.

// --- CONFIGURACIÓN GENERAL ---
// Aquí definimos "apodos" o constantes para no tener que recordar números complicados.

#define Version "1.1"                     ///< Le ponemos un número de versión a nuestro código para saber en cuál estamos trabajando.

// --- CONFIGURACIÓN DE PINES ---
// Le decimos al ESP32 qué "patitas" (pines) usará para cada cosa.

// Pines para hablar con el módulo de voz
#define PIN_TX 17                         ///< El pin 17 será para "transmitir" (TX) datos al módulo de voz.
#define PIN_RX 16                         ///< El pin 16 será para "recibir" (RX) datos desde el módulo de voz.

// Pines de Entrada/Salida (I/O)
#define LED_INTERNO 2                     ///< El pin 2 controla el pequeño LED que viene integrado en la placa ESP32.
#define NUM_REGISTROS 11                  ///< Definimos que vamos a usar 11 comandos de voz en total (del 0 al 10).

// Pines de salida para activar cosas cuando se reconoce un comando
#define SALIDA_0 25                       ///< El pin 25 se activará con el comando "Jarvis".
#define SALIDA_1 26                       ///< El pin 26 se activará con el comando "Abrir".
#define SALIDA_2 27                       ///< El pin 27 se activará con el comando "Cerrar".
#define SALIDA_REACTOR_ON 33              ///< El pin 33 se activará con el comando "Reactor On".

// Pines para la tarjeta SD
#define CS 5                              ///< El pin 5 es el "Chip Select", para que el ESP32 sepa que quiere hablar con la SD.

// Pines y configuración del servomotor
#define SERVO_PIN 32                      ///< El pin 32 enviará las señales para mover el servomotor.
#define SERVO_NEUTRAL 90                  ///< Definimos que la posición de reposo del servo son 90 grados.
#define SERVO_STEP_DEG 1                  ///< El servo se moverá de 1 grado en 1 grado para que sea un movimiento suave.
#define SERVO_STEP_MS 15                  ///< Haremos una pequeña pausa de 15 milisegundos entre cada grado que se mueva.

// --- VARIABLES GLOBALES Y OBJETOS ---
// Aquí creamos las "cajas" (variables) donde guardaremos información y los "trabajadores" (objetos) que usarán las herramientas.

// Objeto para el reconocimiento de voz
VR miReconocedor(PIN_RX, PIN_TX);         ///< Creamos nuestro "trabajador" que se encargará de escuchar la voz, usando los pines que definimos.

// Arreglo (una lista) con los números de los comandos que vamos a usar
uint8_t registros[NUM_REGISTROS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; ///< Lista de todos los comandos que cargaremos en el módulo.
uint8_t buf[65];                          ///< Una "caja" temporal (buffer) para guardar los mensajes que nos envíe el módulo de voz.

// Objetos para reproducir audio
AudioGeneratorMP3 decodificadorMp3;       ///< Creamos el "trabajador" que convierte el MP3 en sonido.
AudioFileSourceSD fuenteAudio;            ///< Creamos el "trabajador" que busca y lee el archivo de la SD.
AudioOutputI2SNoDAC salidaI2S;            ///< Creamos el "trabajador" que envía el sonido a los altavoces.
bool reproduccionEnCurso = false;         ///< Una bandera (como un interruptor) que nos dice si está sonando algo o no.
String archivoAudioActual = "";           ///< Una "caja" para guardar el nombre del archivo que queremos reproducir.
String archivoSiguiente = "";             ///< Una "caja" para guardar el nombre del siguiente archivo en una secuencia (para la música).

// Variables para que el LED parpadee sin detener el resto del programa
bool parpadeando = false;                 ///< Bandera que nos dice si el LED debe estar parpadeando.
int parpadeosObjetivo = 0;                ///< Cuántas veces queremos que parpadee.
int parpadeosRealizados = 0;              ///< Cuántas veces ha parpadeado ya.
bool estadoLed = false;                   ///< Para saber si el LED está encendido (true) o apagado (false).
unsigned long ultimoParpadeoMillis = 0;   ///< Guarda la última vez que el LED cambió de estado (en milisegundos).
unsigned long intervaloParpadeo = 200;    ///< El tiempo entre un parpadeo y otro.

// Arreglo para controlar las salidas que se encienden por un momento (pulso)
unsigned long salidaOffMillis[NUM_REGISTROS] = {0}; ///< Una lista de "temporizadores" para saber cuándo apagar cada salida.

// Variables y objeto para mover el servo suavemente
Servo servo;                              ///< Creamos nuestro "trabajador" que controla el servomotor.
int servoAnguloActual = SERVO_NEUTRAL;    ///< "Caja" para guardar la posición actual del servo.
int servoAnguloObjetivo = SERVO_NEUTRAL;  ///< "Caja" para guardar la posición a la que queremos que llegue el servo.
unsigned long ultimoMovimientoServoMillis = 0; ///< Guarda la última vez que movimos el servo.
bool servoMoviendose = false;             ///< Bandera que nos dice si el servo está en movimiento.

// Variables para el modo de entrenamiento de voz
#define TIEMPO_ESPERA_ENTRENAMIENTO 8000  ///< Damos 8 segundos para que el usuario diga la palabra a grabar.
#define TIEMPO_CONFIRMACION 5000          ///< Tiempo que dura el parpadeo rápido al inicio del entrenamiento.
#define INTERVALO_PARPADEO_RAPIDO 100     ///< Intervalo del parpadeo rápido de confirmación.

bool modoEntrenamiento = false;           ///< Bandera para saber si estamos en modo normal o en modo entrenamiento.
int registroActualEntrenamiento = 0;      ///< Para saber qué comando estamos grabando en este momento.
unsigned long inicioEsperaEntrenamiento = 0;///< Guarda cuándo empezó la cuenta atrás para grabar.
bool confirmacionVisual = false;          ///< Bandera para controlar el parpadeo de confirmación inicial.
String comandoSerial = "";                ///< "Caja" para guardar texto que escribamos en el monitor serial (para iniciar el entrenamiento).
// Aumentar tamaño de buffers de audio
#define AUDIO_BUFFER_SIZE 2048 // En lugar de 1024 por defecto

// --- DECLARACIÓN DE FUNCIONES ---
// Es como un índice del libro. Le decimos al programa qué funciones existen antes de explicarlas en detalle.
// Así, una función puede "llamar" a otra aunque esté definida más abajo en el código.
bool cargarRegistros();
void leerRespuestaModulo();
String obtenerSignaturaRegistro(int registro);
void manejarModoOperacionNormal();
void reproducirAudio(const char *ruta);
void manejarReproduccionAudio();
void iniciarModoEntrenamiento();
void manejarModoEntrenamiento();
void entrenarSiguienteRegistro(bool exito);
void finalizarModoEntrenamiento();
void actualizarParpadeo();
void ejecutarSalidaPulsada(int registro);
void actualizarSalidas();
void actualizarServo();

/**
 * @brief Esta función es como un diccionario. Le das un número de registro và te devuelve la palabra que le corresponde.
 * @param registro El número del comando de voz (por ejemplo, 0).
 * @retval La palabra asociada a ese comando (por ejemplo, "Jarvis").
 */
String obtenerSignaturaRegistro(int registro) {
  switch(registro) { // Usamos un "switch" para elegir la palabra correcta según el número.
    case 0: return "Jarvis";
    case 1: return "Abrir";
    case 2: return "Cerrar";
    case 3: return "Diagnostico";
    case 4: return "Me Amas";
    case 5: return "Modo Ataque";
    case 6: return "Musica";
    case 7: return "Musica2";
    case 8: return "Reactor On";
    case 9: return "Comando Extra 1"; // Dejamos este como ejemplo para futuros comandos.
    case 10: return "Comando Extra 2"; // Y este también.
    default: return ""; // Si nos piden un número que no existe, no devolvemos nada.
  }
}

/**
 * @brief Esta función le dice al módulo de voz qué comandos queremos que escuche.
 * Como el módulo solo puede cargar 7 comandos a la vez, lo hacemos en dos partes (dos "paquetes").
 * @retval Devuelve 'true' para indicar que los mensajes de carga se enviaron.
 */
bool cargarRegistros() {
    // Paquete 1: Preparamos un mensaje para cargar los primeros 7 comandos (del 0 al 6).
    uint8_t bufferComando1[8] = {0x30, 0, 1, 2, 3, 4, 5, 6}; // 0x30 es el código para "cargar".
    Serial.println("Enviando paquete 1 para cargar registros 0-6..."); // Avisamos por el monitor serial.
    miReconocedor.send_pkt(bufferComando1, 8); // Enviamos el mensaje al módulo de voz.
    delay(100); // Esperamos un poquito para que el módulo lo procese.
    leerRespuestaModulo(); // Leemos si el módulo nos contestó algo.

    // Paquete 2: Preparamos otro mensaje para cargar el resto de comandos (del 7 al 10).
    uint8_t bufferComando2[5] = {0x30, 7, 8, 9, 10};
    Serial.println("Enviando paquete 2 para cargar registros 7-10...");
    miReconocedor.send_pkt(bufferComando2, 5); // Enviamos el segundo mensaje.
    delay(100);
    leerRespuestaModulo();

    Serial.println("Carga de todos los registros enviada."); // Avisamos que terminamos.
    return true; // Devolvemos 'true' para decir que todo salió bien.
}

/**
 * @brief Esta función se pone a la escucha para ver si el módulo de voz nos responde algo y lo muestra en pantalla.
 */
void leerRespuestaModulo() {
  uint8_t bufferRespuesta[400]; // Una "caja" grande para guardar la respuesta.
  uint8_t longitudesPaquetes[32]; // Para guardar el tamaño de cada trozo de la respuesta.
  int longitudTotal = 0; // Para saber el tamaño total de la respuesta.
  int indicePaquete = 0; // Para contar cuántos trozos de respuesta hemos recibido.
  int resultadoRecepcion; // Para guardar el resultado de la escucha.
  
  while(true) { // Bucle infinito para seguir escuchando hasta que no haya más mensajes.
    resultadoRecepcion = miReconocedor.receive_pkt(bufferRespuesta + longitudTotal, 50); // Intentamos recibir un trozo de mensaje.
    if(resultadoRecepcion > 0) { // Si recibimos algo...
      longitudTotal += resultadoRecepcion; // Sumamos su tamaño al total.
      longitudesPaquetes[indicePaquete] = resultadoRecepcion; // Guardamos el tamaño de este trozo.
      indicePaquete++; // Contamos un trozo más.
    } else { // Si no recibimos nada...
      break; // Salimos del bucle.
    }
  }
  
  if(indicePaquete > 0) { // Si hemos recibido al menos un trozo...
    longitudTotal = 0;
    for(int i = 0; i < indicePaquete; i++) { // Recorremos cada trozo recibido.
      Serial.print("< "); // Escribimos "<" para indicar que es un mensaje entrante.
      miReconocedor.writehex(bufferRespuesta + longitudTotal, longitudesPaquetes[i]); // Mostramos el mensaje en formato hexadecimal.
      longitudTotal += longitudesPaquetes[i];
      Serial.println(); // Hacemos un salto de línea.
    }
  } else { // Si no recibimos ninguna respuesta...
    Serial.println("No se recibio respuesta del modulo."); // Lo avisamos.
  }
}

/**
 * @brief La función SETUP es lo primero que se ejecuta cuando encendemos el ESP32. Es como la preparación antes de empezar a trabajar.
 */
void setup() {
  Serial.begin(115200); // Iniciamos la comunicación con la computadora para poder ver mensajes en el monitor serial.

  // Configuramos los pines que van a encender cosas (salidas).
  pinMode(LED_INTERNO, OUTPUT);
  pinMode(SALIDA_0, OUTPUT);
  pinMode(SALIDA_1, OUTPUT);
  pinMode(SALIDA_2, OUTPUT);
  pinMode(SALIDA_REACTOR_ON, OUTPUT);

  // Nos aseguramos de que todas las salidas empiecen apagadas.
  digitalWrite(LED_INTERNO, LOW);
  digitalWrite(SALIDA_0, LOW);
  digitalWrite(SALIDA_1, LOW);
  digitalWrite(SALIDA_2, LOW);
  digitalWrite(SALIDA_REACTOR_ON, LOW);

  // Inicializamos el módulo de voz.
  Serial.println("Inicializando modulo de reconocimiento de voz...");
  miReconocedor.begin(9600); // Empezamos a "hablar" con el módulo a una velocidad de 9600 baudios.
  Serial.println("Modulo de voz inicializado.");
  
  // Inicializamos la tarjeta SD.
  Serial.println("Inicializando tarjeta SD...");
  if (!SD.begin(CS)) { // Intentamos comunicarnos con la tarjeta SD.
    Serial.println("Error: Tarjeta SD no encontrada o inicializacion fallida"); // Si falla, avisamos.
  } else {
    Serial.println("Tarjeta SD inicializada correctamente."); // Si funciona, también avisamos.
  }
  salidaI2S.SetOutputModeMono(true); // Configuramos el sonido para que salga por un solo canal (mono).
  servo.attach(SERVO_PIN); // Le decimos a nuestro "trabajador" servo en qué pin está conectado el motor.
  servo.write(servoAnguloActual); // Movemos el servo a su posición inicial de reposo.
  ultimoMovimientoServoMillis = millis(); // Guardamos el tiempo actual.

  // Le decimos al módulo de voz qué palabras debe escuchar.
  Serial.println("Cargando todos los registros de voz...");
  if (cargarRegistros()) { // Llamamos a la función que carga los comandos.
     Serial.println("Comandos de carga de registros enviados.");
  } else {
     Serial.println("Error al enviar comandos de carga.");
  }

  Serial.print("Version del firmware: "); Serial.println(Version); // Mostramos la versión de nuestro código.
  Serial.println("Dispositivo listo. Esperando comandos..."); // Avisamos que ya está todo listo.
}

/**
 * @brief La función LOOP es el corazón del programa. Se repite una y otra vez, sin parar, mientras el ESP32 esté encendido.
 */
void loop() {
  // Primero, revisamos si hemos escrito algo en el monitor serial.
  if (Serial.available()) { // Si hay texto disponible...
      char c = Serial.read(); // Leemos una letra.
      if (c == '\n') { // Si la letra es un "Enter" (salto de línea)...
          String comando = comandoSerial; // Guardamos todo el texto que habíamos acumulado.
          comandoSerial = ""; // Limpiamos la "caja" para el próximo comando.
          comando.trim(); // Quitamos espacios en blanco inútiles.
          if (comando.equalsIgnoreCase("entrenar")) { // Si hemos escrito "entrenar"...
              iniciarModoEntrenamiento(); // ...empezamos el modo de entrenamiento de voz.
          }
      } else { // Si no es un "Enter"...
          comandoSerial += c; // ...seguimos guardando las letras en nuestra "caja".
      }
  }

  // Decidimos qué hacer: ¿entrenar o escuchar comandos?
  if (modoEntrenamiento) { // Si la bandera de entrenamiento está activada...
    manejarModoEntrenamiento(); // ...ejecutamos la lógica para grabar voces.
  } else { // Si no...
    manejarModoOperacionNormal(); // ...nos ponemos a escuchar comandos de voz.
  }

  // Estas son tareas que se hacen constantemente en segundo plano, sin importar el modo.
  actualizarParpadeo(); // Revisa si el LED tiene que parpadear.
  actualizarSalidas(); // Revisa si hay que apagar alguna salida.
  actualizarServo(); // Revisa si el servo tiene que moverse un poquito.
  manejarReproduccionAudio(); // Revisa si hay que reproducir o seguir reproduciendo un audio.
}

/**
 * @brief Esta función se encarga de escuchar y reaccionar a los comandos de voz cuando estamos en modo normal.
 */
void manejarModoOperacionNormal() {
  int ret = miReconocedor.recognize(buf, 50); // Le preguntamos al módulo si ha reconocido algo.
  if (ret > 0) { // Si la respuesta es positiva (ha oído algo que conoce)...
    int registro = buf[1]; // Vemos qué número de comando ha reconocido.
    iniciarParpadeo(registro); // Hacemos parpadear el LED para dar una señal visual.
    
    switch (registro) { // Usamos un "switch" para hacer una cosa diferente para cada comando.
      case 0: // Si reconoció el comando 0 ("Jarvis")
        Serial.println("Comando reconocido: Jarvis");
        ejecutarSalidaPulsada(registro); // Activamos la salida correspondiente por un momento.
        archivoAudioActual = "/jarvis.mp3"; // Preparamos el audio de respuesta.
        break;
      case 1: // Si reconoció el comando 1 ("Abrir")
        Serial.println("Comando reconocido: Abrir mascara");
        ejecutarSalidaPulsada(registro);
        archivoAudioActual = "/abrir.mp3";
        iniciarMovimientoServoAbrir(); // Le decimos al servo que empiece a moverse para abrir.
        break;
      case 2: // Si reconoció el comando 2 ("Cerrar")
        Serial.println("Comando reconocido: Cerrar mascara");
        ejecutarSalidaPulsada(registro);
        archivoAudioActual = "/cerrar.mp3";
        iniciarMovimientoServoCerrar(); // Le decimos al servo que empiece a moverse para cerrar.
        break;
      case 3: // "Diagnostico"
        Serial.println("Comando reconocido: Diagnostico");
        archivoAudioActual = "/audio_diagnostico.mp3";
        break;
      case 4: // "Me Amas"
        Serial.println("Comando reconocido: Me Amas");
        archivoAudioActual = "/audio_me_amas.mp3";
        break;
      case 5: // "Modo Ataque"
        Serial.println("Comando reconocido: Modo Ataque");
        archivoAudioActual = "/audio_modo_ataque.mp3";
        break;
      case 6: // "Musica"
        Serial.println("Comando reconocido: Musica");
        archivoAudioActual = "/audio_pre_musica.mp3"; // Primero, preparamos el audio de introducción.
        archivoSiguiente = "/musica_1.mp3"; // Y dejamos listo el archivo de música para después.
        break;
      case 7: // "Musica2"
        Serial.println("Comando reconocido: Musica2");
        archivoAudioActual = "/audio_pre_musica.mp3"; // Igual que antes, intro primero.
        archivoSiguiente = "/musica_2.mp3"; // Y luego la segunda canción.
        break;
      case 8: // "Reactor On"
        Serial.println("Comando reconocido: Reactor On");
        ejecutarSalidaPulsada(registro); // Activamos la salida digital del reactor.
        archivoAudioActual = "/audio_reactor_on.mp3";
        break;
      case 9: // "Comando Extra 1"
        Serial.println("Comando reconocido: Comando Extra 1");
        // Aquí podrías añadir qué quieres que haga este comando.
        break;
      case 10: // "Comando Extra 2"
        Serial.println("Comando reconocido: Comando Extra 2");
        // Y aquí también.
        break;
      default:
        Serial.println("Registro de comando no reconocido"); // Si por alguna razón no sabe qué comando es.
    }
  }
}

/**
 * @brief Prepara todo para empezar a grabar los comandos de voz.
 */
void iniciarModoEntrenamiento() {
  Serial.println(">>> Modo de entrenamiento activado <<<"); // Avisamos por el monitor.
  modoEntrenamiento = true; // Levantamos la bandera de entrenamiento.
  registroActualEntrenamiento = 0; // Empezamos a grabar desde el primer comando (el 0).
  confirmacionVisual = true; // Activamos la confirmación visual con parpadeos.
  parpadeando = false; // Nos aseguramos de que no haya otro parpadeo activo.
  ultimoParpadeoMillis = millis(); // Guardamos el tiempo actual.
  intervaloParpadeo = INTERVALO_PARPADEO_RAPIDO; // Ponemos el parpadeo en modo rápido.
  Serial.println("Preparando para el entrenamiento...");
  Serial.println("Por favor, espere el parpadeo rapido del LED interno...");
}

/**
 * @brief Gestiona todo el proceso mientras estamos en modo entrenamiento.
 */
void manejarModoEntrenamiento() {
  if (confirmacionVisual) { // Si estamos en la fase de parpadeo rápido...
    if (millis() - ultimoParpadeoMillis >= TIEMPO_CONFIRMACION) { // ...esperamos a que termine.
      confirmacionVisual = false; // Apagamos la bandera de confirmación.
      Serial.println("Entrenamiento comenzando...");
      mostrarInstruccionEntrenamiento(); // Mostramos en pantalla qué palabra hay que decir.
      inicioEsperaEntrenamiento = millis(); // Empezamos a contar el tiempo para que el usuario hable.
      iniciarParpadeoEntrenamiento(registroActualEntrenamiento); // Iniciamos el parpadeo normal que indica que está escuchando.
    }
    return; // Salimos de la función para esperar.
  }
  
  if (millis() - inicioEsperaEntrenamiento >= TIEMPO_ESPERA_ENTRENAMIENTO) { // Si se acabó el tiempo y el usuario no ha hablado...
    Serial.println("Tiempo de espera agotado. Intentelo de nuevo."); // ...avisamos.
    entrenarSiguienteRegistro(false); // ...y volvemos a intentarlo con el mismo comando.
    return;
  }

  String signaturaActual = obtenerSignaturaRegistro(registroActualEntrenamiento); // Buscamos qué palabra corresponde a este número de comando.
  // Le pedimos al módulo que grabe la voz y la asocie con la palabra.
  int ret = miReconocedor.trainWithSignature(registroActualEntrenamiento, 
                                   (uint8_t*)signaturaActual.c_str(), 
                                   signaturaActual.length(), 
                                   buf);
  
  if (ret >= 0) { // Si el módulo dice que la grabación fue un éxito...
    Serial.print("Entrenamiento del registro ");
    Serial.print(registroActualEntrenamiento);
    Serial.print(" ('");
    Serial.print(signaturaActual);
    Serial.println("') exitoso!");
    entrenarSiguienteRegistro(true); // ...pasamos al siguiente comando.
  } else if (ret == -1) { // Si dice que hubo un fallo...
    Serial.print("Fallo en el entrenamiento del registro "); 
    Serial.print(registroActualEntrenamiento);
    Serial.print(" ('");
    Serial.print(signaturaActual);
    Serial.println("'). Intentelo de nuevo.");
    entrenarSiguienteRegistro(false); // ...lo volvemos a intentar.
  }
}

/**
 * @brief Muestra en el monitor serial las instrucciones para el usuario sobre qué palabra decir.
 */
void mostrarInstruccionEntrenamiento() {
  String signatura = obtenerSignaturaRegistro(registroActualEntrenamiento); // Obtenemos la palabra a grabar.
  Serial.print("Registro ");
  Serial.print(registroActualEntrenamiento);
  Serial.print(": Diga la palabra '");
  Serial.print(signatura);
  Serial.println("' cuando el LED se encienda.");
}

/**
 * @brief Decide si pasar al siguiente comando a grabar o si ya hemos terminado.
 * @param exito Nos dice si la última grabación fue exitosa o no.
 */
void entrenarSiguienteRegistro(bool exito) {
  if (exito) { // Si la grabación fue exitosa...
      if (registroActualEntrenamiento < (NUM_REGISTROS - 1)) { // ...y si todavía no hemos grabado todos los comandos...
          registroActualEntrenamiento++; // ...pasamos al siguiente.
          Serial.print("Preparando para entrenar el siguiente registro: ");
          Serial.println(registroActualEntrenamiento);
          delay(2000); // Esperamos 2 segundos para dar tiempo al usuario.
          iniciarParpadeoEntrenamiento(registroActualEntrenamiento); // Iniciamos el parpadeo de escucha.
          mostrarInstruccionEntrenamiento(); // Mostramos la nueva instrucción.
      } else { // ...pero si ya era el último comando...
          Serial.println("--- Entrenamiento de todos los registros finalizado con exito. ---");
          for(int i=0; i<NUM_REGISTROS; i++){ // Mostramos un resumen de todo lo que hemos grabado.
            Serial.print("- Registro "); Serial.print(i); Serial.print(": '");
            Serial.print(obtenerSignaturaRegistro(i)); Serial.println("'");
          }
          finalizarModoEntrenamiento(); // ...damos por terminado el entrenamiento.
          return;
      }
  } else { // Si la grabación falló...
      Serial.print("Reiniciando entrenamiento del registro ");
      Serial.println(registroActualEntrenamiento);
      delay(1000); // Esperamos 1 segundo.
      iniciarParpadeoEntrenamiento(registroActualEntrenamiento); // Y volvemos a empezar con el mismo comando.
      mostrarInstruccionEntrenamiento();
  }
  
  inicioEsperaEntrenamiento = millis(); // Reiniciamos el temporizador de espera.
}

/**
 * @brief Termina el modo entrenamiento y vuelve al modo normal de escucha.
 */
void finalizarModoEntrenamiento() {
  modoEntrenamiento = false; // Bajamos la bandera de entrenamiento.
  parpadeando = false; // Detenemos cualquier parpadeo.
  digitalWrite(LED_INTERNO, LOW); // Nos aseguramos de que el LED quede apagado.
  Serial.println("Saliendo del modo de entrenamiento. Volviendo al modo de reconocimiento.");
  delay(1000);
  
  Serial.println("Cargando registros entrenados para reconocimiento...");
  if (cargarRegistros()) { // Volvemos a cargar los comandos en el módulo, ahora con las nuevas grabaciones.
    Serial.println("Registros cargados. El sistema esta listo.");
  } else {
    Serial.println("Error al cargar registros.");
  }
}

/**
 * @brief Inicia el parpadeo del LED para indicar que está escuchando durante el entrenamiento.
 * El número de parpadeos indica el número de registro que se está grabando.
 */
void iniciarParpadeoEntrenamiento(int registro) {
  parpadeosObjetivo = (registro + 1) * 2; // Queremos que parpadee una vez por el registro 0, dos por el 1, etc.
  parpadeosRealizados = 0;
  estadoLed = false;
  digitalWrite(LED_INTERNO, LOW);
  parpadeando = true;
  ultimoParpadeoMillis = millis();
  intervaloParpadeo = 1000; // Un parpadeo lento, de 1 segundo.
}

/**
 * @brief Inicia un parpadeo rápido para confirmar que un comando ha sido reconocido.
 */
void iniciarParpadeo(int registro) {
  parpadeosObjetivo = (registro + 1) * 2;
  parpadeosRealizados = 0;
  estadoLed = true;
  digitalWrite(LED_INTERNO, HIGH);
  parpadeando = true;
  ultimoParpadeoMillis = millis();
  intervaloParpadeo = 200; // Un parpadeo más rápido.
}

/**
 * @brief Esta función, que se llama constantemente desde el loop, se encarga de hacer que el LED parpadee sin detener el resto del programa.
 */
void actualizarParpadeo() {
  if (!parpadeando) return; // Si no hay que parpadear, no hacemos nada.

  unsigned long ahora = millis(); // Miramos la hora actual.
  if (ahora - ultimoParpadeoMillis >= intervaloParpadeo) { // Si ya ha pasado el tiempo del intervalo...
    ultimoParpadeoMillis = ahora; // ...actualizamos la hora del último cambio.
    estadoLed = !estadoLed; // ...cambiamos el estado del LED (si estaba encendido, lo apagamos, y viceversa).
    digitalWrite(LED_INTERNO, estadoLed ? HIGH : LOW); // ...aplicamos el nuevo estado al pin del LED.
    parpadeosRealizados++; // ...contamos un parpadeo más.
    if (parpadeosRealizados >= parpadeosObjetivo) { // Si ya hemos parpadeado las veces que queríamos...
        parpadeando = false; // ...bajamos la bandera de parpadeo.
        digitalWrite(LED_INTERNO, LOW); // ...y dejamos el LED apagado por si acaso.
    }
  }
}

/**
 * @brief Enciende una salida y la deja encendida solo por medio segundo (500 ms).
 */
void ejecutarSalidaPulsada(int registro) {
  unsigned long ahora = millis();
  unsigned long tiempoApagado = ahora + 500; // Calculamos en qué momento del futuro hay que apagarla.

  switch (registro) { // Activamos la salida que corresponda al comando.
    case 0:
      digitalWrite(SALIDA_0, HIGH);
      salidaOffMillis[0] = tiempoApagado; // Guardamos la hora de apagado para la salida 0.
      break;
    case 1:
      digitalWrite(SALIDA_1, HIGH);
      salidaOffMillis[1] = tiempoApagado; // Guardamos la hora de apagado para la salida 1.
      break;
    case 2:
      digitalWrite(SALIDA_2, HIGH);
      salidaOffMillis[2] = tiempoApagado; // Guardamos la hora de apagado para la salida 2.
      break;
    case 8: // Reactor On
      digitalWrite(SALIDA_REACTOR_ON, HIGH);
      salidaOffMillis[8] = tiempoApagado; // Guardamos la hora de apagado para la salida del reactor.
      break;
  }
}

/**
 * @brief Revisa constantemente si alguna de las salidas que encendimos tiene que ser apagada ya.
 */
void actualizarSalidas() {
  unsigned long ahora = millis(); // Miramos la hora actual.

  // Recorremos nuestra lista de "temporizadores".
  for(int i=0; i < NUM_REGISTROS; i++){
    if (salidaOffMillis[i] && ahora >= salidaOffMillis[i]) { // Si hay un tiempo de apagado guardado y ya hemos llegado a esa hora...
      switch(i){ // ...apagamos la salida correspondiente.
        case 0: digitalWrite(SALIDA_0, LOW); break;
        case 1: digitalWrite(SALIDA_1, LOW); break;
        case 2: digitalWrite(SALIDA_2, LOW); break;
        case 8: digitalWrite(SALIDA_REACTOR_ON, LOW); break;
      }
      salidaOffMillis[i] = 0; // Y ponemos el temporizador a cero para que no vuelva a apagarse.
    }
  }
}

/**
 * @brief Le dice al servo que su nuevo objetivo es la posición de "abierto".
 */
void iniciarMovimientoServoAbrir() {
  servoAnguloObjetivo = constrain(SERVO_NEUTRAL + 60, 0, 180); // Calculamos el ángulo objetivo.
  servoMoviendose = true; // Levantamos la bandera para que empiece a moverse.
  ultimoMovimientoServoMillis = millis();
}

/**
 * @brief Le dice al servo que su nuevo objetivo es la posición de "cerrado".
 */
void iniciarMovimientoServoCerrar() {
  servoAnguloObjetivo = constrain(SERVO_NEUTRAL - 60, 0, 180);
  servoMoviendose = true;
  ultimoMovimientoServoMillis = millis();
}

/**
 * @brief Mueve el servo un poquito cada vez, para que el movimiento sea suave y no bloquee el programa.
 */
void actualizarServo() {
  if (!servoMoviendose) return; // Si no hay que moverse, no hacemos nada.

  unsigned long ahora = millis();
  if (ahora - ultimoMovimientoServoMillis < SERVO_STEP_MS) return; // Si no ha pasado el tiempo suficiente, esperamos.

  ultimoMovimientoServoMillis = ahora; // Actualizamos la hora del último movimiento.
  if (servoAnguloActual < servoAnguloObjetivo) { // Si todavía no hemos llegado al objetivo...
    servoAnguloActual = min(servoAnguloObjetivo, servoAnguloActual + SERVO_STEP_DEG); // ...nos movemos un gradito más cerca.
    servo.write(servoAnguloActual); // Le enviamos la nueva posición al motor.
  } else if (servoAnguloActual > servoAnguloObjetivo) { // Lo mismo si nos estamos moviendo en la otra dirección.
    servoAnguloActual = max(servoAnguloObjetivo, servoAnguloActual - SERVO_STEP_DEG);
    servo.write(servoAnguloActual);
  }

  if (servoAnguloActual == servoAnguloObjetivo) { // Si ya hemos llegado al destino...
    servoMoviendose = false; // ...bajamos la bandera de movimiento.
  }
}

/**
 * @brief Gestiona la reproducción de audio, dándole prioridad para que no se corte.
 */
void manejarReproduccionAudio() {
  if (decodificadorMp3.isRunning()) { // Si hay un audio sonando...
     // Dar máxima prioridad al audio
    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);
    yield(); // ...le damos prioridad al proceso de audio para que no se trabe.
    if (!decodificadorMp3.loop()) { // ...y si el decodificador nos dice que ya terminó el archivo...
         // Restaurar prioridad normal
      vTaskPrioritySet(NULL, tskIDLE_PRIORITY);
      decodificadorMp3.stop(); // ...detenemos todo.
      fuenteAudio.close(); // ...cerramos el archivo en la tarjeta SD.
      reproduccionEnCurso = false; // ...bajamos la bandera de reproducción.
      Serial.println("Audio detenido y archivo cerrado.");
      
      // Si el servo se había movido, lo devolvemos a su posición neutral.
      if(servoAnguloActual != SERVO_NEUTRAL){
        servoAnguloObjetivo = SERVO_NEUTRAL;
        servoMoviendose = true;
      }

      // Revisamos si había una canción esperando en la cola (para la secuencia de música).
      if (archivoSiguiente.length() > 0) {
          reproducirAudio(archivoSiguiente.c_str()); // Si la hay, la reproducimos.
          archivoSiguiente = ""; // Y limpiamos la cola.
      }
    }
  } else { // Si no hay nada sonando...
    if (!reproduccionEnCurso && archivoAudioActual.length() > 0) { // ...pero tenemos un archivo preparado para reproducir...
      reproducirAudio(archivoAudioActual.c_str()); // ...lo empezamos a reproducir.
      archivoAudioActual = ""; // Y limpiamos la variable para que no se reproduzca otra vez.
    }
  }
}

/**
 * @brief Abre un archivo MP3 de la tarjeta SD y lo empieza a reproducir.
 * @param ruta El nombre del archivo que queremos reproducir (ej: "/jarvis.mp3").
 */
void reproducirAudio(const char *ruta) {
  if (reproduccionEnCurso) { // Si ya hay algo sonando, no hacemos nada para evitar problemas.
      Serial.println("Advertencia: Se intento reproducir un audio mientras otro estaba en curso.");
      return;
  }

  if (!SD.exists(ruta)) { // Comprobamos si el archivo existe en la tarjeta SD.
    Serial.print("Archivo no encontrado: ");
    Serial.println(ruta);
    return; // Si no existe, salimos.
  }

  if (!fuenteAudio.open(ruta)) { // Intentamos abrir el archivo.
    Serial.print("Error al abrir el archivo: ");
    Serial.println(ruta);
    return; // Si no se puede abrir, salimos.
  }

  Serial.print("Iniciando reproduccion de: ");
  Serial.println(ruta);
  reproduccionEnCurso = true; // Levantamos la bandera de reproducción.
  decodificadorMp3.begin(&fuenteAudio, &salidaI2S); // Y le damos el archivo a nuestro "trabajador" decodificador para que empiece la magia.
}