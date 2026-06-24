#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "";
const char* password = "";

typedef struct __attribute__((packed)) struct_message {
    uint8_t pgm_mask;
    uint8_t pvw_mask;
} struct_message;

struct_message Cam;

// ====================================================================
// --- TX CONFIGURATION ESP32 (DUAL OUTPUT: ESP-NOW + nRF24L01) ---
// ====================================================================

#ifdef MODE_TX_ESP32
  #include <esp_now.h>
  #include <WiFi.h>
  #include <SPI.h>
  #include <nRF24L01.h>
  #include <RF24.h>

  RF24 radio(4, 5); // CE, CSN
  const byte nrfAddress[6] = "TALLY";

  uint8_t rx1[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D};

  const uint8_t PGM_PINS[4] = {12, 13, 25, 26}; // Kamera 1-4 PGM
  const uint8_t PVW_PINS[4] = {27, 32, 33, 34}; // Kamera 1-4 PVW

  const uint8_t LED_MODE = 2; 
  
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

  void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", mac_addr[i]);
      if (i < 5) Serial.println(";");
    }
    if (status == ESP_NOW_SEND_SUCCESS) {
      Serial.println("0");
    } else {
      Serial.println("1");
    }
  }

  void setup() {
    Serial.begin(115200);
    
    // --- Inisialisasi ESP-NOW ---
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK) {
      esp_now_register_send_cb(OnDataSent);
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, rx1, 6);
      peerInfo.channel = 1;  
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
      
    }

    // --- Inisialisasi nRF24L01 ---
    if (radio.begin()) {
      radio.openWritingPipe(nrfAddress);
      radio.setPALevel(RF24_PA_MAX);
      radio.stopListening();
    }

    // Inisialisasi Pin Input Switcher Fisik
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
    vmixActive = (millis() - lastSerialRx) <= SERIAL_TIMEOUT;
    digitalWrite(LED_MODE, vmixActive ? HIGH : LOW);

    // Baca vMix Serial
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');
      data.trim();
      int commaIndex = data.indexOf(',');

      if (commaIndex > 0) {
        Cam.pgm_mask = (uint8_t)data.substring(0, commaIndex).toInt();
        Cam.pvw_mask = (uint8_t)data.substring(commaIndex + 1).toInt();
        lastSerialRx = millis(); 
      }
    }

    // Baca Fisik DB15
    if (!vmixActive) {
      static unsigned long lastPhysicalRead = 0;
      if (millis() - lastPhysicalRead >= DEBOUNCE_MS) {
        lastPhysicalRead = millis();
        Cam.pgm_mask = readPhysicalMask(PGM_PINS);
        Cam.pvw_mask = readPhysicalMask(PVW_PINS);
      }
    }

    if (millis() - lastSend >= SEND_INTERVAL) {
      lastSend = millis();
      
      // 1. ESP-NOW
      uint8_t payload[2];
      payload[0] = Cam.pgm_mask;
      payload[1] = Cam.pvw_mask;
      esp_now_send(rx1, payload, 2);

      // 2. nRF24L01
      radio.write(&Cam, sizeof(Cam)); 
    }
  }
#endif

// ====================================================================
// --- TX CONFIGURATION ESP32 (DUAL OUTPUT: ESP-NOW + nRF24L01) ---
// ====================================================================

#ifdef MODE_TX_ESPUDP
#include <WiFiUdp.h>

IPAddress TIP[] = {
  IPAddress(192, 168, 1, 101), 
  IPAddress(192, 168, 1, 102)
};

WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  udp.begin(8888);
}

void loop () {
  Cam.pgm_mask = ;

  
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP32 UDP ---
// ====================================================================

#ifdef MODE_RX_ESPUDP
#include <WiFiUdp.h>

IPAddress local_ip ();
IPAddress gateway ();
IPAddress subnet ();

WiFiUDP udp;

void setup () {
  Serial.begin(115200);

  WiFi.config(local_ip, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  udp.begin(8888)
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

  volatile uint8_t pgm_mask = 0;
  volatile uint8_t pvw_mask = 0;

  // Callback penerima ESP-NOW versi ESP8266 (Tipe argumen berbeda dengan ESP32)
  void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
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

    struct station_config conf;
    wifi_station_get_config(&conf);
    conf.bssid_set = 0;
    wifi_station_get_config(&conf);
    
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
    } 
    else if (pvw_mask == 1) {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, HIGH);
      digitalWrite(BLUE, LOW);
    } 
    else {
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