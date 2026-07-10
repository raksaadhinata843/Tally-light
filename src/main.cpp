#include <Arduino.h>
#include <ArduinoOTA.h>

// Deklarasi Tipe Data
typedef struct __attribute__((packed)) {
    uint8_t pgm_mask;
    uint8_t pvw_mask;
} TallyPacket;

// ====================================================================
// --- TX CONFIGURATION ESP32(SwITCHER)---
// ====================================================================
#ifdef MODE_TX_SWITCHER32
#include <WiFi.h>
#include <esp_now.h>

const uint8_t PGM_PINS[4] = {12, 13, 25, 26}; 
const uint8_t PVW_PINS[4] = {27, 32, 33, 34};
const int MODE_SWITCH_PIN = 14;

uint8_t broadcastAddress[4][6] = {
  {0x24,0xD7,0xEB,0xCD,0x27,0x3D},
 // {0x20,0x50,0x0d,0xd2,0x38,0x8c},
  {0x24,0xD7,0xEB,0xCD,0x27,0x5D},
  {0x24,0xD7,0xEB,0xCD,0x27,0x6D}
};
bool modePC = true;
bool lastButtonState = HIGH;

TallyPacket txPacket;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;

  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  for(int i = 0; i < 4; i++) {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress[i], 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
}

void checkModeButton() {
  bool currentButtonState = digitalRead(MODE_SWITCH_PIN);
  
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    modePC = !modePC;
    Serial.println("Switch");
    delay(200);
  }
  lastButtonState = currentButtonState;
}

void updateManualData() {
  txPacket.pgm_mask = 0;
  txPacket.pvw_mask = 0;
  for(int i = 0; i < 4; i++) {
    if (digitalRead(PGM_PINS[i]) == LOW) txPacket.pgm_mask |= (1 << i);
    if (digitalRead(PVW_PINS[i]) == LOW) txPacket.pvw_mask |= (1 << i);
  }
}

void loop() {
  checkModeButton();

  if (modePC) {
    if (Serial.available() >= 2) {
      txPacket.pgm_mask = Serial.read();
      txPacket.pvw_mask = Serial.read();
    }
  } else {
    updateManualData();
  }
  esp_now_send(NULL, (uint8_t*)&txPacket, sizeof(txPacket));
  delay(50);
}
#endif

// ====================================================================
// --- TX CONFIGURATION ESP32(UDP) ---
// ====================================================================
#ifdef MODE_TX_ESP32UDP
#include <WiFi.h>
#include <WiFiUdp.h>

// --- KONFIGURASI ---
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
WiFiUDP udp;

const uint8_t PGM_PINS[4] = {12, 13, 25, 26}; 
const uint8_t PVW_PINS[4] = {27, 32, 33, 34};

TallyPacket txPacket;

void setup() {
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("Tally-ESP32");
  ArduinoOTA.begin();
  
  udp.begin(4210);
  for (int i = 0; i < 4; i++) {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
  }
}

void loop() {
  ArduinoOTA.handle();
  txPacket.pgm_mask = 0;
  txPacket.pvw_mask = 0;
  for (int i = 0; i < 4; i++) {
    if (digitalRead(PGM_PINS[i]) == LOW) txPacket.pgm_mask |= (1 << i);
    if (digitalRead(PVW_PINS[i]) == LOW) txPacket.pvw_mask |= (1 << i);
  }

  udp.beginPacket("239.1.2.3", 4210);
  udp.write((uint8_t*)&txPacket, sizeof(txPacket));
  udp.endPacket();

  delay(50);
}
#endif

// ====================================================================
// --- TX CONFIGURATION ESP8266(UDP) ---
// ====================================================================
#ifdef MODE_TX_ESP8266UDP
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Rec.709"; 
const char* password = "malammalam";
const int udpPort = 8888;
WiFiUDP udp;

const uint8_t PGM_PINS[4] = {D1, D2, D3, D5}; 
const uint8_t PVW_PINS[4] = {D6, D7, D8, RX}; 

TallyPacket txPacket;

void setup() {
  Serial.begin(115200);
  
  // Konfigurasi IP Statis (Opsional tapi disarankan)
  IPAddress local_IP(192, 168, 0, 5);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  udp.begin(udpPort);

  for (int i = 0; i < 4; i++) {
      pinMode(PGM_PINS[i], INPUT_PULLUP);
      pinMode(PVW_PINS[i], INPUT_PULLUP);
  }
}

void loop() {
  txPacket.pgm_mask = 0;
  txPacket.pvw_mask = 0;
  for(int i = 0; i < 4; i++) {
    if (digitalRead(PGM_PINS[i]) == LOW) txPacket.pgm_mask |= (1 << i);
    if (digitalRead(PVW_PINS[i]) == LOW) txPacket.pvw_mask |= (1 << i);
  }
  udp.beginPacket(IPAddress(192, 168, 0, 255), udpPort);
  udp.write((uint8_t*)&txPacket, sizeof(txPacket));
  udp.endPacket();
  delay(10);
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266---
// ====================================================================
#ifdef MODE_RX_ESP8266
#include <ESP8266WiFi.h>
#include <espnow.h>

#define RED 5    // D1
#define GREEN 4  // D2
#define BLUE 14  // D5

// Tentukan ID Tally ini (Misal ID 1, ID 2, dst)
const uint8_t CAM_ID = 1; 

TallyPacket rxPacket;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
    if (len == sizeof(TallyPacket)) {
        memcpy(&rxPacket, data, sizeof(TallyPacket));
    }
}

void setup() {
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    if (esp_now_init() != 0) return;
    
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    // Gunakan Bitwise untuk cek apakah ID ini ada di dalam mask
    // (1 << (CAM_ID - 1)) akan menghasilkan bit yang sesuai untuk ID tersebut
    bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID - 1)));
    bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID - 1)));

    if (isPgm) {
        // Status PGM (Merah)
        digitalWrite(RED, HIGH);
        digitalWrite(GREEN, LOW);
        digitalWrite(BLUE, LOW);
    } else if (isPvw) {
        // Status PVW (Hijau)
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, HIGH);
        digitalWrite(BLUE, LOW);
    } else {
        // Status IDLE (Biru)
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, LOW);
        digitalWrite(BLUE, HIGH);
    }
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP32---
// ====================================================================
#ifdef MODE_RX_ESP32
#include <WiFi.h>
#include <esp_now.h>

#define RED 18
#define GREEN 19
#define BLUE 21

// Tentukan ID Tally ini (Misal ID 1, ID 2, dst)
const uint8_t CAM_ID = 2; 

TallyPacket rxPacket;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
    if (len == sizeof(TallyPacket)) {
        memcpy(&rxPacket, data, sizeof(TallyPacket));
    }
}

void setup() {
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    if (esp_now_init() != 0) return;
}

void loop() {
    // Gunakan Bitwise untuk cek apakah ID ini ada di dalam mask
    // (1 << (CAM_ID - 1)) akan menghasilkan bit yang sesuai untuk ID tersebut
    bool isPgm = (rxPacket.pgm_mask & (2 << (CAM_ID - 2)));
    bool isPvw = (rxPacket.pvw_mask & (2 << (CAM_ID - 2)));

    if (isPgm) {
        // Status PGM (Merah)
        digitalWrite(RED, HIGH);
        digitalWrite(GREEN, LOW);
        digitalWrite(BLUE, LOW);
    } else if (isPvw) {
        // Status PVW (Hijau)
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, HIGH);
        digitalWrite(BLUE, LOW);
    } else {
        // Status IDLE (Biru)
        digitalWrite(RED, LOW);
        digitalWrite(GREEN, LOW);
        digitalWrite(BLUE, HIGH);
    }
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP32 (UDP)---
// ====================================================================
#ifdef MODE_RX_ESP32UDP
#include <WiFi.h>
#include <WiFiUdp.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define RED 25
#define GREEN 26
#define BLUE 27

// Tentukan ID Tally ini (0, 1, 2, 3)
const uint8_t CAM_ID = 1; 

TallyPacket rxPacket;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const int udpPort = 55755;
WiFiUDP udp;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

void setup() {
  Serial.begin(115200);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    
  WiFi.begin(ssid, password);
    
  udp.begin(udpPort);
}

void loop() {
    int packetSize = udp.parsePacket();
    if (packetSize == sizeof(TallyPacket)) {
        TallyPacket rxPacket;
        udp.read((unsigned char*)&rxPacket, sizeof(TallyPacket));

        // LOGIKA BITWISE YANG BENAR:
        // Gunakan operator '&' untuk men-masking bit spesifik
        bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID - 1)));
        bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID - 1)));

        if (isPgm) {
            Serial.println("Red");
            digitalWrite(RED, HIGH);
            digitalWrite(GREEN, LOW);
            digitalWrite(BLUE, LOW);
        } else if (isPvw) {
            Serial.println("Green");
            digitalWrite(RED, LOW);
            digitalWrite(GREEN, HIGH);
            digitalWrite(BLUE, LOW);
        } else {
            Serial.println("Blue");
            digitalWrite(RED, LOW);
            digitalWrite(GREEN, LOW);
            digitalWrite(BLUE, HIGH);
        }
    }
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP32WS (UDP)---
// ====================================================================
#ifdef MODE_RX_ESP32UDP_WS
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

#define PIN        5
#define NUMPIXELS  1

TallyPacket rxPacket;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
WiFiUDP udp;

Adafruit_NeoPixel leds(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void recon() {
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED) {
    leds.setPixelColor(0, leds.Color(255, 0, 0));
    leds.show();
    delay(500);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(500);
  }
  udp.beginMulticast(IPAddress(239, 1, 2, 3), 4210);
}

void setup() {
  leds.begin();
  leds.setBrightness(50);
  leds.setPixelColor(0, leds.Color(0, 0, 0));
  leds.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    leds.setPixelColor(0, leds.Color(255, 0, 0));
    leds.show();
    delay(500);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(500);
  }

  ArduinoOTA.setHostname(("tally-cam" + String(CAM_ID)).c_str());
  ArduinoOTA.begin();

  // Flash putih sebanyak CAM_ID+1 kali agar ESP bisa dikenali
  delay(500);
  for (uint8_t i = 0; i <= CAM_ID; i++) {
    leds.setPixelColor(0, leds.Color(255, 255, 255));
    leds.show();
    delay(300);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(300);
  }

  // Indikator biru = siap menerima
  leds.setPixelColor(0, leds.Color(0, 0, 255));
  leds.show();

  udp.beginMulticast(IPAddress(239, 1, 2, 3), 4210);
}

void loop() {
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) {
    recon();
    return;
  }

  int packetSize = udp.parsePacket();
  if (packetSize == sizeof(TallyPacket)) {
    TallyPacket rxPacket;
    udp.read((unsigned char*)&rxPacket, sizeof(TallyPacket));

    bool isPgm = (rxPacket.pgm_mask & (1 << CAM_ID));
    bool isPvw = (rxPacket.pvw_mask & (1 << CAM_ID));

    if (isPgm) {
      leds.setPixelColor(0, leds.Color(255, 0, 0));
    } else if (isPvw) {
      leds.setPixelColor(0, leds.Color(0, 255, 0));
    } else {
      leds.setPixelColor(0, leds.Color(0, 0, 255));
    }

    leds.show();
  }
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266WS (UDP)---
// ====================================================================
#ifdef MODE_RX_ESP8266UDP_WS
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

#define PIN        4 //D2
#define NUMPIXELS  1

TallyPacket rxPacket;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
WiFiUDP udp;

Adafruit_NeoPixel leds(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void recon() {
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED) {
    leds.setPixelColor(0, leds.Color(255, 0, 0));
    leds.show();
    delay(500);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(500);
  }
  udp.beginMulticast(WiFi.localIP(), IPAddress(239, 1, 2, 3), 4210);
}

void setup() {
  leds.begin();
  leds.setBrightness(50);
  leds.setPixelColor(0, leds.Color(0, 0, 0));
  leds.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    leds.setPixelColor(0, leds.Color(255, 0, 0));
    leds.show();
    delay(500);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(500);
  }

  ArduinoOTA.setHostname(("tally-cam" + String(CAM_ID)).c_str());
  ArduinoOTA.begin();

  // Flash putih sebanyak CAM_ID+1 kali agar ESP bisa dikenali
  delay(500);
  for (uint8_t i = 0; i <= CAM_ID; i++) {
    leds.setPixelColor(0, leds.Color(255, 255, 255));
    leds.show();
    delay(300);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(300);
  }

  // Indikator biru = siap menerima
  leds.setPixelColor(0, leds.Color(0, 0, 255));
  leds.show();

  udp.beginMulticast(WiFi.localIP(), IPAddress(239, 1, 2, 3), 4210);
}

void loop() {
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) {
    recon();
    return;
  }

  int packetSize = udp.parsePacket();
  if (packetSize == sizeof(TallyPacket)) {
    TallyPacket rxPacket;
    udp.read((unsigned char*)&rxPacket, sizeof(TallyPacket));

    bool isPgm = (rxPacket.pgm_mask & (1 << CAM_ID));
    bool isPvw = (rxPacket.pvw_mask & (1 << CAM_ID));

    if (isPgm) {
      leds.setPixelColor(0, leds.Color(255, 0, 0));
    } else if (isPvw) {
      leds.setPixelColor(0, leds.Color(0, 255, 0));
    } else {
      leds.setPixelColor(0, leds.Color(0, 0, 255));
    }

    leds.show();
  }
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266(WS)---
// ====================================================================
#ifdef MODE_RX_ESP8266_WS
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <FastLED.h>

#define PIN        5  
#define NUMPIXELS  1

// Tentukan ID Tally ini (Misal ID 1, ID 2, dst)
const uint8_t CAM_ID = 1; 

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

CRGB leds[NUMPIXELS];

TallyPacket rxPacket;

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
    if (len == sizeof(TallyPacket)) {
        memcpy(&rxPacket, data, sizeof(TallyPacket));
    }
}

void setup() {
    FastLED.addLeds<WS2812, PIN, GRB>(leds, NUMPIXELS);
    FastLED.setBrightness(50);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    if (esp_now_init() != 0) return;
    
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    // Gunakan Bitwise untuk cek apakah ID ini ada di dalam mask
    // (1 << (CAM_ID - 1)) akan menghasilkan bit yang sesuai untuk ID tersebut
    bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID - 1)));
    bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID - 1)));

    pixels.clear();

    if (isPgm) {
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    } else if (isPvw) {
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    } else {
        pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    }

    pixels.show();
}
#endif

// ====================================================================
// --- TX CONFIGURATION ANANO ---
// ====================================================================
#ifdef MODE_TX_NANO
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Struktur Data yang dikirim lewat Udara (Persis seperti logika ESP Anda)
struct Packet {
  uint8_t pgm_mask; 
  uint8_t pvw_mask; 
};

Packet pkt;

// Inisialisasi pin CE dan CSN untuk NRF24L01
RF24 radio(7, 8); 
const byte address[6] = "TALLY"; // Alamat pipa komunikasi (Harus sama dengan Receiver)

// Pemetaan Pin Fisik DB15 (Skenario Kamera 1-4 untuk IN3-IN6 Switcher)
// A0-A3 pada Arduino Nano bisa diinisialisasi langsung sebagai pin digital input
const uint8_t PGM_PINS[4] = {A2, A0, 4, 2}; // Kamera 1-4 PGM
const uint8_t PVW_PINS[4] = {A3, A1, 5, 3}; // Kamera 1-4 PVW

const uint8_t LED_MODE = 6; // LED Indikator Mode vMix Aktif

const unsigned long DEBOUNCE_MS = 50;
const unsigned long SERIAL_TIMEOUT = 2000;
const unsigned long SEND_INTERVAL = 100;

unsigned long lastSerialRx = 0;
unsigned long lastSend = 0;
bool vmixActive = false;

uint8_t readPhysicalMask(const uint8_t pins[4]) {
  uint8_t m = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (digitalRead(pins[i]) == LOW) {
      m |= (1 << i);
    }
  }
  return m;
}

void setup() {
  Serial.begin(115200); // Digunakan untuk menangkap data serial vMix komputer
  
  // Inisialisasi Modul Wireless NRF24L01
  if (!radio.begin()) {
    while (1); // Kunci di sini jika modul NRF rusak/kabelnya salah pasang
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MAX); // Jangkauan sinyal maksimum
  radio.stopListening();         // Set sebagai Transmitter (TX)

  // Inisialisasi Seluruh Pin Input (Menggunakan Pull-up internal 5V yang kuat)
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
  }
  pinMode(LED_MODE, OUTPUT);
}

void loop() {
  // 1. Deteksi Mode Otomatis (vMix Serial vs Fisik DB15)
  vmixActive = (millis() - lastSerialRx) <= SERIAL_TIMEOUT;
  digitalWrite(LED_MODE, vmixActive ? HIGH : LOW);

  // 2. Baca Data Serial vMix ("PGM_MASK,PVW_MASK\n")
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    int commaIndex = data.indexOf(',');
    if (commaIndex > 0) {
      pkt.pgm_mask = data.substring(0, commaIndex).toInt();
      pkt.pvw_mask = data.substring(commaIndex + 1).toInt();
      lastSerialRx = millis(); 
    }
  }

  // 3. Baca Data Fisik DB15 jika vMix tidak aktif
  if (!vmixActive) {
    static unsigned long lastPhysicalRead = 0;
    if (millis() - lastPhysicalRead >= DEBOUNCE_MS) {
      lastPhysicalRead = millis();
      pkt.pgm_mask = readPhysicalMask(PGM_PINS);
      pkt.pvw_mask = readPhysicalMask(PVW_PINS);
    }
  }

  // 4. Kirim Paket via NRF24L01 Secara Berkala
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();
    radio.write(&pkt, sizeof(pkt));
  }
}
#endif

// ====================================================================
// --- RX CONFIGURATION ANANO ---
// ====================================================================
#ifdef MODE_RX_NANO
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Definisikan nomor kamera untuk board penerima ini (Contoh: Lampu Kamera 1)
const uint8_t CAM_ID = 1; 

// Definisi Pin Output LED Tally sesuai Tabel Hardware
#define RED 5    
#define GREEN 4  
#define BLUE 3   

// Struktur Data harus sama persis dengan yang dikirim oleh Transmitter
struct Packet {
  uint8_t pgm_mask; 
  uint8_t pvw_mask; 
};

Packet receivedPkt;

RF24 radio(7, 8); // Pin CE, CSN
const byte address[6] = "TALLY"; // Alamat pipa harus sama dengan Transmitter

void setup() {
  Serial.begin(115200);
  
  pinMode(RED, OUTPUT); 
  pinMode(GREEN, OUTPUT); 
  pinMode(BLUE, OUTPUT);
  
  // Tes lampu saat dinyalakan (opsional, untuk memastikan LED berfungsi)
  digitalWrite(BLUE, HIGH);
  digitalWrite(BLUE, LOW);

  // Inisialisasi Modul Wireless nRF24L01
  if radio.begin() =! ;
  
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_MAX); // Jangkauan maksimum
  radio.startListening();
}

void loop() {
  // Cek apakah ada data masuk melalui udara
  if (radio.available()) {
    // Baca data paket bitmask
    radio.read(&receivedPkt, sizeof(receivedPkt));

    // Dekode status kamera berdasarkan CAM_ID yang ditentukan di atas
    bool isPgm = (receivedPkt.pgm_mask & (1 << (CAM_ID - 1))) != 0;
    bool isPvw = (receivedPkt.pvw_mask & (1 << (CAM_ID - 1))) != 0;

    // Kendali Lampu Tally
    if (isPgm) {
      digitalWrite(RED, HIGH);    // Merah jika LIVE (Program)
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, LOW);
    } 
    else if (isPvw) {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, HIGH);  // Hijau jika Standby (Preview)
      digitalWrite(BLUE, LOW);
    } 
    else {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, HIGH);   // Biru jika Kamera tidak aktif (Idle)
    }
  }
}
#endif


#ifdef MODE_TX_ESP32_MESH
#include "esp_mesh.h"

#define MESH_PREFIX   "Rec.709"
#define MESH_PASSWORD "malammalam"
#define MESH_PORT     5555

painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
}

void loop() {
  mesh.update();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 3000) { // Kirim perintah tiap 3 detik
    
    // CONTOH PERINTAH: Kita mau suruh RX ID 2 menyalakan warna MERAH dan BIRU (Ungu)
    uint8_t targetRX = 2;       // ID 2  -> (Binary: 0010)
    uint8_t warnaRGB = 0b101;   // R=1, G=0, B=1 (Ungu)
    
    // Gabungkan menggunakan bitwise OR dan Shift Left
    // Geser ID target ke 4 bit kiri, lalu gabungkan dengan warna
    uint8_t dataKirim = (targetRX << 4) | warnaRGB; 

    // Kirim data secara broadcast ke semua mesh
    mesh.sendBroadcast(String(dataKirim));
    
    Serial.printf("TX Mengirim Data: %d (Binary: 0b", dataKirim);
    for (int i = 7; i >= 0; i--) Serial.print((dataKirim >> i) & 1);
    Serial.println(")");

    lastSend = millis();
  }
}
#endif

#ifdef MODE_RX_ESP32_MESH
#include <esp_mesh.h>

#define MESH_PREFIX   "Rec.709"
#define MESH_PASSWORD "malammalam"
#define MESH_PORT     5555

// !!! UBAH ID INI DI SETIAP BOARD RX (1 sampai 15) !!!
#define MY_RX_ID      2  

// Definisikan Pin LED RGB (Sesuaikan dengan pin ESP32/ESP8266 lu)
#define PIN_R         25 
#define PIN_G         26
#define PIN_B         27

painlessMesh mesh;

// Fungsi masking untuk membaca data masukan
void receivedCallback(uint32_t from, String &msg) {
  uint8_t dataDiterima = msg.toInt();

  // 1. Ekstrak ID Target menggunakan Bitmask 0xF0 (0b11110000) dan geser kanan 4 kali
  uint8_t targetID = (dataDiterima & 0xF0) >> 4;

  // Cek apakah data ini memang ditujukan untuk RX ini
  if (targetID == MY_RX_ID) {
    Serial.printf("Data cocok untuk RX [%d]! Memproses warna...\n", MY_RX_ID);

    // 2. Ekstrak status RGB menggunakan Bitmask 0x07 (0b00000111)
    bool statusR = dataDiterima & 0x04; // Cek Bit 2 (Red)  -> 0b100
    bool statusG = dataDiterima & 0x02; // Cek Bit 1 (Green)-> 0b010
    bool statusB = dataDiterima & 0x01; // Cek Bit 0 (Blue) -> 0b001

    // 3. Eksekusi ke Pin Hardware (Asumsi LED Common Cathode / Aktif HIGH)
    digitalWrite(PIN_R, statusR ? HIGH : LOW);
    digitalWrite(PIN_G, statusG ? HIGH : LOW);
    digitalWrite(PIN_B, statusB ? HIGH : LOW);
    
    Serial.printf("Status Output -> R:%d G:%d B:%d\n", statusR, statusG, statusB);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
}

void loop() {
  mesh.update();
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP32 ARBITER (Tally Arbiter over WebSocket) ---
// ====================================================================
#ifdef MODE_RX_ARBITER_ESP32
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

#define LED_PIN     5
#define LED_COUNT   1
#define BRIGHTNESS  50

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define ARB_WHITE leds.Color(255, 255, 255)
#define ARB_BLACK leds.Color(0, 0, 0)

String listenerDeviceName = "tally-arbiter";
char tallyarbiter_host[40] = "192.168.1.2";
char tallyarbiter_port[6]  = "4455";

Preferences preferences;
WiFiManager wm;
SocketIOclient socket;

JSONVar BusOptions;
JSONVar Devices;
JSONVar DeviceStates;
String DeviceId = "unassigned";
String DeviceName = "Unassigned";
String prevType = "";
String actualType = "";
String actualColor = "";
int actualPriority = 0;
bool networkConnected = false;

void setColor(uint32_t color) {
  for (int i = 0; i < leds.numPixels(); i++) {
    leds.setPixelColor(i, color);
  }
  leds.show();
}

String stripQuot(String str) {
  if (str[0] == '"') str.remove(0, 1);
  if (str.endsWith("\"")) str.remove(str.length() - 1, 1);
  return str;
}

void wsEmit(String event, const char* payload = NULL) {
  String msg = payload ? ("[\"" + event + "\"," + payload + "]") : ("[\"" + event + "\"]");
  socket.sendEVENT(msg);
}

String getBusTypeById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++) {
    if (JSON.stringify(BusOptions[i]["id"]) == busId) return JSON.stringify(BusOptions[i]["type"]);
  }
  return "invalid";
}

String getBusColorById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++) {
    if (JSON.stringify(BusOptions[i]["id"]) == busId) return JSON.stringify(BusOptions[i]["color"]);
  }
  return "invalid";
}

int getBusPriorityById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++) {
    if (JSON.stringify(BusOptions[i]["id"]) == busId) return JSON.stringify(BusOptions[i]["priority"]).toInt();
  }
  return 0;
}

void evaluateMode() {
  if (actualType == prevType) return;
  actualColor.replace("#", "");
  long number = actualType != "" ? strtol(actualColor.c_str(), NULL, 16) : 0;
  int r = (number >> 16) & 0xFF;
  int g = (number >> 8) & 0xFF;
  int b = number & 0xFF;
  setColor(actualType != "" ? leds.Color(r, g, b) : ARB_BLACK);
  prevType = actualType;
}

void setDeviceName() {
  for (int i = 0; i < Devices.length(); i++) {
    if (JSON.stringify(Devices[i]["id"]) == "\"" + DeviceId + "\"") {
      String strDevice = JSON.stringify(Devices[i]["name"]);
      DeviceName = strDevice.substring(1, strDevice.length() - 1);
      break;
    }
  }
  preferences.begin("tally-arbiter", false);
  preferences.putString("devicename", DeviceName);
  preferences.end();
  evaluateMode();
}

void processTallyData() {
  bool typeChanged = false;
  for (int i = 0; i < DeviceStates.length(); i++) {
    if (DeviceStates[i]["sources"].length() > 0) {
      typeChanged = true;
      actualType = getBusTypeById(JSON.stringify(DeviceStates[i]["busId"]));
      actualColor = getBusColorById(JSON.stringify(DeviceStates[i]["busId"]));
      actualPriority = getBusPriorityById(JSON.stringify(DeviceStates[i]["busId"]));
    }
  }
  if (!typeChanged) {
    actualType = "";
    actualColor = "";
    actualPriority = 0;
  }
  evaluateMode();
}

void socketFlash() {
  leds.setBrightness(255);
  for (int i = 0; i < 3; i++) {
    setColor(ARB_WHITE);
    delay(300);
    setColor(ARB_BLACK);
    delay(300);
  }
  leds.setBrightness(BRIGHTNESS);
  evaluateMode();
}

void socketReassign(String payload) {
  String oldDeviceId = payload.substring(0, payload.indexOf(','));
  String newDeviceId = payload.substring(oldDeviceId.length() + 1);
  newDeviceId = newDeviceId.substring(0, newDeviceId.indexOf(','));
  oldDeviceId = stripQuot(oldDeviceId);
  newDeviceId = stripQuot(newDeviceId);

  String reassignObj = "{\"oldDeviceId\": \"" + oldDeviceId + "\", \"newDeviceId\": \"" + newDeviceId + "\"}";
  wsEmit("listener_reassign_object", reassignObj.c_str());
  wsEmit("devices");

  setColor(ARB_WHITE);
  delay(200);
  setColor(ARB_BLACK);

  DeviceId = newDeviceId;
  preferences.begin("tally-arbiter", false);
  preferences.putString("deviceid", newDeviceId);
  preferences.end();
  setDeviceName();
}

void socketConnected() {
  String deviceObj = "{\"deviceId\": \"" + DeviceId + "\", \"listenerType\": \"" + listenerDeviceName + "\", \"canBeReassigned\": true, \"canBeFlashed\": true, \"supportsChat\": false }";
  wsEmit("listenerclient_connect", deviceObj.c_str());
}

void socketEvent(socketIOmessageType_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case sIOtype_CONNECT:
      socketConnected();
      break;
    case sIOtype_EVENT: {
      String msg = (char*)payload;
      String evt = msg.substring(2, msg.indexOf("\"", 2));
      String content = msg.substring(evt.length() + 4);
      content.remove(content.length() - 1);

      if (evt == "bus_options") BusOptions = JSON.parse(content);
      if (evt == "reassign") socketReassign(content);
      if (evt == "flash") socketFlash();
      if (evt == "deviceId") {
        DeviceId = content.substring(1, content.length() - 1);
        setDeviceName();
      }
      if (evt == "devices") {
        Devices = JSON.parse(content);
        setDeviceName();
      }
      if (evt == "device_states") {
        DeviceStates = JSON.parse(content);
        processTallyData();
      }
      break;
    }
    default:
      break;
  }
}

void saveParamCallback() {
  String str_taHost = wm.server->hasArg("taHostIP") ? wm.server->arg("taHostIP") : "";
  String str_taPort = wm.server->hasArg("taHostPort") ? wm.server->arg("taHostPort") : "";
  preferences.begin("tally-arbiter", false);
  preferences.putString("taHost", str_taHost);
  preferences.putString("taPort", str_taPort);
  preferences.end();
}

void connectToNetwork() {
  WiFi.mode(WIFI_STA);

  WiFiManagerParameter custom_taServer("taHostIP", "Tally Arbiter Server", tallyarbiter_host, 40);
  WiFiManagerParameter custom_taPort("taHostPort", "Port", tallyarbiter_port, 6);
  wm.addParameter(&custom_taServer);
  wm.addParameter(&custom_taPort);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setConfigPortalTimeout(120);

  networkConnected = wm.autoConnect(listenerDeviceName.c_str());
}

void connectToServer() {
  socket.onEvent(socketEvent);
  socket.begin(tallyarbiter_host, atoi(tallyarbiter_port));
}

void setup() {
  leds.begin();
  leds.setBrightness(BRIGHTNESS);
  setColor(leds.Color(0, 0, 255));

  uint64_t chipid = ESP.getEfuseMac();
  listenerDeviceName = "tally-" + String((uint32_t)chipid, HEX);

  preferences.begin("tally-arbiter", false);
  if (preferences.getString("deviceid").length() > 0) DeviceId = preferences.getString("deviceid");
  if (preferences.getString("devicename").length() > 0) DeviceName = preferences.getString("devicename");
  if (preferences.getString("taHost").length() > 0) preferences.getString("taHost").toCharArray(tallyarbiter_host, 40);
  if (preferences.getString("taPort").length() > 0) preferences.getString("taPort").toCharArray(tallyarbiter_port, 6);
  preferences.end();

  connectToNetwork();
  while (!networkConnected) {
    setColor(leds.Color(255, 0, 0));
    delay(300);
    setColor(ARB_BLACK);
    delay(300);
  }

  ArduinoOTA.setHostname(listenerDeviceName.c_str());
  ArduinoOTA.begin();

  setColor(leds.Color(0, 255, 0));
  delay(500);

  connectToServer();
}

void loop() {
  ArduinoOTA.handle();
  socket.loop();
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266 ARBITER (Tally Arbiter over WebSocket) ---
// ====================================================================
#ifdef MODE_RX_ARBITER_ESP8266
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

#define LED_PIN     4 //D2
#define LED_COUNT   1
#define BRIGHTNESS  50

#define EEPROM_SIZE      256
#define ADDR_DEVICEID    0
#define ADDR_DEVICENAME  40
#define ADDR_TAHOST      80
#define ADDR_TAPORT      120

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define ARB_WHITE leds.Color(255, 255, 255)
#define ARB_BLACK leds.Color(0, 0, 0)

String listenerDeviceName = "tally-arbiter";
char tallyarbiter_host[40] = "192.168.1.2";
char tallyarbiter_port[6]  = "4455";

WiFiManager wm;
SocketIOclient socket;
WiFiEventHandler gotIpHandler, disconnectHandler;

JSONVar BusOptions;
JSONVar Devices;
JSONVar DeviceStates;
String DeviceId = "unassigned";
String DeviceName = "Unassigned";
String prevType = "";
String actualType = "";
String actualColor = "";
int actualPriority = 0;
bool networkConnected = false;

void eepromWriteString(int addr, String data, int maxLen) {
  int len = min((int)data.length(), maxLen - 1);
  for (int i = 0; i < len; i++) EEPROM.write(addr + i, data[i]);
  EEPROM.write(addr + len, 0);
  EEPROM.commit();
}

String eepromReadString(int addr, int maxLen) {
  char buf[maxLen];
  int i = 0;
  for (; i < maxLen - 1; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0 || c == 255) break;
    buf[i] = c;
  }
  buf[i] = 0;
  return String(buf);
}

void setColor(uint32_t color) {
  for (int i = 0; i < leds.numPixels(); i++) {
    leds.setPixelColor(i, color);
  }
  leds.show();
}

String stripQuot(String str) {
  if (str[0] == '"') str.remove(0, 1);
  if (str.endsWith("\"")) str.remove(str.length() - 1, 1);
  return str;
}

void wsEmit(String event, const char* payload = NULL) {
  String msg = payload ? ("[\"" + event + "\"," + payload + "]") : ("[\"" + event + "\"]");
  socket.sendEVENT(msg);
}

String getBusTypeById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++) {
    if (JSON.stringify(BusOptions[i]["id"]) == busId) return JSON.stringify(BusOptions[i]["type"]);
  }
  return "invalid";
}

String getBusColorById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++) {
    if (JSON.stringify(BusOptions[i]["id"]) == busId) return JSON.stringify(BusOptions[i]["color"]);
  }
  return "invalid";
}

int getBusPriorityById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++) {
    if (JSON.stringify(BusOptions[i]["id"]) == busId) return JSON.stringify(BusOptions[i]["priority"]).toInt();
  }
  return 0;
}

void evaluateMode() {
  if (actualType == prevType) return;
  actualColor.replace("#", "");
  long number = actualType != "" ? strtol(actualColor.c_str(), NULL, 16) : 0;
  int r = (number >> 16) & 0xFF;
  int g = (number >> 8) & 0xFF;
  int b = number & 0xFF;
  setColor(actualType != "" ? leds.Color(r, g, b) : ARB_BLACK);
  prevType = actualType;
}

void setDeviceName() {
  for (int i = 0; i < Devices.length(); i++) {
    if (JSON.stringify(Devices[i]["id"]) == "\"" + DeviceId + "\"") {
      String strDevice = JSON.stringify(Devices[i]["name"]);
      DeviceName = strDevice.substring(1, strDevice.length() - 1);
      break;
    }
  }
  eepromWriteString(ADDR_DEVICENAME, DeviceName, 40);
  evaluateMode();
}

void processTallyData() {
  bool typeChanged = false;
  for (int i = 0; i < DeviceStates.length(); i++) {
    if (DeviceStates[i]["sources"].length() > 0) {
      typeChanged = true;
      actualType = getBusTypeById(JSON.stringify(DeviceStates[i]["busId"]));
      actualColor = getBusColorById(JSON.stringify(DeviceStates[i]["busId"]));
      actualPriority = getBusPriorityById(JSON.stringify(DeviceStates[i]["busId"]));
    }
  }
  if (!typeChanged) {
    actualType = "";
    actualColor = "";
    actualPriority = 0;
  }
  evaluateMode();
}

void socketFlash() {
  leds.setBrightness(255);
  for (int i = 0; i < 3; i++) {
    setColor(ARB_WHITE);
    delay(300);
    setColor(ARB_BLACK);
    delay(300);
  }
  leds.setBrightness(BRIGHTNESS);
  evaluateMode();
}

void socketReassign(String payload) {
  String oldDeviceId = payload.substring(0, payload.indexOf(','));
  String newDeviceId = payload.substring(oldDeviceId.length() + 1);
  newDeviceId = newDeviceId.substring(0, newDeviceId.indexOf(','));
  oldDeviceId = stripQuot(oldDeviceId);
  newDeviceId = stripQuot(newDeviceId);

  String reassignObj = "{\"oldDeviceId\": \"" + oldDeviceId + "\", \"newDeviceId\": \"" + newDeviceId + "\"}";
  wsEmit("listener_reassign_object", reassignObj.c_str());
  wsEmit("devices");

  setColor(ARB_WHITE);
  delay(200);
  setColor(ARB_BLACK);

  DeviceId = newDeviceId;
  eepromWriteString(ADDR_DEVICEID, DeviceId, 40);
  setDeviceName();
}

void socketConnected() {
  String deviceObj = "{\"deviceId\": \"" + DeviceId + "\", \"listenerType\": \"" + listenerDeviceName + "\", \"canBeReassigned\": true, \"canBeFlashed\": true, \"supportsChat\": false }";
  wsEmit("listenerclient_connect", deviceObj.c_str());
}

void socketEvent(socketIOmessageType_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case sIOtype_CONNECT:
      socketConnected();
      break;
    case sIOtype_EVENT: {
      String msg = (char*)payload;
      String evt = msg.substring(2, msg.indexOf("\"", 2));
      String content = msg.substring(evt.length() + 4);
      content.remove(content.length() - 1);

      if (evt == "bus_options") BusOptions = JSON.parse(content);
      if (evt == "reassign") socketReassign(content);
      if (evt == "flash") socketFlash();
      if (evt == "deviceId") {
        DeviceId = content.substring(1, content.length() - 1);
        setDeviceName();
      }
      if (evt == "devices") {
        Devices = JSON.parse(content);
        setDeviceName();
      }
      if (evt == "device_states") {
        DeviceStates = JSON.parse(content);
        processTallyData();
      }
      break;
    }
    default:
      break;
  }
}

void saveParamCallback() {
  String str_taHost = wm.server->hasArg("taHostIP") ? wm.server->arg("taHostIP") : "";
  String str_taPort = wm.server->hasArg("taHostPort") ? wm.server->arg("taHostPort") : "";
  eepromWriteString(ADDR_TAHOST, str_taHost, 40);
  eepromWriteString(ADDR_TAPORT, str_taPort, 6);
}

void connectToNetwork() {
  WiFi.mode(WIFI_STA);

  gotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP&) {
    networkConnected = true;
  });
  disconnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected&) {
    networkConnected = false;
  });

  WiFiManagerParameter custom_taServer("taHostIP", "Tally Arbiter Server", tallyarbiter_host, 40);
  WiFiManagerParameter custom_taPort("taHostPort", "Port", tallyarbiter_port, 6);
  wm.addParameter(&custom_taServer);
  wm.addParameter(&custom_taPort);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setConfigPortalTimeout(120);

  networkConnected = wm.autoConnect(listenerDeviceName.c_str());
}

void connectToServer() {
  socket.onEvent(socketEvent);
  socket.begin(tallyarbiter_host, atoi(tallyarbiter_port));
}

void setup() {
  leds.begin();
  leds.setBrightness(BRIGHTNESS);
  setColor(leds.Color(0, 0, 255));

  EEPROM.begin(EEPROM_SIZE);

  listenerDeviceName = "tally-" + String(ESP.getChipId(), HEX);

  String storedId = eepromReadString(ADDR_DEVICEID, 40);
  String storedName = eepromReadString(ADDR_DEVICENAME, 40);
  String storedHost = eepromReadString(ADDR_TAHOST, 40);
  String storedPort = eepromReadString(ADDR_TAPORT, 6);
  if (storedId.length() > 0) DeviceId = storedId;
  if (storedName.length() > 0) DeviceName = storedName;
  if (storedHost.length() > 0) storedHost.toCharArray(tallyarbiter_host, 40);
  if (storedPort.length() > 0) storedPort.toCharArray(tallyarbiter_port, 6);

  connectToNetwork();
  while (!networkConnected) {
    setColor(leds.Color(255, 0, 0));
    delay(300);
    setColor(ARB_BLACK);
    delay(300);
  }

  ArduinoOTA.setHostname(listenerDeviceName.c_str());
  ArduinoOTA.begin();

  setColor(leds.Color(0, 255, 0));
  delay(500);

  connectToServer();
}

void loop() {
  ArduinoOTA.handle();
  socket.loop();
}
#endif