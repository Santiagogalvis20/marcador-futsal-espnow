# marcador-futsal-espnow
# Marcador inalámbrico para futsal con ESP-NOW

Prototipo de marcador deportivo inalámbrico para canchas de futsal o microfútbol.
Un control portátil envía los datos del partido a un tablero de dígitos de siete
segmentos mediante ESP-NOW, sin necesidad de router, Wi-Fi ni Bluetooth.

Proyecto de grado de Santiago Galvis Castañeda, Unidades Tecnológicas de
Santander (UTS).

## Características

- Comunicación directa entre módulos con ESP-NOW.
- Control de goles, cronómetro y modos de juego con pulsadores.
- Tablero de siete segmentos con LEDs WS2812B y carcasas impresas en 3D.
- Control portátil con batería LiPo; tablero con power bank de 5 V.
- Alcance probado entre 20 y 40 m en una cancha de futsal.

## Hardware

| Módulo | Componentes principales |
|---|---|
| Transmisor (control) | ESP32-S3 Super Mini, batería LiPo 3,7 V / 300 mAh, pulsadores, interruptor de palanca, PCB propia |
| Receptor (tablero) | ESP32, LEDs WS2812B (40), power bank de 5000 mAh (5 V), PCB propia, carcasas impresas en 3D con acrílico difusor |

## Estructura del repositorio

- `transmisor/`: código Arduino del control.
- `receptor/`: código Arduino del tablero.
- `hardware/`: esquemáticos y PCB (opcional).
- `3d/`: archivos de las carcasas impresas (opcional).

## Requisitos de software

- Arduino IDE [versión]
- Soporte de placas ESP32 de Espressif (core [versión])
- Biblioteca para los LEDs WS2812B: [FastLED / Adafruit NeoPixel, versión]

## Cómo usarlo

1. Abre `receptor/receptor.ino`, carga el código en el ESP32 del tablero y
   anota la dirección MAC que muestra en el monitor serie.
2. En `transmisor/transmisor.ino`, escribe esa dirección MAC del receptor en
   la variable correspondiente.
3. Carga el código en el ESP32-S3 Super Mini del control.
4. Alimenta el tablero con la power bank (puerto de 5 V / 2,1 A) y enciende
   el control con el interruptor.

## Consumo y autonomía (estimados)

Valores calculados a partir de hojas de datos, no medidos con instrumentos.

- Control: unas 3 h 25 min con la radio en espera.
- Tablero: de 1 h 25 min (LEDs al 100 %) a más de 9 h (LEDs al 10 %).

Se recomienda limitar el brillo de los LEDs al 50 % o menos.

## Licencia

MIT

## Autor

Santiago Galvis Castañeda, UTS.
