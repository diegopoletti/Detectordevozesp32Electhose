Proyecto de Control por Voz con ESP32 y Reconocimiento de Voz VR3
Introducci車n
Este proyecto implementa un sistema de control por voz utilizando un m車dulo ESP32, un m車dulo de reconocimiento de voz VR3 de ELECHOUSE, una tarjeta SD para almacenamiento de archivos de audio, y un servomotor para control f赤sico. El sistema permite ejecutar comandos como "Abrir" y "Cerrar" mediante reconocimiento de voz, con capacidad de entrenamiento personalizado y retroalimentaci車n auditiva.

Componentes Requeridos
ESP32 (placa de desarrollo)

M車dulo de reconocimiento de voz VR3 (ELECHOUSE V3)

M車dulo lector de tarjetas SD

Servomotor (SG90 o similar)

LED indicador

Resistencias (220次 para LED)

Cables de conexi車n

Tarjeta microSD (formateada en FAT32)

Fuente de alimentaci車n (5V/2A recomendado)

Diagrama de Conexiones
Diagram
Code
graph TD
    subgraph ESP32
        A[GPIO16 - RX2] --> B[VR3 TX]
        C[GPIO17 - TX2] --> D[VR3 RX]
        E[GPIO5 - CS] --> F[SD Card CS]
        G[GPIO23 - MOSI] --> H[SD Card MOSI]
        I[GPIO19 - MISO] --> J[SD Card MISO]
        K[GPIO18 - SCK] --> L[SD Card SCK]
        M[GPIO32] --> N[Servo Signal]
        O[GPIO25] --> P[Salida 0 - LED]
        Q[GPIO34] --> R[Bot車n Entrenamiento]
    end

    subgraph M車dulo VR3
        B -->|Serial| VR3
        D -->|Serial| VR3
        VR3 -->|VCC| 5V
        VR3 -->|GND| GND
    end

    subgraph M車dulo SD
        F --> SD
        H --> SD
        J --> SD
        L --> SD
        SD -->|VCC| 3.3V
        SD -->|GND| GND
    end

    subgraph Servomotor
        N --> Servo
        Servo -->|VCC| 5V
        Servo -->|GND| GND
    end
Instalaci車n de Software
Requisitos Previos
Arduino IDE (versi車n 1.8.x o superior)

Plataforma ESP32 (instalada via Arduino Board Manager)

Bibliotecas Requeridas:

VoiceRecognitionV3

ESP32Servo

AudioFileSourceSD

AudioGeneratorMP3

AudioOutputI2SNoDAC

Configuraci車n del Entorno
Instalar las bibliotecas desde Gestor de Bibliotecas (Sketch > Incluir Biblioteca > Administrar Bibliotecas)

Seleccionar la placa ESP32 Dev Module en Herramientas > Placa

Configurar los par芍metros:

Upload Speed: 921600

Flash Frequency: 80MHz

Partition Scheme: Default 4MB

Preparaci車n de Archivos de Audio
Formatear tarjeta SD como FAT32

Crear archivos MP3 con nombres espec赤ficos:

/jarvis.mp3 - Audio para comando "Jarvis"

/abrir.mp3 - Audio para comando "Abrir"

/cerrar.mp3 - Audio para comando "Cerrar"

Colocar los archivos en la ra赤z de la SD

Funcionamiento del Sistema
Modo Normal de Operaci車n
Al iniciar, el sistema carga los comandos predeterminados:

Registro 0: "Jarvis" (activaci車n)

Registro 1: "Abrir" (abre servo)

Registro 2: "Cerrar" (cierra servo)

Al detectar un comando:

Reproduce el audio correspondiente

Mueve el servomotor

Parpadea el LED seg迆n el comando

Modo Entrenamiento
Activaci車n:

Mantener pulsado el bot車n de entrenamiento (GPIO34) por 3 segundos

O enviar "entrenar" por monitor serial

Proceso:

Sistema gu赤a mediante parpadeos LED

Entrena cada comando secuencialmente

Confirma con parpadeo r芍pido

Comandos personalizables:

Registro 0: Palabra de activaci車n

Registro 1: Comando para abrir

Registro 2: Comando para cerrar

Estructura del C車digo
Funciones Principales
cpp
// Configuraci車n inicial
void setup() {
  // Inicializa perif谷ricos y m車dulos
}

// Bucle principal
void loop() {
  // Gestiona modos operaci車n/entrenamiento
  // Actualiza tareas no bloqueantes
}

// Gesti車n de audio
void manejarReproduccionAudio() {
  // Controla reproducci車n MP3
}

// Movimiento de servo
void actualizarServo() {
  // Control suave no bloqueante
}

// Entrenamiento de comandos
void manejarModoEntrenamiento() {
  // Gu赤a al usuario en el proceso
}
Variables Clave
modoEntrenamiento: Bandera de estado

servoAnguloObjetivo: Posici車n destino del servo

parpadeando: Control de retroalimentaci車n visual

archivoAudioActual: Audio a reproducir

registroActualEntrenamiento: Comando en entrenamiento

Programaci車n No Bloqueante
El sistema utiliza t谷cnicas de programaci車n no bloqueante para gestionar m迆ltiples tareas simult芍neamente:

Control de servo: Movimiento gradual mediante millis()

Parpadeo LED: Temporizaci車n precisa sin delay()

Reproducci車n audio: Flujo continuo con gesti車n de buffers

Detecci車n comandos: Escucha constante sin interrupciones

Soluci車n de Problemas
Problemas Comunes
M車dulo VR3 no responde:

Verificar conexiones RX/TX

Comprobar LED amarillo en m車dulo

Asegurar alimentaci車n estable

SD no detectada:

Revisar formato (FAT32)

Verificar conexi車n SPI

Probar con otra tarjeta

Servo no se mueve:

Comprobar conexi車n de 5V

Verificar se?al en osciloscopio

Revisar rango de movimiento (0-180～)

Mensajes de Error
"Tarjeta SD no encontrada": Revisar conexiones

"Archivo no encontrado": Verificar nombres en SD

"Fallo en entrenamiento": Repetir en ambiente silencioso

Aplicaciones Educativas
Introducci車n a sistemas embebidos

Programaci車n de interfaces de voz

Control de actuadores mec芍nicos

Gesti車n de sistemas de archivos

T谷cnicas de programaci車n no bloqueante

Mejoras Futuras
Implementar conexi車n WiFi para control remoto