
---

## ⚙️ Requisitos

- Placa ESP32.
- Módulo ELECHOUSE Voice Recognition V3 frankvanhooft / ESP32-ElechouseVR3.
- Biblioteca https://github.com/frankvanhooft/ESP32-ElechouseVR3 y [VoiceRecognitionV3](https://github.com/Elechouse/VoiceRecognitionV3).
- Arduino IDE.
- Cables jumper.

---

## 🧠 Comandos Reconocidos

El sistema carga 3 comandos por defecto:

| Registro | Comando de voz  | Acción                                      |
|----------|-----------------|---------------------------------------------|
| 0        | `Jarvis`        | Activa salida GPIO18                        |
| 1        | `Abrir mascara` | Activa salida GPIO19                        |
| 2        | `Cerrar mascara`| Activa salida GPIO20                        |

---

## 🚦 Funcionamiento

1. Al iniciar, el ESP32 carga los registros 0, 1 y 2.
2. El usuario dicta un comando entrenado.
3. Si es reconocido:
   - Se muestra el comando por consola.
   - El LED interno (GPIO2) parpadea `registro + 1` veces.
   - Se activa la salida correspondiente por 500 ms.

---

## 📝 Instrucciones de Uso

1. Entrena los comandos de voz usando el software oficial de ELECHOUSE remplazando por la versión frankvanhooft.
2. Conecta el módulo VR3 al ESP32 según el diagrama.
3. Carga el sketch `ReconocimientoVoz.ino` desde Arduino IDE.
4. Abre el monitor serie a 115200 baudios.
5. Pronuncia uno de los comandos entrenados.

---

## 🧩 Estructura del Código

- `setup()`: Inicializa el módulo, pines y carga comandos.
- `loop()`: Espera un comando de voz y ejecuta la acción.
- `parpadearLed(int veces)`: Parpadea el LED interno.
- `reproducir(int registro)`: Activa la salida según el comando.

---

## 🧪 Estado del Proyecto

- ✅ Reconocimiento de comandos funcional.
- ✅ Control de salidas digitales.
- ✅ Retroalimentación visual con LED interno.
- ✅ Código documentado para desarrollo y mantenimiento.

---

## 🧰 Créditos

Desarrollado para proyectos educativos con ESP32 y comandos de voz.  
Basado en la librería de ELECHOUSE y frankvanhooft.

---
