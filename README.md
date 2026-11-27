# Levitador neumático controlado con PLC Siemens S7-1200

Sistema de control de lazo cerrado que mantiene una pelota de ping-pong a una altura deseada dentro de un tubo de acrílico, regulando el flujo de aire de un ventilador. El control se realiza con un PLC Siemens S7-1200 (CPU 1214C DC/DC/DC) programado en TIA Portal V20 en lenguaje Ladder, con un PID_Compact, una HMI KTP700 Basic y telemetría en la nube por MQTT mediante una ESP32.

**Fundación Universitaria Los Libertadores – Ingeniería Mecatrónica**

- **Autores:** David Gómez y Bryan Espinosa
- **Docente:** Alfonso Durán

---

## Contenido del repositorio

| Archivo | Descripción |
|---|---|
| `Informe técnico Levitador neumático ... .pdf` | Informe completo del proyecto: diseño, cálculos, programación, pruebas y conclusiones |
| `Proyecto levitador v20.0.rar` | Proyecto de TIA Portal V20 (PLC + HMI) |
| `levitador_mqtt.ino` | Programa de la ESP32: lectura del sensor, salida analógica y publicación MQTT |

## Cómo funciona

```
Sensor VL53L0X ──I2C──> ESP32 ──PWM 0–3,3 V──> Amplificador LM358 (×3,13) ──0–10 V──> PLC (AI0)
                          │                                                            │
                          └──WiFi──> MQTT (nube)                     Filtro + PID_Compact (OB30, 30 ms)
                                                                                       │
Pelota <── aire ── Ventilador 12 V (4 cables) <── 2N2222A <── PWM 25 kHz (Q0.0) ────────┘
                          ▲
                Regulador buck 24 V → 12 V
                                                   HMI KTP700 Basic <──PROFINET──> PLC
```

1. El sensor láser VL53L0X mide la distancia a la pelota.
2. La ESP32 convierte la distancia (0–450 mm) en una señal de 0–3,3 V y la publica por MQTT.
3. Un amplificador LM358 lleva la señal a 0–10 V para la entrada analógica del PLC.
4. El PLC calcula la altura, la filtra y la compara con el setpoint en un PID_Compact.
5. La salida del PID genera una PWM de 25 kHz en Q0.0 que, a través de un 2N2222A, controla la velocidad del ventilador.
6. Desde la HMI se cambia el setpoint (0–400 mm) y se ve la altura de la pelota y la gráfica de la PWM.

## Componentes

- PLC Siemens S7-1200, CPU 1214C DC/DC/DC (maletín didáctico ICL PLC Training)
- HMI Siemens KTP700 Basic
- ESP32
- Sensor de distancia láser VL53L0X
- Ventilador 12 V, 0,6 A, 4 cables (control PWM)
- Regulador buck 24 V → 12 V
- Amplificador operacional LM358 y resistencias de 10 kΩ, 4,7 kΩ y 1 kΩ
- Transistor 2N2222A
- Tubo de acrílico de 45 cm de largo y 5 cm de diámetro
- Base del ventilador y conductor de aire impresos en 3D (PLA)
- Pelota de ping-pong estándar (40 mm, 2,7 g)

## Parámetros del PID

| Parámetro | Valor |
|---|---|
| Kp | 0,01809 %/mm |
| Ti | 0,578 s |
| Td | 0,125 s |
| Tiempo de muestreo | 30 ms |
| Precontrol (Disturbance) | 65 % |

## Telemetría MQTT

| Parámetro | Valor |
|---|---|
| Broker | test.mosquitto.org |
| Puerto | 1883 |
| Tópico | `levitador/prototipo_7A31/telemetria` |
| Periodo | 500 ms |

Ejemplo de mensaje:

```json
{"distancia_mm":280.0,"altura_mm":170.0,"setpoint_mm":150.0,"zona_util":true}
```

Para ver los datos en tiempo real, conéctate al broker con cualquier cliente MQTT (por ejemplo, MQTT Explorer) y suscríbete al tópico anterior.

## Cómo usar el proyecto

1. **PLC y HMI:** descomprime el `.rar` y abre el proyecto con TIA Portal V20. Descárgalo a la CPU 1214C y a la HMI.
2. **ESP32:** abre `levitador_mqtt.ino` en Arduino IDE, instala las librerías `Adafruit_VL53L0X` y `PubSubClient`, escribe el nombre y la clave de tu red WiFi y cárgalo a la ESP32.
3. Activa el selector **MARCHA** del maletín y escribe el setpoint en la HMI.

Más detalles en el informe PDF.
