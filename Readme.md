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

Cambios en V1.3
Principales Mejoras Implementadas:
Logica de Control Centralizada en loop(): Se reestructuro la funcion loop() para que actue como un controlador de estado. Ahora, el programa solo escucha nuevos comandos de voz (manejarModoOperacionNormal()) cuando el reproductor de MP3 no esta ocupado (!decodificadorMp3.isRunning()). Esto previene de forma efectiva que un nuevo comando interrumpa un audio en curso.

Eliminacion de Banderas Redundantes: Se elimino la variable global reproduccionEnCurso. La funcion decodificadorMp3.isRunning() ya nos proporciona esta informacion de manera directa y fiable, simplificando el codigo y reduciendo posibles puntos de error.

Llamadas Directas a reproducirAudio(): La funcion manejarModoOperacionNormal() ahora llama directamente a reproducirAudio() en lugar de simplemente asignar un nombre de archivo a una variable. Esto es posible porque ahora tenemos la certeza de que solo se entra en esa funcion cuando no hay nada reproduciendose.

Simplificacion de reproducirAudio(): Se elimino la comprobacion inicial de la funcion, ya que la nueva logica en loop() garantiza que no sera llamada si ya hay un audio en curso.

Manejo de Audio Consolidado: La logica que gestionaba la finalizacion de un audio se ha movido directamente al loop(), dentro de la seccion que se ejecuta cuando decodificadorMp3.isRunning() es verdadero. Esto hace que el flujo del programa sea mucho mas claro y facil de seguir.

# **Proyecto: Modulo de Voz Interactivo v1.3 con ESP32**

\!\[\]

## **1\. Descripcion General**

Este proyecto consiste en un sistema interactivo controlado por voz basado en el microcontrolador ESP32. El dispositivo es capaz de reconocer comandos de voz predefinidos, reproducir archivos de audio MP3 desde una tarjeta SD, controlar salidas digitales y mover un servomotor de manera precisa.

La caracteristica principal de esta version es su arquitectura robusta, que utiliza una **maquina de estados** para gestionar las tareas, garantizando que operaciones criticas como la reproduccion de audio se completen sin interrupciones. Ademas, incluye un **modo de entrenamiento** que permite al usuario grabar su propia voz para los comandos, aumentando significativamente la precision del reconocimiento.

## **2\. Caracteristicas Principales**

* **Reconocimiento de Voz**: Utiliza el modulo Voice Recognition V3 para procesar hasta 11 comandos de voz diferentes.  
* **Reproduccion de Audio MP3**: Reproduce archivos de audio de alta calidad desde una tarjeta SD a traves del protocolo I2S.  
* **Gestion de Tareas sin Interrupciones**: Una maquina de estados en el bucle principal prioriza la reproduccion de audio, evitando que nuevos comandos la corten.  
* **Control de Actuadores**:  
  * **Servomotor**: Control suave y no bloqueante para movimientos precisos.  
  * **Salidas Digitales**: Activacion de LEDs o reles en respuesta a comandos de voz.  
* **Modo de Entrenamiento Personalizado**: Permite al usuario grabar su propia voz para cada comando, mejorando la fiabilidad del sistema.  
* **Feedback Visual**: Un LED integrado proporciona informacion sobre el estado del sistema (escuchando, procesando, entrenando).  
* **Codigo Altamente Comentado**: El firmware esta documentado en detalle para facilitar su comprension y modificacion.

## **3\. Cambios y Mejoras (Version 1.2 / 1.3)**

La mejora fundamental respecto a versiones anteriores es la reestructuracion completa del bucle principal (loop()) para que funcione como un **controlador de estados**.

### **Version Anterior (Problematica)**

En la version original, el programa intentaba escuchar nuevos comandos de voz constantemente, incluso mientras un archivo de audio se estaba reproduciendo. Esto llevaba a una condicion de carrera: si se reconocia un nuevo comando a mitad de una reproduccion, la logica de audio se interrumpia bruscamente para procesar el nuevo comando, cortando el sonido.

### **Nueva Version (Solucion Implementada)**

La version actual soluciona este problema implementando una logica de estados claros y excluyentes:

1. **Estado de Reposo/Escucha**: Si no hay ninguna tarea critica en ejecucion (como reproducir un audio), el sistema se dedica a escuchar un nuevo comando de voz (manejarModoOperacionNormal()).  
2. **Estado de Reproduccion de Audio**: En cuanto se inicia la reproduccion de un audio (reproducirAudio()), la condicion decodificadorMp3.isRunning() se vuelve verdadera. El bucle principal detecta este estado y **deja de escuchar nuevos comandos**. Toda la atencion del procesador se centra en mantener la reproduccion fluida hasta que el archivo termine.  
3. **Transicion de Estados**: Una vez que el audio finaliza, decodificadorMp3.isRunning() se vuelve falso, y el sistema regresa automaticamente al **Estado de Reposo/Escucha**, listo para recibir el siguiente comando.

Este cambio garantiza una experiencia de usuario mucho mas profesional y predecible, donde cada accion se completa en su totalidad antes de iniciar la siguiente.

## **4\. Diagrama de Flujo del Programa**

El siguiente diagrama ilustra la logica de la maquina de estados implementada en la funcion loop() principal.

graph TD  
    A\[Inicio del Bucle \`loop()\`\] \--\> B{Tareas de Fondo (Parpadeo, Servo, Salidas)};  
    B \--\> C{?Comando "entrenar" por Serial?};  
    C \-- Si \--\> D\[Activar Modo Entrenamiento\];  
    C \-- No \--\> E{?Modo Entrenamiento Activo?};  
    E \-- Si \--\> F\[Ejecutar \`manejarModoEntrenamiento()\`\];  
    F \--\> G\[Fin del Ciclo, Volver a A\];  
    E \-- No \--\> H{?Audio en Reproduccion? (\`decodificadorMp3.isRunning()\`)};  
    H \-- Si \--\> I\[Gestionar Audio: Aumentar Prioridad\];  
    I \--\> J{?Audio Finalizado?};  
    J \-- No \--\> G;  
    J \-- Si \--\> K\[Detener Audio y Restaurar Prioridad\];  
    K \--\> L{?Hay audio en cola?};  
    L \-- Si \--\> M\[Reproducir Siguiente Audio\];  
    L \-- No \--\> G;  
    M \--\> G;  
    H \-- No \--\> N\[Estado de Escucha: \`manejarModoOperacionNormal()\`\];  
    N \--\> G;

## **5\. Componentes y Conexiones**

### **Hardware Requerido**

* Microcontrolador ESP32 DevKitC V4 (o similar).  
* Modulo de Reconocimiento de Voz V3.  
* Modulo reproductor de MP3 con amplificador (ej. MAX98357A I2S).  
* Lector de tarjetas MicroSD.  
* Tarjeta MicroSD (formateada en FAT32).  
* Servomotor (ej. SG90).  
* Altavoz (3W, 4-8 Ohm).  
* Fuente de alimentacion externa (5V, 2A recomendada).  
* Cables, protoboard y componentes pasivos (resistencias, si son necesarias).

### **Librerias de Arduino**

Asegurate de tener instaladas las siguientes librerias a traves del Gestor de Librerias del IDE de Arduino:

* ESP32Servo por Kevin Harrington  
* ESP8266Audio por Earle F. Philhower (funciona tambien para ESP32)

## **6\. Estructura de la Tarjeta SD**

Para que el programa funcione correctamente, crea los siguientes archivos de audio en formato MP3 y colocalos en la raiz de la tarjeta MicroSD:

* /jarvis.mp3  
* /abrir.mp3  
* /cerrar.mp3  
* /audio\_diagnostico.mp3  
* /audio\_me\_amas.mp3  
* /audio\_modo\_ataque.mp3  
* /audio\_pre\_musica.mp3  
* /musica\_1.mp3  
* /musica\_2.mp3  
* /audio\_reactor\_on.mp3

## **7\. Modo de Uso**

### **Operacion Normal**

1. Alimenta el circuito. El ESP32 se iniciara y cargara los comandos de voz.  
2. El monitor serial mostrara "Dispositivo listo. Esperando comandos...".  
3. Di uno de los comandos entrenados. El sistema ejecutara la accion correspondiente.

### **Modo de Entrenamiento**

1. Conecta el ESP32 a tu ordenador y abre el Monitor Serial (velocidad 115200).  
2. Escribe la palabra entrenar en la caja de texto del monitor y presiona Enter.  
3. Sigue las instrucciones que apareceran en el monitor. El sistema te pedira que digas cada una de las palabras de los comandos para grabarlas con tu voz.  
4. Una vez finalizado, el sistema volvera al modo de operacion normal, utilizando las nuevas grabaciones.

### **Tabla de Comandos y Acciones**

| Registro | Comando de Voz | Accion del Sistema | Archivo de Audio Asociado |
| :---- | :---- | :---- | :---- |
| 0 | "Jarvis" | Activa SALIDA\_0 por 500ms. | /jarvis.mp3 |
| 1 | "Abrir" | Activa SALIDA\_1 y mueve el servo a la posicion abierta. | /abrir.mp3 |
| 2 | "Cerrar" | Activa SALIDA\_2 y mueve el servo a la posicion cerrada. | /cerrar.mp3 |
| 3 | "Diagnostico" | Solo reproduce audio. | /audio\_diagnostico.mp3 |
| 4 | "Me Amas" | Solo reproduce audio. | /audio\_me\_amas.mp3 |
| 5 | "Modo Ataque" | Solo reproduce audio. | /audio\_modo\_ataque.mp3 |
| 6 | "Musica" | Reproduce intro y luego la cancion 1\. | /audio\_pre\_musica.mp3 |
| 7 | "Musica2" | Reproduce intro y luego la cancion 2\. | /audio\_pre\_musica.mp3 |
| 8 | "Reactor On" | Activa SALIDA\_REACTOR\_ON por 500ms. | /audio\_reactor\_on.mp3 |
| 9 | "Comando Extra 1" | Sin accion definida (personalizable). | N/A |
| 10 | "Comando Extra 2" | Sin accion definida (personalizable). | N/A |

