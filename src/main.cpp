#include <Arduino.h>

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
const char* ssid = "Rec.709"; 
const char* password = "malammalam";
WiFiUDP udp;

const uint8_t PGM_PINS[4] = {12, 13, 25, 26}; 
const uint8_t PVW_PINS[4] = {27, 32, 33, 34};

TallyPacket txPacket;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); Serial.print("."); }

  udp.begin(4210);

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
  udp.beginPacket("239.1.2.3", 4210);
  udp.write((uint8_t*)&txPacket, sizeof(txPacket));
  udp.endPacket();
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
  udp.beginPacket("192.168.0.255", udpPort);
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

const char* ssid = "Rec.709";
const char* password = "malammalam";
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
#include <FastLED.h>

#define PIN        5
#define NUMPIXELS  1

// Tentukan ID Tally ini (0, 1, 2, 3)
const uint8_t CAM_ID = 0; 

TallyPacket rxPacket;

const char* ssid = "Rec.709";
const char* password = "malammalam";
WiFiUDP udp;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

CRGB leds[NUMPIXELS];

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812, PIN, GRB>(leds, NUMPIXELS);
  FastLED.setBrightness(50);
  leds[0] = CRGB::Black;
  FastLED.show();
    
  WiFi.begin(ssid, password);

  if (WiFi.status() == WL_CONNECTED) {
    leds[0] = CRGB::Blue;
  }
    
  udp.beginMulticast(IPAddress(239, 1, 2, 3), 4210);
}

void recon() {
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED) {
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);
    leds[0] = CRGB::Black;
    FastLED.show();
  }
  udp.beginMulticast(IPAddress(239, 1, 2, 3), 4210);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    int packetSize = udp.parsePacket();
    if (packetSize == sizeof(TallyPacket)) {
        TallyPacket rxPacket;
        udp.read((unsigned char*)&rxPacket, sizeof(TallyPacket));

        // LOGIKA BITWISE YANG BENAR:
        // Gunakan operator '&' untuk men-masking bit spesifik
        bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID)));
        bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID)));

        leds[0] = CRGB::Black;

        if (isPgm) {
            leds[0] = CRGB::Red;
            Serial.print("RED");
        } else if (isPvw) {
          Serial.print("GREEN");
            leds[0] = CRGB::Green;
        } else {
          Serial.print("BLUE");
            leds[0] = CRGB::Blue;
        }

        FastLED.show();
    }
  } else {
    recon();
  }
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266WS (UDP)---
// ====================================================================
#ifdef MODE_RX_ESP8266UDP_WS
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>

#define PIN        4 //D2
#define NUMPIXELS  1

// Tentukan ID Tally ini (0, 1, 2, 3)
const uint8_t CAM_ID = 1; 

TallyPacket rxPacket;

const char* ssid = "Tally_Light";
const char* password = "malammalam";
WiFiUDP udp;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

CRGB leds[NUMPIXELS];

void setup() {
  Serial.begin(115200);
  
  FastLED.addLeds<WS2812, PIN, GRB>(leds, NUMPIXELS);
  FastLED.setBrightness(50);
  leds[0] = CRGB::Black;
  FastLED.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  udp.beginMulticast(WiFi.localIP(), IPAddress(239, 1, 2, 3), 4210);
}

void recon() {
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED) {
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);
    leds[0] = CRGB::Black;
    FastLED.show();
  }
  udp.beginMulticast(WiFi.localIP(), IPAddress(239, 1, 2, 3), 4210);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    int packetSize = udp.parsePacket();
    if (packetSize == sizeof(TallyPacket)) {
        TallyPacket rxPacket;
        udp.read((unsigned char*)&rxPacket, sizeof(TallyPacket));

        // LOGIKA BITWISE YANG BENAR:
        // Gunakan operator '&' untuk men-masking bit spesifik
        bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID)));
        bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID)));

        leds[0] = CRGB::Black;

        if (isPgm) {
            leds[0] = CRGB::Red;
            Serial.print("RED");
        } else if (isPvw) {
          Serial.print("GREEN");
            leds[0] = CRGB::Green;
        } else {
          Serial.print("BLUE");
            leds[0] = CRGB::Blue;
        }

        FastLED.show();
    }
  } else {
    recon();
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