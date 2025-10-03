# **Proyecto: Módulo de Voz Interactivo v1.4 con ESP32**

## **1\. Descripción General**

Este proyecto consiste en un sistema interactivo controlado por voz basado en el microcontrolador ESP32. El dispositivo es capaz de reconocer comandos de voz predefinidos, reproducir archivos de audio MP3 desde una tarjeta SD, controlar salidas digitales y mover un servomotor de manera precisa y con memoria de estado.

La característica principal de esta versión es su arquitectura robusta, que utiliza una **máquina de estados** para gestionar las tareas, garantizando que operaciones críticas como la reproducción de audio se completen sin interrupciones. Además, incluye un **modo de entrenamiento** que permite al usuario grabar su propia voz para los comandos, aumentando significativamente la precisión del reconocimiento.

## **2\. Características Principales**

* **Reconocimiento de Voz**: Utiliza el módulo Voice Recognition V3 para procesar hasta 11 comandos de voz diferentes.  
* **Reproducción de Audio MP3**: Reproduce archivos de audio de alta calidad desde una tarjeta SD a través del protocolo I2S.  
* **Gestión de Tareas sin Interrupciones**: Una máquina de estados en el bucle principal prioriza la reproducción de audio, evitando que nuevos comandos la corten.  
* **Control de Actuadores**:  
  * **Servomotor con Estado**: Control suave y no bloqueante para movimientos precisos entre una posición abierta y una cerrada, manteniendo el estado hasta recibir un nuevo comando.  
  * **Salidas Digitales**: Activación de LEDs o relés en respuesta a comandos de voz.  
* **Modo de Entrenamiento Personalizado**: Permite al usuario grabar su propia voz para cada comando, mejorando la fiabilidad del sistema.  
* **Feedback Visual**: Un LED integrado proporciona información sobre el estado del sistema (escuchando, procesando, entrenando).  
* **Código Altamente Comentado**: El firmware está documentado en detalle para facilitar su comprensión y modificación.

## **3\. Cambios y Mejoras**

### **Versión 1.2 / 1.3: Implementación de la Máquina de Estados de Audio**

La mejora fundamental inicial fue la reestructuración completa del bucle principal (loop()) para que funcione como un **controlador de estados**.

* **Problema Anterior**: El programa intentaba escuchar nuevos comandos constantemente. Si se reconocía un nuevo comando mientras un audio se estaba reproduciendo, la reproducción se interrumpía bruscamente.  
* **Solución Implementada**: Se implementó una lógica de estados claros:  
  1. **Estado de Reposo/Escucha**: Si no hay ninguna tarea crítica en ejecución, el sistema se dedica a escuchar un nuevo comando de voz.  
  2. **Estado de Reproducción de Audio**: En cuanto se inicia la reproducción, el sistema **deja de escuchar nuevos comandos** y se enfoca en mantener la reproducción fluida hasta que termine.  
  3. **Transición de Estados**: Al finalizar el audio, el sistema regresa automáticamente al estado de escucha.

### **Versión 1.4: Nuevo Control de Servo con Memoria de Estado**

En esta versión, se ha mejorado la lógica de control del servomotor para que sea más intuitiva y funcional.

* **Problema Anterior**: El servo realizaba un movimiento y luego, en algunas lógicas, volvía a una posición neutral automáticamente. El movimiento no era persistente.  
* **Solución Implementada**:  
  1. **Dos Estados Definidos**: Se crearon dos posiciones fijas: SERVO\_CERRADO (0 grados) y SERVO\_ABIERTO (90 grados).  
  2. **Movimiento por Comando**: El comando "Abrir" mueve el servo a la posición de 90 grados. El comando "Cerrar" lo mueve a la posición de 0 grados.  
  3. **Persistencia de Estado**: El cambio más importante es que el servo ahora **permanece en su última posición comandada** (abierto o cerrado) indefinidamente, hasta que se recibe el comando opuesto. Se eliminó cualquier lógica que lo devolviera a un estado neutral de forma automática.

Este cambio permite que el servo actúe como un interruptor mecánico (por ejemplo, para mantener una compuerta abierta o cerrada) en lugar de realizar solo un movimiento momentáneo.

## **4\. Diagrama de Conexiones**

A continuación se muestra la tabla de conexiones recomendada entre el ESP32 y los módulos periféricos.

| Pin ESP32 | Conectar a | Módulo | Notas |
| :---- | :---- | :---- | :---- |
| **VIN (5V)** | VCC | Todos los Módulos | Alimentación principal. Usar fuente externa si el consumo es alto. |
| **GND** | GND | Todos los Módulos | Tierra común. Es crucial que todas las tierras estén conectadas. |
| **GPIO 17 (TX2)** | RX | Módulo de Voz V3 | Transmisión del ESP32 a la Recepción del Módulo de Voz. |
| **GPIO 16 (RX2)** | TX | Módulo de Voz V3 | Recepción del ESP32 desde la Transmisión del Módulo de Voz. |
| **GPIO 22** | BCLK | Amplificador I2S (MAX98357A) | Bit Clock para la comunicación de audio. |
| **GPIO 21** | LRC / WS | Amplificador I2S (MAX98357A) | Left/Right Clock (Word Select) para audio. |
| **GPIO 19** | DIN / SD | Amplificador I2S (MAX98357A) | Data In para la señal de audio. |
| **GPIO 23 (MOSI)** | DI / MOSI | Lector de Tarjeta SD | Master Out Slave In para la comunicación SPI. |
| **GPIO 18 (SCK)** | CLK / SCK | Lector de Tarjeta SD | Serial Clock para la comunicación SPI. |
| **GPIO 5 (CS)** | CS | Lector de Tarjeta SD | Chip Select para activar la comunicación con la SD. |
| **GPIO 32** | Señal (Naranja/Amarillo) | Servomotor SG90 | Pin de control para la posición del servo. |
| **GPIO 2** | \- | LED Interno | Usado para feedback visual. |
| **GPIO 25, 26, 27, 33** | \- | Salidas Digitales | Disponibles para conectar LEDs externos, relés, etc. |

## **5\. Diagrama de Flujo del Programa**

El siguiente diagrama ilustra la lógica de la máquina de estados implementada en la función loop() principal.

graph TD  
    A\[Inicio del Bucle \`loop()\`\] \--\> B{Tareas de Fondo (Parpadeo, Servo, Salidas)};  
    B \--\> C{¿Comando "entrenar" por Serial?};  
    C \-- Sí \--\> D\[Activar Modo Entrenamiento\];  
    C \-- No \--\> E{¿Modo Entrenamiento Activo?};  
    E \-- Sí \--\> F\[Ejecutar \`manejarModoEntrenamiento()\`\];  
    F \--\> G\[Fin del Ciclo, Volver a A\];  
    E \-- No \--\> H{¿Audio en Reproducción? (\`decodificadorMp3.isRunning()\`)};  
    H \-- Sí \--\> I\[Gestionar Audio: Aumentar Prioridad\];  
    I \--\> J{¿Audio Finalizado?};  
    J \-- No \--\> G;  
    J \-- Sí \--\> K\[Detener Audio y Restaurar Prioridad\];  
    K \--\> L{¿Hay audio en cola?};  
    L \-- Sí \--\> M\[Reproducir Siguiente Audio\];  
    L \-- No \--\> G;  
    M \--\> G;  
    H \-- No \--\> N\[Estado de Escucha: \`manejarModoOperacionNormal()\`\];  
    N \--\> G;

## **6\. Estructura de la Tarjeta SD**

Para que el programa funcione correctamente, crea los siguientes archivos de audio en formato MP3 y colócalos en la raíz de la tarjeta MicroSD:

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

### **Operación Normal**

1. Alimenta el circuito. El ESP32 se iniciará y cargará los comandos de voz.  
2. El monitor serial mostrará "Dispositivo listo. Esperando comandos...".  
3. Di uno de los comandos entrenados. El sistema ejecutará la acción correspondiente.

### **Modo de Entrenamiento**

1. Conecta el ESP32 a tu ordenador y abre el Monitor Serial (velocidad 115200).  
2. Escribe la palabra entrenar en la caja de texto del monitor y presiona Enter.  
3. Sigue las instrucciones que aparecerán en el monitor. El sistema te pedirá que digas cada una de las palabras de los comandos para grabarlas con tu voz.  
4. Una vez finalizado, el sistema volverá al modo de operación normal, utilizando las nuevas grabaciones.

### **Tabla de Comandos y Acciones**

| Registro | Comando de Voz | Acción del Sistema | Archivo de Audio Asociado |
| :---- | :---- | :---- | :---- |
| 0 | "Jarvis" | Activa SALIDA\_0 por 500ms. | /jarvis.mp3 |
| 1 | "Abrir" | Activa SALIDA\_1 y mueve el servo a la posición abierta (90°). | /abrir.mp3 |
| 2 | "Cerrar" | Activa SALIDA\_2 y mueve el servo a la posición cerrada (0°). | /cerrar.mp3 |
| 3 | "Diagnostico" | Solo reproduce audio. | /audio\_diagnostico.mp3 |
| 4 | "Me Amas" | Solo reproduce audio. | /audio\_me\_amas.mp3 |
| 5 | "Modo Ataque" | Solo reproduce audio. | /audio\_modo\_ataque.mp3 |
| 6 | "Musica" | Reproduce intro y luego la canción 1\. | /audio\_pre\_musica.mp3 |
| 7 | "Musica2" | Reproduce intro y luego la canción 2\. | /audio\_pre\_musica.mp3 |
| 8 | "Reactor On" | Activa SALIDA\_REACTOR\_ON por 500ms. | /audio\_reactor\_on.mp3 |
| 9 | "Comando Extra 1" | Sin acción definida (personalizable). | N/A |
| 10 | "Comando Extra 2" | Sin acción definida (personalizable). | N/A |

