/*
  CONTROL REMOTO (TRANSMISOR) - MARCADOR DEPORTIVO ESP-NOW
  Placa: ESP32-S3 Super Mini

  Botones (todos conectados entre el GPIO y GND):
    GPIO 13 -> 'a'  suma punto a A   (LED cian)
    GPIO 11 -> 'b'  suma punto a B   (LED magenta)
    GPIO  9 -> 'd'  resta punto      (LED amarillo)
    GPIO 10 -> 'f'  reinicia puntos  (LED rojo)
*/

/* ================= LIBRERIAS ================= */
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>

/* ================= PINES ================= */
#define BTN1 13
#define BTN2 11
#define BTN3 9
#define BTN4 10

#define NEOPIXEL_PIN 48
#define NUMPIXELS    1

/* ================= MAC DEL RECEPTOR ================= */
// Cambia estos valores por la MAC de TU ESP32 del marcador
uint8_t receiverMac[] = {0x68, 0x09, 0x47, 0x5B, 0x95, 0x44};

/* ================= VARIABLES ================= */
Adafruit_NeoPixel pixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

typedef struct struct_message {
  char value;
} struct_message;

struct_message message;

bool bloqueo = false;
unsigned long tiempoEnvio = 0;
const unsigned long tiempoLED = 300;   // tiempo que dura encendido el LED (ms)

bool lastState1 = HIGH;
bool lastState2 = HIGH;
bool lastState3 = HIGH;
bool lastState4 = HIGH;

/* ================= FUNCIONES ================= */
void enviarCaracter(char c, uint32_t color) {
  message.value = c;
  esp_now_send(receiverMac, (uint8_t *)&message, sizeof(message));

  pixel.setPixelColor(0, color);
  pixel.show();

  bloqueo = true;
  tiempoEnvio = millis();
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  setCpuFrequencyMhz(80);        // ahorro de bateria: baja la frecuencia de la CPU

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);

  pixel.begin();
  pixel.setBrightness(5);        // ahorro de bateria: poco brillo
  pixel.clear();
  pixel.show();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);           // ahorro de bateria: WiFi en reposo

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error iniciando ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

/* ================= LOOP ================= */
void loop() {

  // Apaga el LED pasado el tiempo y desbloquea los botones
  if (bloqueo && millis() - tiempoEnvio >= tiempoLED) {
    pixel.clear();
    pixel.show();
    bloqueo = false;
  }

  // Lee los botones
  bool current1 = digitalRead(BTN1);
  bool current2 = digitalRead(BTN2);
  bool current3 = digitalRead(BTN3);
  bool current4 = digitalRead(BTN4);

  // Detecta el momento en que se presiona cada boton
  if (!bloqueo && lastState1 == HIGH && current1 == LOW)
    enviarCaracter('a', pixel.Color(0, 255, 255));

  if (!bloqueo && lastState2 == HIGH && current2 == LOW)
    enviarCaracter('b', pixel.Color(255, 0, 255));

  if (!bloqueo && lastState3 == HIGH && current3 == LOW)
    enviarCaracter('d', pixel.Color(255, 255, 0));

  if (!bloqueo && lastState4 == HIGH && current4 == LOW)
    enviarCaracter('f', pixel.Color(255, 0, 0));

  // Guarda el estado anterior
  lastState1 = current1;
  lastState2 = current2;
  lastState3 = current3;
  lastState4 = current4;

  delay(100);   // ahorro de bateria: pequeño descanso de la CPU
}
