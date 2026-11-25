/*
  MARCADOR DEPORTIVO ESP-NOW (RECEPTOR)
  Placa: ESP32 (WROOM-32)

  Modos (interruptor a GND):
    Ninguno activo -> Marcador estilo tenis
    MODE_24        -> Suma A+B = 24 (termina al llegar a 21 en el codigo original)
    MODE_16        -> Suma A+B = 16 (termina al llegar a 28 en el codigo original)
    MODE_25        -> Juego a 25 con ventaja de 2 (voleibol)
    MODE_BLU       -> Desactiva los LDR (solo control remoto ESP-NOW)
    MODO_SWITCH    -> Tenis: mejor de 3 games (2 sets de 3)

  Comandos recibidos por ESP-NOW:
    'a' -> punto A    'b' -> punto B    'd' -> resta punto    'f' -> reinicia
*/

/* ================= LIBRERIAS ================= */
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <esp_now.h>

/* ================= CONFIGURACION DE PINES ================= */
#define LED_PIN_PUNTOS    18
#define NUM_LEDS_PUNTOS   28

#define LED_PIN_GAMES     13
#define NUM_LEDS_GAMES    12   // 6 A + 6 B

#define LDR_A 35
#define LDR_B 33

#define BTN_BRIGHT 23
#define BUZZER_PIN 3

#define MODO_SWITCH 14   // GND = mejor de 3 games
#define MODE_24     32   // GND = suma A+B = 24
#define MODE_16     5    // GND = suma A+B = 16
#define MODE_25     17   // GND = juego a 25 con ventaja de 2
#define MODE_BLU    19   // GND = marcador solo por control remoto

#define SUMA_TIEMPO  500    // ms que se tapa el LDR para sumar punto
#define RESTA_TIEMPO 3000   // ms que se tapa el LDR para restar punto

/* ================= MENSAJE ESP-NOW ================= */
typedef struct struct_message {
  char value;
} struct_message;

struct_message incomingMessage;

/* ================= LEDS ================= */
Adafruit_NeoPixel stripPuntos(NUM_LEDS_PUNTOS, LED_PIN_PUNTOS, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripGames(NUM_LEDS_GAMES, LED_PIN_GAMES, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_A;
uint32_t COLOR_B;
uint32_t COLOR_OFF;

/* ================= CALIBRACION / LDR ================= */
int pin_calibrar = 16;        // pulsador de calibracion
int UMBRAL_LDR   = 3800;      // umbrales de los LDR (se calibran)
int UMBRAL_LDR_B = 3800;
int estado_calibracion = 1;
float suma_calibra   = 0;
float suma_calibra_B = 0;

int btnBLUE   = 1;
int valorLdrA = 0;
int valorLdrB = 0;
int ldrA = 0;
int ldrB = 0;

char c = 'z';                 // ultimo comando recibido
int numero_del_blu = 0;

/* ================= PUNTOS Y GAMES ================= */
// Modos de suma / voleibol
int puntosA = 0;
int puntosB = 0;

// Modo tenis
int puntosA_T = 0;
int puntosB_T = 0;

int puntosA_T_BLU = 0;
int puntosB_T_BLU = 0;

int gamesA = 0;
int gamesB = 0;

bool ventajaA = false;
bool ventajaB = false;

bool tieBreak = false;
bool setTerminado = false;
bool modoCorto = false;

unsigned long tiempoFinSet = 0;

/* ================= ESTADO DE LOS LDR ================= */
unsigned long ldrATiempo = 0;
unsigned long ldrBTiempo = 0;

bool ldrAActivo = false;
bool ldrBActivo = false;

bool ldrA_Activo = false;
bool ldrB_Activo = false;

bool ldrA_prev = false;
bool ldrB_prev = false;

/* ================= BRILLO ================= */
int brilloIndex = 0;
int nivelesBrillo[4] = {50, 255, 180, 15};
bool btnEstadoAnterior = HIGH;

/* ================= 7 SEGMENTOS ================= */
// Tenis: 0, 15, 30, 40, ADV (14 LEDs por digito)
const byte digitos[5][14] = {
  {1,1,1,0,1,1,1,1,1,1,0,1,1,1},  // 0
  {0,0,1,0,0,0,1,1,1,0,1,0,1,1},  // 15
  {0,1,1,1,0,1,1,1,1,1,0,1,1,1},  // 30
  {1,0,1,1,0,0,1,1,1,1,0,1,1,1},  // 40
  {1,1,1,1,1,0,1,0,0,0,1,0,0,0}   // A-
};

// Modos de suma / voleibol: digitos 0-9 (7 LEDs por digito)
const bool digitSegments[10][7] = {
  {1,1,1,0,1,1,1},  // 0
  {0,0,1,0,0,0,1},  // 1
  {0,1,1,1,1,1,0},  // 2
  {0,1,1,1,0,1,1},  // 3
  {1,0,1,1,0,0,1},  // 4
  {1,1,0,1,0,1,1},  // 5
  {1,0,0,1,1,1,1},  // 6
  {0,1,1,0,0,0,1},  // 7
  {1,1,1,1,1,1,1},  // 8
  {1,1,1,1,0,0,1}   // 9
};

/* ================================================================= */
/*                          FUNCIONES BASICAS                        */
/* ================================================================= */

void beep(int t) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(t);
  digitalWrite(BUZZER_PIN, LOW);
}

/* ================= CALIBRACION ================= */
void calibracion() {
  int k = 0;
  delay(500);
  estado_calibracion = 1;

  // --- LDR A ---
  for (int i = 0; i < 100; i++) {
    stripGames.setPixelColor(k, 255, 255, 0);
    stripGames.show();
    beep(25);
    int Valor_LDR = analogRead(LDR_A);
    suma_calibra = suma_calibra + Valor_LDR;
    delay(10);
    stripGames.setPixelColor(k, 0, 0, 0);
    k = k + 1;
    if (k >= 6) k = 0;
  }
  suma_calibra = suma_calibra / 100;
  UMBRAL_LDR = suma_calibra + 250;
  if (UMBRAL_LDR > 4090) UMBRAL_LDR = 4060;   // sensor obstruido

  // --- LDR B ---
  for (int j = 0; j < 105; j++) {
    stripGames.setPixelColor(k, 0, 255, 0);
    stripGames.show();
    beep(25);
    int Valor_LDR_B = analogRead(LDR_B);
    suma_calibra_B = suma_calibra_B + Valor_LDR_B;
    delay(10);
    stripGames.setPixelColor(k, 0, 0, 0);
    k = k + 1;
    if (k >= 14) k = 6;
  }
  suma_calibra_B = suma_calibra_B / 105;
  UMBRAL_LDR_B = suma_calibra_B + 250;
  if (UMBRAL_LDR_B > 4090) UMBRAL_LDR_B = 4060;

  delay(100);
  stripGames.clear();
}

/* ================================================================= */
/*                        RECEPCION ESP-NOW                          */
/* ================================================================= */

void funcionA() {
  puntosA++;
  c = 'a';
  numero_del_blu = 1;
  beep(100);
}

void funcionB() {
  puntosB++;
  c = 'b';
  numero_del_blu = 2;
  beep(100);
}

void funcionD() {
  puntosA--;
  numero_del_blu = 3;
  puntosB--;
  numero_del_blu = 5;
  beep(100); delay(100); beep(100); delay(100); beep(100);
  c = 'd';
}

void funcionF() {
  puntosA = 0;
  puntosB = 0;
  beep(200); delay(100); beep(100); delay(100); beep(100);
  c = 'f';
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));

  switch (incomingMessage.value) {
    case 'a': funcionA(); break;
    case 'b': funcionB(); break;
    case 'd': funcionD(); break;
    case 'f': funcionF(); break;
  }
}

/* ================================================================= */
/*                        PANTALLAS (LEDS)                           */
/* ================================================================= */

// --- Modos de suma / voleibol ---
void mostrarDigitos(int digito, int offset, uint32_t color) {
  for (int s = 0; s < 7; s++) {
    stripPuntos.setPixelColor(offset + s,
      digitSegments[digito][s] ? color : 0);
  }
}

void mostrarMarcador() {
  stripPuntos.clear();

  int dA = puntosA / 10;
  int uA = puntosA % 10;
  int dB = puntosB / 10;
  int uB = puntosB % 10;

  uint32_t colorA = stripPuntos.Color(255, 0, 0);  // A: Rojo
  uint32_t colorB = stripPuntos.Color(0, 255, 0);  // B: Verde

  mostrarDigitos(dA, 0,  colorA);
  mostrarDigitos(uA, 7,  colorA);
  mostrarDigitos(dB, 14, colorB);
  mostrarDigitos(uB, 21, colorB);

  stripPuntos.show();
}

// --- Modo tenis ---
void mostrarDigito(int inicio, int valor, uint32_t color) {
  for (int i = 0; i < 14; i++) {
    if (digitos[valor][i])
      stripPuntos.setPixelColor(inicio + i, color);
    else
      stripPuntos.setPixelColor(inicio + i, COLOR_OFF);
  }
}

void actualizarDisplay() {
  stripPuntos.clear();

  if (tieBreak) {
    for (int i = 0; i < puntosA_T && i < 14; i++)
      stripPuntos.setPixelColor(i, COLOR_A);

    for (int i = 0; i < puntosB_T && i < 14; i++)
      stripPuntos.setPixelColor(i + 14, COLOR_B);
  }
  else {
    int valA = ventajaA ? 4 : puntosA_T;
    int valB = ventajaB ? 4 : puntosB_T;

    mostrarDigito(0,  valA, COLOR_A);
    mostrarDigito(14, valB, COLOR_B);
  }

  stripPuntos.show();
}

uint32_t colorPorGame(int g) {
  switch (g) {
    case 1: return stripGames.Color(255, 255, 255);
    case 2: return stripGames.Color(255, 255, 0);
    case 3: return stripGames.Color(0, 255, 0);
    case 4: return stripGames.Color(0, 0, 255);
    case 5: return stripGames.Color(255, 0, 0);
    case 6: return stripGames.Color(255, 50, 10);
  }
  return stripGames.Color(0, 0, 0);
}

void actualizarGames() {
  stripGames.clear();

  for (int i = 0; i < gamesA && i < 6; i++)
    stripGames.setPixelColor(i, colorPorGame(gamesA));

  for (int i = 0; i < gamesB && i < 6; i++)
    stripGames.setPixelColor(i + 6, colorPorGame(gamesB));

  stripGames.show();
}

/* ================================================================= */
/*              MODOS DE SUMA TOTAL (24 / 16) Y VOLEIBOL (25)        */
/* ================================================================= */

void resetMarcador() {
  puntosA = 0;
  puntosB = 0;
  ldrA_Activo = false;
  ldrB_Activo = false;
  calibracion();
  mostrarMarcador();
}

void parpadearResultado() {
  unsigned long inicio = millis();
  bool visible = true;

  while (millis() - inicio < 10000) {
    if (visible) mostrarMarcador();
    else { stripPuntos.clear(); stripPuntos.show(); }

    visible = !visible;
    beep(100);
    delay(300);
  }
  resetMarcador();
}

/* ================================================================= */
/*                           MODO TENIS                              */
/* ================================================================= */

void resetPuntos() {
  puntosA_T = 0;
  puntosB_T = 0;
  ventajaA = false;
  ventajaB = false;
}

void animacionVictoria(uint32_t color) {
  unsigned long inicio = millis();

  while (millis() - inicio < 10000) {
    stripPuntos.fill(0, 0, 0); stripPuntos.show();
    beep(100);
    delay(50);
    stripPuntos.fill(color); stripPuntos.show();
    beep(100);
    delay(100);
  }
  resetMarcador();
}

void terminarSet(char ganador) {
  setTerminado = true;
  tiempoFinSet = millis();
  animacionVictoria(ganador == 'A' ? COLOR_A : COLOR_B);
  calibracion();
}

void verificarSet() {

  // Mejor de 3 games
  if (modoCorto) {
    if (gamesA >= 2) {
      terminarSet('A');
    }
    else if (gamesB >= 2) {
      terminarSet('B');
    }
    return;
  }

  // Tie-break
  if (tieBreak) {
    if ((puntosA_T >= 7 || puntosB_T >= 7) && abs(puntosA_T - puntosB_T) >= 2) {
      terminarSet(puntosA_T > puntosB_T ? 'A' : 'B');
    }
    return;
  }

  // 6-6 entra a tie-break
  if (gamesA == 6 && gamesB == 6) {
    tieBreak = true;
    resetPuntos();
    return;
  }

  // Gana el set
  if ((gamesA >= 6 || gamesB >= 6) && abs(gamesA - gamesB) >= 2) {
    terminarSet(gamesA > gamesB ? 'A' : 'B');
  }
}

/* ===== LOGICA DE PUNTOS ===== */

void puntoA() {
  beep(100);

  if (tieBreak) {
    puntosA_T++;
    verificarSet();
    return;
  }

  if (puntosA_T < 3) puntosA_T++;
  else if (puntosA_T == 3 && puntosB_T < 3) { gamesA++; resetPuntos(); }
  else if (puntosA_T == 3 && puntosB_T == 3 && !ventajaA && !ventajaB) ventajaA = true;
  else if (ventajaA) { gamesA++; resetPuntos(); }
  else if (ventajaB) ventajaB = false;

  verificarSet();
}

void puntoB() {
  beep(100);

  if (tieBreak) {
    puntosB_T++;
    verificarSet();
    return;
  }

  if (puntosB_T < 3) puntosB_T++;
  else if (puntosB_T == 3 && puntosA_T < 3) { gamesB++; resetPuntos(); }
  else if (puntosA_T == 3 && puntosB_T == 3 && !ventajaA && !ventajaB) ventajaB = true;
  else if (ventajaB) { gamesB++; resetPuntos(); }
  else if (ventajaA) ventajaA = false;

  verificarSet();
}

/* ===== MARCADOR ESTILO TENIS ===== */
void OtroPrograma() {
  unsigned long ahora = millis();

  /* ----- Reinicio del set ----- */
  if (setTerminado && ahora - tiempoFinSet >= 10000) {
    gamesA = 0;
    gamesB = 0;
    tieBreak = false;
    setTerminado = false;
    resetPuntos();
    actualizarGames();
  }

  if (setTerminado) return;

  /* ----- Control remoto (si el interruptor Blue esta activo no se leen los LDR) ----- */
  btnBLUE = digitalRead(MODE_BLU);
  if (btnBLUE == 1) {
    ldrA = analogRead(LDR_A);
    ldrB = analogRead(LDR_B);
  } else {

    if (c == 'a') {
      beep(100);
      if (tieBreak) {
        puntosA_T++;
        verificarSet();
        return;
      }
      if (puntosA_T < 3) puntosA_T++;
      else if (puntosA_T == 3 && puntosB_T < 3) { gamesA++; resetPuntos(); }
      else if (puntosA_T == 3 && puntosB_T == 3 && !ventajaA && !ventajaB) ventajaA = true;
      else if (ventajaA) { gamesA++; resetPuntos(); }
      else if (ventajaB) ventajaB = false;
      verificarSet();
    }

    if (c == 'b') {
      beep(100);
      if (tieBreak) {
        puntosB_T++;
        verificarSet();
        return;
      }
      if (puntosB_T < 3) puntosB_T++;
      else if (puntosB_T == 3 && puntosA_T < 3) { gamesB++; resetPuntos(); }
      else if (puntosA_T == 3 && puntosB_T == 3 && !ventajaA && !ventajaB) ventajaB = true;
      else if (ventajaB) { gamesB++; resetPuntos(); }
      else if (ventajaA) ventajaA = false;
      verificarSet();
    }

    if (c == 'c') {
      if (puntosA_T > 0) puntosA_T--;
      beep(100); delay(100); beep(100);
    }

    if (c == 'e') {
      if (puntosB_T > 0) puntosB_T--;
      beep(100); delay(100); beep(100);
    }

    if (c == 'd') {
      if (puntosA_T > 0) puntosA_T--;
      if (puntosB_T > 0) puntosB_T--;
      beep(100); delay(100); beep(100);
    }

    if (c == 'f') {
      if (puntosA_T > 0) puntosA_T = 0;
      if (puntosB_T > 0) puntosB_T = 0;
      beep(100); delay(100); beep(100);
    }

    c = 'x';   // comando ya atendido
  }

  /* ----- Jugador A (LDR) ----- */
  if (ldrA > UMBRAL_LDR) {
    if (!ldrAActivo) {
      beep(100);
      ldrAActivo = true;
      ldrATiempo = ahora;
    }
  } else if (ldrAActivo) {
    unsigned long d = ahora - ldrATiempo;
    if (d >= RESTA_TIEMPO) {
      if (puntosA_T > 0) puntosA_T--;
      beep(100); delay(100); beep(100);
    }
    else if (d >= SUMA_TIEMPO) {
      beep(100);
      puntoA();
    }
    ldrAActivo = false;
  }

  /* ----- Jugador B (LDR) ----- */
  if (ldrB > UMBRAL_LDR_B) {
    if (!ldrBActivo) {
      beep(100);
      ldrBActivo = true;
      ldrBTiempo = ahora;
    }
  } else if (ldrBActivo) {
    unsigned long d = ahora - ldrBTiempo;
    if (d >= RESTA_TIEMPO) {
      if (puntosB_T > 0) puntosB_T--;
      beep(100); delay(100); beep(100);
    }
    else if (d >= SUMA_TIEMPO) {
      beep(100);
      puntoB();
    }
    ldrBActivo = false;
  }

  actualizarDisplay();
  actualizarGames();
}

/* ================================================================= */
/*                              SETUP                                */
/* ================================================================= */
void setup() {

  // Pines
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_BRIGHT, INPUT_PULLUP);
  pinMode(MODO_SWITCH, INPUT_PULLUP);
  pinMode(pin_calibrar, INPUT);

  pinMode(MODE_24, INPUT_PULLUP);
  pinMode(MODE_16, INPUT_PULLUP);
  pinMode(MODE_25, INPUT_PULLUP);
  pinMode(MODE_BLU, INPUT_PULLUP);

  // Tiras LED
  stripPuntos.begin();
  stripGames.begin();

  stripPuntos.setBrightness(nivelesBrillo[0]);
  stripGames.setBrightness(nivelesBrillo[0]);

  stripPuntos.show();
  stripGames.show();

  Serial.begin(115200);

  // Colores
// Colores de los jugadores
 COLOR_A   = stripPuntos.Color(255, 0, 0);  // Jugador A: Rojo
 COLOR_B   = stripPuntos.Color(0, 255, 0);  // Jugador B: Verde
 COLOR_OFF = stripPuntos.Color(0, 0, 0);    // Apagado

  actualizarDisplay();
  actualizarGames();

  // ESP-NOW
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error iniciando ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receptor listo");

  // Aviso de arranque y calibracion
  beep(200); delay(400);
  beep(200); delay(400);
  beep(200); delay(400);
  calibracion();
}

/* ================================================================= */
/*                               LOOP                                */
/* ================================================================= */
void loop() {

  // Si MODE_BLU esta en LOW se ignoran los LDR (solo control remoto)
  bool modoBluetooth = digitalRead(MODE_BLU) == LOW;

  if (!modoBluetooth) {
    valorLdrA = analogRead(LDR_A);
    valorLdrB = analogRead(LDR_B);
    bool ldrA_act = valorLdrA < UMBRAL_LDR;
    bool ldrB_act = valorLdrB < UMBRAL_LDR_B;
    if (ldrA_act && !ldrA_prev && puntosA < 99) { puntosA++; beep(100); }
    if (ldrB_act && !ldrB_prev && puntosB < 99) { puntosB++; beep(100); }
    ldrA_prev = ldrA_act;
    ldrB_prev = ldrB_act;
  }

  modoCorto = (digitalRead(MODO_SWITCH) == LOW);

  /* ===== BOTON DE BRILLO ===== */
  bool btnActual = digitalRead(BTN_BRIGHT);
  if (btnActual == LOW && btnEstadoAnterior == HIGH) {
    brilloIndex++;
    if (brilloIndex > 3) brilloIndex = 0;
    stripPuntos.setBrightness(nivelesBrillo[brilloIndex]);
    stripGames.setBrightness(nivelesBrillo[brilloIndex]);
    stripPuntos.show();
    stripGames.show();
    delay(200);
  }
  btnEstadoAnterior = btnActual;

  /* ===== MODO TENIS (ningun otro modo activo) ===== */
  if (digitalRead(MODE_24) && digitalRead(MODE_16) && digitalRead(MODE_25)) {
    OtroPrograma();
    return;
  }

  mostrarMarcador();

  /* ===== MODO SUMA TOTAL 24 ===== */
  if (!digitalRead(MODE_24)) {
    if (puntosA + puntosB >= 21) parpadearResultado();
  }

  /* ===== MODO SUMA TOTAL 16 ===== */
  if (!digitalRead(MODE_16)) {
    if (puntosA + puntosB >= 28) parpadearResultado();
  }

  /* ===== MODO 25 CON VENTAJA DE 2 ===== */
  if (!digitalRead(MODE_25)) {
    if ((puntosA >= 25 || puntosB >= 25) &&
        abs(puntosA - puntosB) >= 2) {
      parpadearResultado();
    }
  }

  delay(40);
}
