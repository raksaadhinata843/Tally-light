#include <Arduino.h>

// Deklarasi Tipe Data
typedef struct __attribute__((packed)) {
    uint8_t pgm_mask;
    uint8_t pvw_mask;
} TallyPacket;

// ====================================================================
// --- TX CONFIGURATION ESP32(SwITCHER)---
// ====================================================================
#ifdef MODE_TX_ESP32(SWITCHER)
#include <WiFi.h>
#include <esp_now.h>

const uint8_t PGM_PINS[4] = {12, 13, 25, 26}; 
const uint8_t PVW_PINS[4] = {27, 32, 33, 34};
const int MODE_SWITCH_PIN = 14;

uint8_t broadcastAddress[] = {0x24,0xD7,0xEB,0xCD,0x27,0x3D};
bool modePC = true;
bool lastButtonState = HIGH;

TallyPacket txPacket;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  for(int i = 0; i < 4; i++) {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
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
  esp_now_send(broadcastAddress, (uint8_t*)&txPacket, sizeof(txPacket));
  delay(50);
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266UDP---
// ====================================================================
#ifdef MODE_RX_ESP8266UDP 
#include <ESP8266WiFi.h>
#include <espnow.h>

#define RED 5    // D1
#define GREEN 4  // D2
#define BLUE 14  // D5

// Tentukan ID Tally ini (Misal ID 1, ID 2, dst)
const uint8_t CAM_ID = 1; 

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

TallyPacket rxPacket;

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
// --- TX CONFIGURATION ESP32(VMIX) (DUAL OUTPUT: ESP-NOW + nRF24L01) ---
// ====================================================================

#ifdef MODE_TX_ESP32(VMIX)
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_now.h>

// --- KONFIGURASI ---
const char* ssid = "TP-Link_9BFE"; 
const char* password = "05674411";
const char* vmixApi = "http://192.168.1.100:58088/api/"; // IP vMix kamu

uint8_t broadcastAddress[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D}; // RX1

TallyPacket txPacket;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  if (esp_now_init() != ESP_OK) return;
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(vmixApi);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      
      // Cari posisi <active>1</active>
      // Kita ambil angka di antara tag tersebut
      int startActive = payload.indexOf("<active>") + 8;
      int endActive = payload.indexOf("</active>");
      String activeStr = payload.substring(startActive, endActive);
      int active = activeStr.toInt();
      
      // Cari posisi <preview>2</preview>
      int startPreview = payload.indexOf("<preview>") + 9;
      int endPreview = payload.indexOf("</preview>");
      String previewStr = payload.substring(startPreview, endPreview);
      int preview = previewStr.toInt();
      
      // LOGIC MASKING (Sama seperti sebelumnya)
      txPacket.pgm_mask = (active > 0) ? (1 << (active - 1)) : 0;
      txPacket.pvw_mask = (preview > 0) ? (1 << (preview - 1)) : 0;
      
      // KIRIM KE ESP-NOW
      esp_now_send(broadcastAddress, (uint8_t*)&txPacket, sizeof(txPacket));
      }
    http.end();
  }
  delay(100); // Latensi vMix API biasanya di kisaran 100ms, ini sudah sangat cukup
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266 ---
// ====================================================================

#ifdef MODE_RX_ESP8266 
  #include <ESP8266WiFi.h>
  #include <espnow.h>

  // Pin Output LED Tally pada ESP8266 (Ganti nomor pin jika diinginkan)
  #define RED 5    // GPIO 5 (D1)
  #define GREEN 4  // GPIO 4 (D2)
  #define BLUE 14  // GPIO 14 (D5)

  volatile uint8_t pgm_mask = 0;
  volatile uint8_t pvw_mask = 0;

  // Callback penerima ESP-NOW versi ESP8266 (Tipe argumen berbeda dengan ESP32)
  void OnDataRecv(uint8_t *mac_addr, uint8_t *incomingData, uint8_t len) {
    if (len >= 2) {
      pgm_mask = incomingData[0];
      pvw_mask = incomingData[1];
    }
  }

  void setup() {
    Serial.begin(115200);
    pinMode(RED, OUTPUT); 
    pinMode(BLUE, OUTPUT); 
    pinMode(GREEN, OUTPUT);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // Membersihkan koneksi Wi-Fi bawaan
    wifi_set_channel(1); // Pastikan channel sama dengan TX
    // Inisialisasi ESP-NOW khusus ESP8266
    if (esp_now_init() != 0) return;
    
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
  }

void loop() {

  if (pgm_mask == 1) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  } else if (pvw_mask == 1) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, LOW);
  } else {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, HIGH);
  }
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