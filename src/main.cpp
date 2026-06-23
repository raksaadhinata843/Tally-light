#include <Arduino.h>

typedef struct __attribute__((packed)) struct_message {
    uint8_t pgm_mask;
    uint8_t pvw_mask;
} struct_message;

struct_message myData;

// ====================================================================
// --- TX CONFIGURATION ESP32 ---
// ====================================================================

#ifdef MODE_TX_ESP32
  #include <esp_now.h>
  #include <WiFi.h>

  // Alamat MAC Address ESP8266 Receiver Anda (Ganti sesuai MAC Receiver Anda)
  uint8_t rxAddress[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D};

  const uint8_t PGM_PINS[4] = {12, 13, 25, 26}; // Kamera 1-4 PGM
  const uint8_t PVW_PINS[4] = {27, 32, 33, 34}; // Kamera 1-4 PVW

  const uint8_t LED_MODE = 2; // LED internal ESP32 untuk indikator vMix Active
  
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

  void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

  void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) return;
    esp_now_register_send_cb(OnDataSent);

    // Daftarkan Peer Receiver
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, rxAddress, 6);
    peerInfo.channel = 1;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // Inisialisasi Pin Input
    for (uint8_t i = 0; i < 4; i++) {
      pinMode(PGM_PINS[i], INPUT_PULLUP);
      if(PVW_PINS[i] == 34) {
        pinMode(PVW_PINS[i], INPUT);
      } else {
        pinMode(PVW_PINS[i], INPUT_PULLUP);
      }
    }
    pinMode(LED_MODE, OUTPUT);
  }

  void loop() {
    // 1. Deteksi Mode Otomatis
    vmixActive = (millis() - lastSerialRx) <= SERIAL_TIMEOUT;
    digitalWrite(LED_MODE, vmixActive ? HIGH : LOW);

    // 2. Baca Data Serial vMix ("PGM_MASK,PVW_MASK\n")
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');
      int commaIndex = data.indexOf(',');
      if (commaIndex > 0) {
        myData.pgm_mask = data.substring(0, commaIndex).toInt();
        myData.pvw_mask = data.substring(commaIndex + 1).toInt();
        lastSerialRx = millis(); 
      }
    }

    // 3. Baca Data Fisik DB15 jika vMix tidak aktif
    if (!vmixActive) {
      static unsigned long lastPhysicalRead = 0;
      if (millis() - lastPhysicalRead >= DEBOUNCE_MS) {
        lastPhysicalRead = millis();
        myData.pgm_mask = readPhysicalMask(PGM_PINS);
        myData.pvw_mask = readPhysicalMask(PVW_PINS);
      }
    }

    // 4. Kirim Data Secara Berkala
    if (millis() - lastSend >= SEND_INTERVAL) {
      lastSend = millis();
      esp_now_send(rxAddress, (uint8_t *) &myData, sizeof(myData));
    }
  }
#endif

// ====================================================================
// --- RX CONFIGURATION ESP8266 ---
// ====================================================================

#ifdef MODE_RX_ESP8266 
  #include <ESP8266WiFi.h>
  #include <espnow.h>

  // Isikan nomor kamera untuk board penerima ini (Misal Lampu Kamera 1)
  const uint8_t CAM_ID = 1; 

  // Pin Output LED Tally pada ESP8266 (Ganti nomor pin jika diinginkan)
  #define RED 5    // GPIO 5 (D1)
  #define GREEN 4  // GPIO 4 (D2)
  #define BLUE 14  // GPIO 14 (D5)

  // Callback penerima ESP-NOW versi ESP8266 (Tipe argumen berbeda dengan ESP32)
  void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
    if (len >= sizeof(myData)) {
      memcpy(&myData, incomingData, sizeof(myData));
    }
  }

  void setup() {
    Serial.begin(115200);
    pinMode(RED, OUTPUT); 
    pinMode(BLUE, OUTPUT); 
    pinMode(GREEN, OUTPUT);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // Membersihkan koneksi Wi-Fi bawaan
    
    // Inisialisasi ESP-NOW khusus ESP8266
    if (esp_now_init() != 0) return;
    
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
  }

  void loop() {
    // Dekode status kamera dari data bitmask mask yang diterima
    bool isPgm = (myData.pgm_mask & (1 << (CAM_ID - 1))) != 0;
    bool isPvw = (myData.pvw_mask & (1 << (CAM_ID - 1))) != 0;

    // Kendali Lampu Tally menggunakan digitalWrite standard (Lebih stabil untuk On/Off)
    if (isPgm) {
      digitalWrite(RED, HIGH);    // Merah Menyala jika LIVE (Program)
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, LOW);
    } 
    else if (isPvw) {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, HIGH);  // Hijau Menyala jika Preview / Standby
      digitalWrite(BLUE, LOW);
    } 
    else {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, HIGH);   // Biru Menyala jika posisi Idle / Kamera tidak aktif
    }
    delay(50);
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