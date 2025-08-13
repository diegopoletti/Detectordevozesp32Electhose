Proyecto de Control por Voz con ESP32 y Reconocimiento de Voz VR3
Introducci��n
Este proyecto implementa un sistema de control por voz utilizando un m��dulo ESP32, un m��dulo de reconocimiento de voz VR3 de ELECHOUSE, una tarjeta SD para almacenamiento de archivos de audio, y un servomotor para control f��sico. El sistema permite ejecutar comandos como "Abrir" y "Cerrar" mediante reconocimiento de voz, con capacidad de entrenamiento personalizado y retroalimentaci��n auditiva.

Componentes Requeridos
ESP32 (placa de desarrollo)

M��dulo de reconocimiento de voz VR3 (ELECHOUSE V3)

M��dulo lector de tarjetas SD

Servomotor (SG90 o similar)

LED indicador

Resistencias (220�� para LED)

Cables de conexi��n

Tarjeta microSD (formateada en FAT32)

Fuente de alimentaci��n (5V/2A recomendado)

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
        Q[GPIO34] --> R[Bot��n Entrenamiento]
    end

    subgraph M��dulo VR3
        B -->|Serial| VR3
        D -->|Serial| VR3
        VR3 -->|VCC| 5V
        VR3 -->|GND| GND
    end

    subgraph M��dulo SD
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
Instalaci��n de Software
Requisitos Previos
Arduino IDE (versi��n 1.8.x o superior)

Plataforma ESP32 (instalada via Arduino Board Manager)

Bibliotecas Requeridas:

VoiceRecognitionV3

ESP32Servo

AudioFileSourceSD

AudioGeneratorMP3

AudioOutputI2SNoDAC

Configuraci��n del Entorno
Instalar las bibliotecas desde Gestor de Bibliotecas (Sketch > Incluir Biblioteca > Administrar Bibliotecas)

Seleccionar la placa ESP32 Dev Module en Herramientas > Placa

Configurar los par��metros:

Upload Speed: 921600

Flash Frequency: 80MHz

Partition Scheme: Default 4MB

Preparaci��n de Archivos de Audio
Formatear tarjeta SD como FAT32

Crear archivos MP3 con nombres espec��ficos:

/jarvis.mp3 - Audio para comando "Jarvis"

/abrir.mp3 - Audio para comando "Abrir"

/cerrar.mp3 - Audio para comando "Cerrar"

Colocar los archivos en la ra��z de la SD

Funcionamiento del Sistema
Modo Normal de Operaci��n
Al iniciar, el sistema carga los comandos predeterminados:

Registro 0: "Jarvis" (activaci��n)

Registro 1: "Abrir" (abre servo)

Registro 2: "Cerrar" (cierra servo)

Al detectar un comando:

Reproduce el audio correspondiente

Mueve el servomotor

Parpadea el LED seg��n el comando

Modo Entrenamiento
Activaci��n:

Mantener pulsado el bot��n de entrenamiento (GPIO34) por 3 segundos

O enviar "entrenar" por monitor serial

Proceso:

Sistema gu��a mediante parpadeos LED

Entrena cada comando secuencialmente

Confirma con parpadeo r��pido

Comandos personalizables:

Registro 0: Palabra de activaci��n

Registro 1: Comando para abrir

Registro 2: Comando para cerrar

Estructura del C��digo
Funciones Principales
cpp
// Configuraci��n inicial
void setup() {
  // Inicializa perif��ricos y m��dulos
}

// Bucle principal
void loop() {
  // Gestiona modos operaci��n/entrenamiento
  // Actualiza tareas no bloqueantes
}

// Gesti��n de audio
void manejarReproduccionAudio() {
  // Controla reproducci��n MP3
}

// Movimiento de servo
void actualizarServo() {
  // Control suave no bloqueante
}

// Entrenamiento de comandos
void manejarModoEntrenamiento() {
  // Gu��a al usuario en el proceso
}
Variables Clave
modoEntrenamiento: Bandera de estado

servoAnguloObjetivo: Posici��n destino del servo

parpadeando: Control de retroalimentaci��n visual

archivoAudioActual: Audio a reproducir

registroActualEntrenamiento: Comando en entrenamiento

Programaci��n No Bloqueante
El sistema utiliza t��cnicas de programaci��n no bloqueante para gestionar m��ltiples tareas simult��neamente:

Control de servo: Movimiento gradual mediante millis()

Parpadeo LED: Temporizaci��n precisa sin delay()

Reproducci��n audio: Flujo continuo con gesti��n de buffers

Detecci��n comandos: Escucha constante sin interrupciones

Soluci��n de Problemas
Problemas Comunes
M��dulo VR3 no responde:

Verificar conexiones RX/TX

Comprobar LED amarillo en m��dulo

Asegurar alimentaci��n estable

SD no detectada:

Revisar formato (FAT32)

Verificar conexi��n SPI

Probar con otra tarjeta

Servo no se mueve:

Comprobar conexi��n de 5V

Verificar se?al en osciloscopio

Revisar rango de movimiento (0-180��)

Mensajes de Error
"Tarjeta SD no encontrada": Revisar conexiones

"Archivo no encontrado": Verificar nombres en SD

"Fallo en entrenamiento": Repetir en ambiente silencioso

Aplicaciones Educativas
Introducci��n a sistemas embebidos

Programaci��n de interfaces de voz

Control de actuadores mec��nicos

Gesti��n de sistemas de archivos

T��cnicas de programaci��n no bloqueante

Mejoras Futuras
Implementar conexi��n WiFi para control remoto