#include <Arduino.h>
#include <ArduinoOTA.h>

// Deklarasi Tipe Data
typedef struct __attribute__((packed))
{
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
    {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D},
    // {0x20,0x50,0x0d,0xd2,0x38,0x8c},
    {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x5D},
    {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x6D}};
bool modePC = true;
bool lastButtonState = HIGH;

TallyPacket txPacket;

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
    return;

  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  for (int i = 0; i < 4; i++)
  {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
}

void checkModeButton()
{
  bool currentButtonState = digitalRead(MODE_SWITCH_PIN);

  if (currentButtonState == LOW && lastButtonState == HIGH)
  {
    modePC = !modePC;
    Serial.println("Switch");
    delay(200);
  }
  lastButtonState = currentButtonState;
}

void updateManualData()
{
  txPacket.pgm_mask = 0;
  txPacket.pvw_mask = 0;
  for (int i = 0; i < 4; i++)
  {
    if (digitalRead(PGM_PINS[i]) == LOW)
      txPacket.pgm_mask |= (1 << i);
    if (digitalRead(PVW_PINS[i]) == LOW)
      txPacket.pvw_mask |= (1 << i);
  }
}

void loop()
{
  checkModeButton();

  if (modePC)
  {
    if (Serial.available() >= 2)
    {
      txPacket.pgm_mask = Serial.read();
      txPacket.pvw_mask = Serial.read();
    }
  }
  else
  {
    updateManualData();
  }
  esp_now_send(NULL, (uint8_t *)&txPacket, sizeof(txPacket));
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
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
WiFiUDP udp;

const uint8_t PGM_PINS[4] = {12, 13, 25, 26};
const uint8_t PVW_PINS[4] = {27, 32, 33, 34};

TallyPacket txPacket;

void setup()
{
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("Tally-ESP32");
  ArduinoOTA.begin();

  udp.begin(4210);
  for (int i = 0; i < 4; i++)
  {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
  }
}

void loop()
{
  ArduinoOTA.handle();
  txPacket.pgm_mask = 0;
  txPacket.pvw_mask = 0;
  for (int i = 0; i < 4; i++)
  {
    if (digitalRead(PGM_PINS[i]) == LOW)
      txPacket.pgm_mask |= (1 << i);
    if (digitalRead(PVW_PINS[i]) == LOW)
      txPacket.pvw_mask |= (1 << i);
  }

  udp.beginPacket("239.1.2.3", 4210);
  udp.write((uint8_t *)&txPacket, sizeof(txPacket));
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

const char *ssid = "Rec.709";
const char *password = "malammalam";
const int udpPort = 8888;
WiFiUDP udp;

const uint8_t PGM_PINS[4] = {D1, D2, D3, D5};
const uint8_t PVW_PINS[4] = {D6, D7, D8, RX};

TallyPacket txPacket;

void setup()
{
  Serial.begin(115200);

  // Konfigurasi IP Statis (Opsional tapi disarankan)
  IPAddress local_IP(192, 168, 0, 5);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  udp.begin(udpPort);

  for (int i = 0; i < 4; i++)
  {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
  }
}

void loop()
{
  txPacket.pgm_mask = 0;
  txPacket.pvw_mask = 0;
  for (int i = 0; i < 4; i++)
  {
    if (digitalRead(PGM_PINS[i]) == LOW)
      txPacket.pgm_mask |= (1 << i);
    if (digitalRead(PVW_PINS[i]) == LOW)
      txPacket.pvw_mask |= (1 << i);
  }
  udp.beginPacket(IPAddress(192, 168, 0, 255), udpPort);
  udp.write((uint8_t *)&txPacket, sizeof(txPacket));
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

#define RED 5   // D1
#define GREEN 4 // D2
#define BLUE 14 // D5

// Tentukan ID Tally ini (Misal ID 1, ID 2, dst)
const uint8_t CAM_ID = 1;

TallyPacket rxPacket;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  if (len == sizeof(TallyPacket))
  {
    memcpy(&rxPacket, data, sizeof(TallyPacket));
  }
}

void setup()
{
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0)
    return;

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  // Gunakan Bitwise untuk cek apakah ID ini ada di dalam mask
  // (1 << (CAM_ID - 1)) akan menghasilkan bit yang sesuai untuk ID tersebut
  bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID - 1)));
  bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID - 1)));

  if (isPgm)
  {
    // Status PGM (Merah)
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
  else if (isPvw)
  {
    // Status PVW (Hijau)
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, LOW);
  }
  else
  {
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
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

#define PIN 5
#define NUMPIXELS 1

TallyPacket rxPacket;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *vmixIP = "192.168.0.100";
const int vmixPort = 8099;
WiFiClient client;

Adafruit_NeoPixel leds(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void recon()
{
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED)
  {
    leds.setPixelColor(0, leds.Color(255, 0, 0));
    leds.show();
    delay(500);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(500);
  }
}

void setup()
{
  leds.begin();
  leds.setBrightness(50);
  leds.setPixelColor(0, leds.Color(0, 0, 0));
  leds.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
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
  for (uint8_t i = 0; i <= CAM_ID; i++)
  {
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
}

void loop()
{
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED)
  {
    recon();
    return;
  }

  if (client.available())
  {
    String data = client.readStringUntil('\n');
    data.trim(); // Hapus spasi dan karakter newline

    if (data.startsWith("TALLY OK"))
    {
      if (data.length() > 8)
      {
        char state = data.charAt(8);

        if (state == '1')
        {
          leds.setPixelColor(0, leds.Color(255, 0, 0));
        }
        else if (state == '2')
        {
          leds.setPixelColor(0, leds.Color(0, 255, 0));
        }
        else
        {
          leds.setPixelColor(0, leds.Color(0, 0, 255));
        }
      }
    }

    leds.show();
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

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const int udpPort = 55755;
WiFiUDP udp;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  WiFi.begin(ssid, password);

  udp.begin(udpPort);
}

void loop()
{
  int packetSize = udp.parsePacket();
  if (packetSize == sizeof(TallyPacket))
  {
    TallyPacket rxPacket;
    udp.read((unsigned char *)&rxPacket, sizeof(TallyPacket));

    // LOGIKA BITWISE YANG BENAR:
    // Gunakan operator '&' untuk men-masking bit spesifik
    bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID - 1)));
    bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID - 1)));

    if (isPgm)
    {
      Serial.println("Red");
      digitalWrite(RED, HIGH);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, LOW);
    }
    else if (isPvw)
    {
      Serial.println("Green");
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, HIGH);
      digitalWrite(BLUE, LOW);
    }
    else
    {
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

#define PIN 5
#define NUMPIXELS 1

TallyPacket rxPacket;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
WiFiUDP udp;

Adafruit_NeoPixel leds(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void recon()
{
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED)
  {
    leds.setPixelColor(0, leds.Color(255, 0, 0));
    leds.show();
    delay(500);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(500);
  }
  udp.beginMulticast(IPAddress(239, 1, 2, 3), 4210);
}

void setup()
{
  leds.begin();
  leds.setBrightness(50);
  leds.setPixelColor(0, leds.Color(0, 0, 0));
  leds.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
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
  for (uint8_t i = 0; i <= CAM_ID; i++)
  {
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

void loop()
{
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED)
  {
    recon();
    return;
  }

  int packetSize = udp.parsePacket();
  if (packetSize == sizeof(TallyPacket))
  {
    TallyPacket rxPacket;
    udp.read((unsigned char *)&rxPacket, sizeof(TallyPacket));

    bool isPgm = (rxPacket.pgm_mask & (1 << CAM_ID));
    bool isPvw = (rxPacket.pvw_mask & (1 << CAM_ID));

    if (isPgm)
    {
      leds.setPixelColor(0, leds.Color(255, 0, 0));
    }
    else if (isPvw)
    {
      leds.setPixelColor(0, leds.Color(0, 255, 0));
    }
    else
    {
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

#define PIN 4 // D2
#define NUMPIXELS 1

TallyPacket rxPacket;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
WiFiUDP udp;

Adafruit_NeoPixel leds(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void recon()
{
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED)
  {
    leds.setPixelColor(0, leds.Color(255, 0, 0));
    leds.show();
    delay(500);
    leds.setPixelColor(0, leds.Color(0, 0, 0));
    leds.show();
    delay(500);
  }
  udp.beginMulticast(WiFi.localIP(), IPAddress(239, 1, 2, 3), 4210);
}

void setup()
{
  leds.begin();
  leds.setBrightness(50);
  leds.setPixelColor(0, leds.Color(0, 0, 0));
  leds.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
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
  for (uint8_t i = 0; i <= CAM_ID; i++)
  {
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

void loop()
{
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED)
  {
    recon();
    return;
  }

  int packetSize = udp.parsePacket();
  if (packetSize == sizeof(TallyPacket))
  {
    TallyPacket rxPacket;
    udp.read((unsigned char *)&rxPacket, sizeof(TallyPacket));

    bool isPgm = (rxPacket.pgm_mask & (1 << CAM_ID));
    bool isPvw = (rxPacket.pvw_mask & (1 << CAM_ID));

    if (isPgm)
    {
      leds.setPixelColor(0, leds.Color(255, 0, 0));
    }
    else if (isPvw)
    {
      leds.setPixelColor(0, leds.Color(0, 255, 0));
    }
    else
    {
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

#define PIN 5
#define NUMPIXELS 1

// Tentukan ID Tally ini (Misal ID 1, ID 2, dst)
const uint8_t CAM_ID = 1;

volatile uint8_t pgm_mask = 0;
volatile uint8_t pvw_mask = 0;

CRGB leds[NUMPIXELS];

TallyPacket rxPacket;

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  if (len == sizeof(TallyPacket))
  {
    memcpy(&rxPacket, data, sizeof(TallyPacket));
  }
}

void setup()
{
  FastLED.addLeds<WS2812, PIN, GRB>(leds, NUMPIXELS);
  FastLED.setBrightness(50);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0)
    return;

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  // Gunakan Bitwise untuk cek apakah ID ini ada di dalam mask
  // (1 << (CAM_ID - 1)) akan menghasilkan bit yang sesuai untuk ID tersebut
  bool isPgm = (rxPacket.pgm_mask & (1 << (CAM_ID - 1)));
  bool isPvw = (rxPacket.pvw_mask & (1 << (CAM_ID - 1)));

  pixels.clear();

  if (isPgm)
  {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  }
  else if (isPvw)
  {
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
  }
  else
  {
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
struct Packet
{
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

uint8_t readPhysicalMask(const uint8_t pins[4])
{
  uint8_t m = 0;
  for (uint8_t i = 0; i < 4; i++)
  {
    if (digitalRead(pins[i]) == LOW)
    {
      m |= (1 << i);
    }
  }
  return m;
}

void setup()
{
  Serial.begin(115200); // Digunakan untuk menangkap data serial vMix komputer

  // Inisialisasi Modul Wireless NRF24L01
  if (!radio.begin())
  {
    while (1)
      ; // Kunci di sini jika modul NRF rusak/kabelnya salah pasang
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MAX); // Jangkauan sinyal maksimum
  radio.stopListening();         // Set sebagai Transmitter (TX)

  // Inisialisasi Seluruh Pin Input (Menggunakan Pull-up internal 5V yang kuat)
  for (uint8_t i = 0; i < 4; i++)
  {
    pinMode(PGM_PINS[i], INPUT_PULLUP);
    pinMode(PVW_PINS[i], INPUT_PULLUP);
  }
  pinMode(LED_MODE, OUTPUT);
}

void loop()
{
  // 1. Deteksi Mode Otomatis (vMix Serial vs Fisik DB15)
  vmixActive = (millis() - lastSerialRx) <= SERIAL_TIMEOUT;
  digitalWrite(LED_MODE, vmixActive ? HIGH : LOW);

  // 2. Baca Data Serial vMix ("PGM_MASK,PVW_MASK\n")
  if (Serial.available() > 0)
  {
    String data = Serial.readStringUntil('\n');
    int commaIndex = data.indexOf(',');
    if (commaIndex > 0)
    {
      pkt.pgm_mask = data.substring(0, commaIndex).toInt();
      pkt.pvw_mask = data.substring(commaIndex + 1).toInt();
      lastSerialRx = millis();
    }
  }

  // 3. Baca Data Fisik DB15 jika vMix tidak aktif
  if (!vmixActive)
  {
    static unsigned long lastPhysicalRead = 0;
    if (millis() - lastPhysicalRead >= DEBOUNCE_MS)
    {
      lastPhysicalRead = millis();
      pkt.pgm_mask = readPhysicalMask(PGM_PINS);
      pkt.pvw_mask = readPhysicalMask(PVW_PINS);
    }
  }

  // 4. Kirim Paket via NRF24L01 Secara Berkala
  if (millis() - lastSend >= SEND_INTERVAL)
  {
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
struct Packet
{
  uint8_t pgm_mask;
  uint8_t pvw_mask;
};

Packet receivedPkt;

RF24 radio(7, 8);                // Pin CE, CSN
const byte address[6] = "TALLY"; // Alamat pipa harus sama dengan Transmitter

void setup()
{
  Serial.begin(115200);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  // Tes lampu saat dinyalakan (opsional, untuk memastikan LED berfungsi)
  digitalWrite(BLUE, HIGH);
  digitalWrite(BLUE, LOW);

  // Inisialisasi Modul Wireless nRF24L01
  if radio
    .begin() = !;

  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_MAX); // Jangkauan maksimum
  radio.startListening();
}

void loop()
{
  // Cek apakah ada data masuk melalui udara
  if (radio.available())
  {
    // Baca data paket bitmask
    radio.read(&receivedPkt, sizeof(receivedPkt));

    // Dekode status kamera berdasarkan CAM_ID yang ditentukan di atas
    bool isPgm = (receivedPkt.pgm_mask & (1 << (CAM_ID - 1))) != 0;
    bool isPvw = (receivedPkt.pvw_mask & (1 << (CAM_ID - 1))) != 0;

    // Kendali Lampu Tally
    if (isPgm)
    {
      digitalWrite(RED, HIGH); // Merah jika LIVE (Program)
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, LOW);
    }
    else if (isPvw)
    {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, HIGH); // Hijau jika Standby (Preview)
      digitalWrite(BLUE, LOW);
    }
    else
    {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, HIGH); // Biru jika Kamera tidak aktif (Idle)
    }
  }
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

#define LED_PIN 5
#define LED_COUNT 1
#define BRIGHTNESS 50

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define ARB_WHITE leds.Color(255, 255, 255)
#define ARB_BLACK leds.Color(0, 0, 0)

String listenerDeviceName = "esp32_tally";
char tallyarbiter_host[40] = "192.168.0.100";
char tallyarbiter_port[6] = "4455";

Preferences preferences;
WiFiManager wm;
SocketIOclient socket;

JSONVar BusOptions;
JSONVar Devices;
JSONVar DeviceStates;
String DeviceId = "d6fb433c";
String DeviceName = "esp32_tally";
String prevType = "";
String actualType = "";
String actualColor = "";
int actualPriority = 0;
bool networkConnected = false;

void setColor(uint32_t color)
{
  for (int i = 0; i < leds.numPixels(); i++)
  {
    leds.setPixelColor(i, color);
  }
  leds.show();
}

String stripQuot(String str)
{
  if (str[0] == '"')
    str.remove(0, 1);
  if (str.endsWith("\""))
    str.remove(str.length() - 1, 1);
  return str;
}

void wsEmit(String event, const char *payload = NULL)
{
  String msg = payload ? ("[\"" + event + "\"," + payload + "]") : ("[\"" + event + "\"]");
  socket.sendEVENT(msg);
}

String getBusTypeById(String busId)
{
  for (int i = 0; i < BusOptions.length(); i++)
  {
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["type"]);
  }
  return "invalid";
}

String getBusColorById(String busId)
{
  for (int i = 0; i < BusOptions.length(); i++)
  {
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["color"]);
  }
  return "invalid";
}

int getBusPriorityById(String busId)
{
  for (int i = 0; i < BusOptions.length(); i++)
  {
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["priority"]).toInt();
  }
  return 0;
}

void evaluateMode()
{
  if (actualType == prevType)
    return;
  actualColor.replace("#", "");
  long number = actualType != "" ? strtol(actualColor.c_str(), NULL, 16) : 0;
  int r = (number >> 16) & 0xFF;
  int g = (number >> 8) & 0xFF;
  int b = number & 0xFF;
  setColor(actualType != "" ? leds.Color(r, g, b) : ARB_BLACK);
  prevType = actualType;
}

void setDeviceName()
{
  for (int i = 0; i < Devices.length(); i++)
  {
    if (JSON.stringify(Devices[i]["id"]) == "\"" + DeviceId + "\"")
    {
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

void processTallyData()
{
  bool typeChanged = false;
  for (int i = 0; i < DeviceStates.length(); i++)
  {
    if (DeviceStates[i]["sources"].length() > 0)
    {
      typeChanged = true;
      actualType = getBusTypeById(JSON.stringify(DeviceStates[i]["busId"]));
      actualColor = getBusColorById(JSON.stringify(DeviceStates[i]["busId"]));
      actualPriority = getBusPriorityById(JSON.stringify(DeviceStates[i]["busId"]));
    }
  }
  if (!typeChanged)
  {
    actualType = "";
    actualColor = "";
    actualPriority = 0;
  }
  evaluateMode();
}

void socketFlash()
{
  leds.setBrightness(255);
  for (int i = 0; i < 3; i++)
  {
    setColor(ARB_WHITE);
    delay(300);
    setColor(ARB_BLACK);
    delay(300);
  }
  leds.setBrightness(BRIGHTNESS);
  evaluateMode();
}

void socketReassign(String payload)
{
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

void socketConnected()
{
  String deviceObj = "{\"deviceId\": \"" + DeviceId + "\", \"listenerType\": \"" + listenerDeviceName + "\", \"canBeReassigned\": true, \"canBeFlashed\": true, \"supportsChat\": false }";
  wsEmit("listenerclient_connect", deviceObj.c_str());
}

void socketEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case sIOtype_CONNECT:
    socketConnected();
    break;
  case sIOtype_EVENT:
  {
    String msg = (char *)payload;
    String evt = msg.substring(2, msg.indexOf("\"", 2));
    String content = msg.substring(evt.length() + 4);
    content.remove(content.length() - 1);

    if (evt == "bus_options")
      BusOptions = JSON.parse(content);
    if (evt == "reassign")
      socketReassign(content);
    if (evt == "flash")
      socketFlash();
    if (evt == "deviceId")
    {
      DeviceId = content.substring(1, content.length() - 1);
      setDeviceName();
    }
    if (evt == "devices")
    {
      Devices = JSON.parse(content);
      setDeviceName();
    }
    if (evt == "device_states")
    {
      DeviceStates = JSON.parse(content);
      processTallyData();
    }
    break;
  }
  default:
    break;
  }
}

void saveParamCallback()
{
  String str_taHost = wm.server->hasArg("taHostIP") ? wm.server->arg("taHostIP") : "";
  String str_taPort = wm.server->hasArg("taHostPort") ? wm.server->arg("taHostPort") : "";
  preferences.begin("tally-arbiter", false);
  preferences.putString("taHost", str_taHost);
  preferences.putString("taPort", str_taPort);
  preferences.end();
}

void connectToNetwork()
{
  WiFi.mode(WIFI_STA);

  WiFiManagerParameter custom_taServer("taHostIP", "Tally Arbiter Server", tallyarbiter_host, 40);
  WiFiManagerParameter custom_taPort("taHostPort", "Port", tallyarbiter_port, 6);
  wm.addParameter(&custom_taServer);
  wm.addParameter(&custom_taPort);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setConfigPortalTimeout(120);

  networkConnected = wm.autoConnect(listenerDeviceName.c_str());
}

void connectToServer()
{
  socket.onEvent(socketEvent);
  socket.begin(tallyarbiter_host, atoi(tallyarbiter_port));
}

void setup()
{
  leds.begin();
  leds.setBrightness(BRIGHTNESS);
  setColor(leds.Color(0, 0, 255));

  uint64_t chipid = ESP.getEfuseMac();
  listenerDeviceName = "tally-" + String((uint32_t)chipid, HEX);

  preferences.begin("tally-arbiter", false);
  if (preferences.getString("deviceid").length() > 0)
    DeviceId = preferences.getString("deviceid");
  if (preferences.getString("devicename").length() > 0)
    DeviceName = preferences.getString("devicename");
  if (preferences.getString("taHost").length() > 0)
    preferences.getString("taHost").toCharArray(tallyarbiter_host, 40);
  if (preferences.getString("taPort").length() > 0)
    preferences.getString("taPort").toCharArray(tallyarbiter_port, 6);
  preferences.end();

  connectToNetwork();
  while (!networkConnected)
  {
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

void loop()
{
  ArduinoOTA.handle();
  socket.loop();
}
#endif

// ============================================================================
// MODE_RX_TALLYHUB_ESP32
// Ported from the ESP32-S3 "Tally Hub" reference project (TFT + QR code
// display), but with the TFT/QR hardware layer replaced by a WS2812B/NeoPixel
// LED. All networking/logic (WiFiManager captive portal, built-in web config
// server, UDP registration/heartbeat/reconnect protocol, admin messages,
// assignment confirmation, JSON message handling) is preserved unchanged.
// ============================================================================
#ifdef MODE_RX_TALLYHUB_ESP32
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

#define FIRMWARE_VERSION "1.0.1"
#define DEVICE_MODEL "ESP32-WS2812B"

// ---- LED hardware config (edit to match your wiring) ----
#define LED_PIN 5
#define LED_COUNT 1
#define LED_BRIGHTNESS 80
#define BOOT_BUTTON_PIN 0

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#define TH_BLACK leds.Color(0, 0, 0)
#define TH_WHITE leds.Color(255, 255, 255)
#define TH_RED leds.Color(255, 0, 0)
#define TH_GREEN leds.Color(0, 255, 0)
#define TH_BLUE leds.Color(0, 0, 255)
#define TH_YELLOW leds.Color(255, 200, 0)

WebServer server(80);
WiFiUDP udp;
Preferences preferences;
WiFiManager wifiManager;

String deviceName = "ESP32 Tally Light";
String deviceID = "tally-";
String macAddress = "";
String ipAddress = "";
String hubIP = "192.168.0.1";
int hubPort = 3000;
String assignedSource = "";
String assignedSourceName = "";
String currentSource = "";
String customDisplayName = "";
String currentStatus = "INIT";
bool isConnected = false;
bool isRegisteredWithHub = false;
bool isAssigned = false;

bool isProgram = false;
bool isPreview = false;
bool isRecording = false;
bool isStreaming = false;

unsigned long lastHeartbeat = 0;
unsigned long lastLedUpdate = 0;
unsigned long bootTime = 0;
unsigned long lastHubResponse = 0;
unsigned long hubConnectionAttempts = 0;
unsigned long lastReconnectionAttempt = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastUDPRestart = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000;
const unsigned long WIFI_CHECK_INTERVAL = 5000;
const unsigned long UDP_RESTART_INTERVAL = 600000;
const unsigned long HUB_TIMEOUT = 60000;
const unsigned long MAX_HUB_RECONNECT_ATTEMPTS = 5;
const unsigned long MIN_RECONNECTION_INTERVAL = 15000;
const unsigned long CONNECTION_CHECK_INTERVAL = 2000;

bool showingRegistrationStatus = false;
unsigned long registrationStatusStart = 0;
String registrationStatusMessage = "";
uint32_t registrationStatusColor = TH_GREEN;

static String adminMessageText = "";
static bool adminMessageActive = false;
static unsigned long adminMessageExpire = 0;
static uint32_t adminMessageBg = TH_BLUE;
static String adminMessageId = "";

bool showingAssignmentConfirmation = false;
unsigned long assignmentConfirmationStart = 0;
String confirmationSourceName = "";
bool confirmationIsAssigned = false;

unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;
const unsigned long WIFI_RESET_HOLD_TIME = 5000;

void setupWiFi();
void setupWebServer();
void loadConfiguration();
void saveConfiguration();
void registerDevice();
void sendHeartbeat();
void handleUDPMessages();
void updateLED();
void updateStatus(const String &status);
void setLedColor(uint32_t color);
void handleRoot();
void handleConfig();
void handleSave();
void handleSources();
void handleStatus();
void handleAssign();
void handleUnassign();
void handleSaveDisplayName();
void handleReset();
void handleRestart();
void handleNotFound();
String formatUptime();
String cleanSourceName(String sourceName);
void checkButtonForWiFiReset();
void reconnectWiFi();
void restartUDP();
void checkWiFiConnection();
void ensureUDPConnection();

void setLedColor(uint32_t color)
{
  for (int i = 0; i < LED_COUNT; i++)
    leds.setPixelColor(i, color);
  leds.show();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 Tally Light (WS2812B) v" + String(FIRMWARE_VERSION) + " ===");
  bootTime = millis();

  leds.begin();
  leds.setBrightness(LED_BRIGHTNESS);
  setLedColor(TH_BLACK);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  macAddress = WiFi.macAddress();
  deviceID = "tally-" + macAddress;
  deviceID.replace(":", "");
  deviceID.toLowerCase();
  Serial.println("Device ID: " + deviceID);

  loadConfiguration();
  setupWiFi();

  if (WiFi.status() == WL_CONNECTED)
  {
    ipAddress = WiFi.localIP().toString();
    Serial.println("IP Address: " + ipAddress);
    setupWebServer();
    ArduinoOTA.setHostname(deviceID.c_str());
    ArduinoOTA.begin();
    if (udp.begin(3000))
    {
      Serial.println("UDP started on port 3000");
    }
    else
    {
      Serial.println("Failed to start UDP");
    }
    lastHubResponse = millis();
    registerDevice();
    updateStatus("READY");
  }
  else
  {
    updateStatus("NO_WIFI");
  }
  delay(500);
}

void checkHubConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    if (isConnected || isRegisteredWithHub)
    {
      isConnected = false;
      isRegisteredWithHub = false;
      updateStatus("NO_WIFI");
    }
    return;
  }

  if (!isRegisteredWithHub)
  {
    unsigned long timeSinceLastAttempt = millis() - lastReconnectionAttempt;
    if (timeSinceLastAttempt < MIN_RECONNECTION_INTERVAL)
      return;

    if (hubConnectionAttempts < MAX_HUB_RECONNECT_ATTEMPTS)
    {
      hubConnectionAttempts++;
      lastReconnectionAttempt = millis();
      showingRegistrationStatus = true;
      registrationStatusStart = millis();
      registrationStatusMessage = "Connecting...";
      registrationStatusColor = TH_YELLOW;
      registerDevice();
    }
    else
    {
      static unsigned long lastAttemptReset = 0;
      if (millis() - lastAttemptReset > 300000)
      {
        hubConnectionAttempts = 0;
        lastAttemptReset = millis();
        return;
      }
      lastReconnectionAttempt = millis();
      showingRegistrationStatus = true;
      registrationStatusStart = millis();
      registrationStatusMessage = "Hub Lost";
      registrationStatusColor = TH_RED;
      isConnected = false;
      isRegisteredWithHub = false;
      registerDevice();
    }
    return;
  }

  unsigned long timeSinceLastResponse = millis() - lastHubResponse;
  if (timeSinceLastResponse > HUB_TIMEOUT)
  {
    unsigned long timeSinceLastAttempt = millis() - lastReconnectionAttempt;
    if (timeSinceLastAttempt < MIN_RECONNECTION_INTERVAL)
      return;

    isRegisteredWithHub = false;

    if (hubConnectionAttempts < MAX_HUB_RECONNECT_ATTEMPTS)
    {
      hubConnectionAttempts++;
      lastReconnectionAttempt = millis();
      showingRegistrationStatus = true;
      registrationStatusStart = millis();
      registrationStatusMessage = "Reconnecting...";
      registrationStatusColor = TH_YELLOW;
      registerDevice();
    }
    else
    {
      static unsigned long lastAttemptReset2 = 0;
      if (millis() - lastAttemptReset2 > 300000)
      {
        hubConnectionAttempts = 0;
        lastAttemptReset2 = millis();
        return;
      }
      lastReconnectionAttempt = millis();
      showingRegistrationStatus = true;
      registrationStatusStart = millis();
      registrationStatusMessage = "Hub Lost";
      registrationStatusColor = TH_RED;
      isConnected = false;
      isRegisteredWithHub = false;
      registerDevice();
    }
  }
}

void monitorConnectionStatus()
{
  static unsigned long lastConnectionCheck = 0;
  if (millis() - lastConnectionCheck > CONNECTION_CHECK_INTERVAL)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      if (isConnected || isRegisteredWithHub)
      {
        isConnected = false;
        isRegisteredWithHub = false;
        updateStatus("NO_WIFI");
        updateLED();
      }
    }
    if (WiFi.status() == WL_CONNECTED && (isConnected || isRegisteredWithHub))
    {
      unsigned long timeSinceLastResponse = millis() - lastHubResponse;
      if (timeSinceLastResponse > HUB_TIMEOUT)
      {
        isConnected = false;
        isRegisteredWithHub = false;
        updateStatus("HUB_LOST");
        updateLED();
      }
    }
    lastConnectionCheck = millis();
  }
}

void loop()
{
  ArduinoOTA.handle();
  checkButtonForWiFiReset();
  monitorConnectionStatus();

  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL)
  {
    checkWiFiConnection();
    lastWiFiCheck = millis();
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    if (isConnected || isRegisteredWithHub)
    {
      isConnected = false;
      isRegisteredWithHub = false;
    }
    updateStatus("NO_WIFI");
    updateLED();
    delay(500);
    return;
  }

  if (millis() - lastUDPRestart > UDP_RESTART_INTERVAL)
  {
    restartUDP();
    lastUDPRestart = millis();
  }

  static unsigned long lastUDPHealthCheck = 0;
  if (millis() - lastUDPHealthCheck > 300000)
  {
    ensureUDPConnection();
    lastUDPHealthCheck = millis();
  }

  server.handleClient();
  handleUDPMessages();
  checkHubConnection();

  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL)
  {
    sendHeartbeat();
    lastHeartbeat = millis();
  }

  if (millis() - lastLedUpdate > 200)
  {
    updateLED();
    lastLedUpdate = millis();
  }

  delay(20);
}

void checkButtonForWiFiReset()
{
  int buttonState = digitalRead(BOOT_BUTTON_PIN);
  if (buttonState == LOW)
  {
    if (!buttonWasPressed)
    {
      buttonPressStart = millis();
      buttonWasPressed = true;
    }
    else if (millis() - buttonPressStart > WIFI_RESET_HOLD_TIME)
    {
      setLedColor(TH_RED);
      delay(1000);
      wifiManager.resetSettings();
      preferences.begin("tally", false);
      preferences.clear();
      preferences.end();
      delay(500);
      ESP.restart();
    }
  }
  else
  {
    if (buttonWasPressed)
    {
      unsigned long held = millis() - buttonPressStart;
      if (held < WIFI_RESET_HOLD_TIME && adminMessageActive)
      {
        String snippet = adminMessageText.substring(0, 24);
        adminMessageActive = false;
        adminMessageText = "";
        JsonDocument ack;
        ack["type"] = "admin_message_ack";
        ack["deviceId"] = deviceID;
        ack["method"] = "button";
        ack["timestamp"] = millis();
        ack["textSnippet"] = snippet;
        if (adminMessageId.length() > 0)
          ack["id"] = adminMessageId;
        String payload;
        serializeJson(ack, payload);
        int r = udp.beginPacket(hubIP.c_str(), hubPort);
        if (r == 1)
        {
          udp.print(payload);
          udp.endPacket();
        }
        updateLED();
      }
    }
    buttonWasPressed = false;
  }
}

void setupWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  wifiManager.setAPCallback([](WiFiManager *myWiFiManager)
                            {
    // No QR-code screen on LED-only hardware; blink blue to indicate AP/config mode.
    for (int i = 0; i < 3; i++) {
      setLedColor(TH_BLUE);
      delay(200);
      setLedColor(TH_BLACK);
      delay(200);
    } });

  wifiManager.setSaveConfigCallback([]()
                                    {
    setLedColor(TH_GREEN);
    delay(1000); });

  String apName = "TallyLight-" + deviceID.substring(6, 12);
  wifiManager.setBreakAfterConfig(true);
  bool connected = wifiManager.autoConnect(apName.c_str());
  if (!connected)
  {
    Serial.println("[WiFiManager] Failed to connect or no credentials. Starting AP mode.");
    setLedColor(TH_RED);
    delay(2000);
  }
  else
  {
    Serial.println("WiFi connected! IP: " + WiFi.localIP().toString());
  }
}

void setupWebServer()
{
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/sources", handleSources);
  server.on("/assign", HTTP_POST, handleAssign);
  server.on("/unassign", HTTP_POST, handleUnassign);
  server.on("/save_display_name", HTTP_POST, handleSaveDisplayName);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/restart", HTTP_POST, handleRestart);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loadConfiguration()
{
  preferences.begin("tally", false);
  deviceName = preferences.getString("deviceName", "ESP32 Tally Light");
  hubIP = preferences.getString("hubIP");
  hubPort = preferences.getInt("hubPort");
  assignedSource = preferences.getString("assignedSource", "");
  assignedSourceName = preferences.getString("assignedSourceName", "");
  customDisplayName = preferences.getString("customDisplayName", "");
  preferences.end();
  isAssigned = (assignedSource.length() > 0);
}

void saveConfiguration()
{
  preferences.begin("tally", false);
  preferences.putString("deviceName", deviceName);
  preferences.putString("hubIP", hubIP);
  preferences.putInt("hubPort", hubPort);
  preferences.putString("assignedSource", assignedSource);
  preferences.putString("assignedSourceName", assignedSourceName);
  preferences.putString("customDisplayName", customDisplayName);
  preferences.end();
}

void registerDevice()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  ensureUDPConnection();

  JsonDocument doc;
  doc["type"] = "register";
  doc["deviceId"] = deviceID;
  doc["deviceName"] = deviceName;
  doc["deviceType"] = "esp32-ws2812b";
  doc["model"] = DEVICE_MODEL;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["ip"] = ipAddress;
  doc["mac"] = macAddress;
  doc["wifiRSSI"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap();
  if (isAssigned && assignedSource.length() > 0)
  {
    doc["assignedSource"] = assignedSource;
    doc["isAssigned"] = true;
  }
  else
  {
    doc["isAssigned"] = false;
  }

  String message;
  serializeJson(doc, message);
  int result = udp.beginPacket(hubIP.c_str(), hubPort);
  if (result == 1)
  {
    udp.print(message);
    if (udp.endPacket() != 1)
      restartUDP();
  }
  else
  {
    restartUDP();
  }
}

void sendHeartbeat()
{
  if (!isRegisteredWithHub)
    return;
  ensureUDPConnection();

  JsonDocument doc;
  doc["type"] = "heartbeat";
  doc["deviceId"] = deviceID;
  doc["uptime"] = millis() - bootTime;
  doc["status"] = currentStatus;
  doc["assignedSource"] = assignedSource;
  doc["wifiRSSI"] = WiFi.RSSI();
  doc["freeHeap"] = ESP.getFreeHeap();

  String message;
  serializeJson(doc, message);
  int result = udp.beginPacket(hubIP.c_str(), hubPort);
  if (result == 1)
  {
    udp.print(message);
    if (udp.endPacket() != 1)
      restartUDP();
  }
  else
  {
    restartUDP();
  }
}

void handleUDPMessages()
{
  int packetSize = udp.parsePacket();
  if (packetSize == 0)
    return;

  char incomingPacket[512];
  int len = udp.read(incomingPacket, 511);
  if (len > 0)
    incomingPacket[len] = 0;

  lastHubResponse = millis();
  hubConnectionAttempts = 0;

  JsonDocument doc;
  if (deserializeJson(doc, incomingPacket) != DeserializationError::Ok)
    return;

  String type = doc["type"];

  if (type == "tally")
  {
    if (doc["data"].is<JsonObject>())
    {
      JsonObject data = doc["data"];
      String sourceId = data["id"];
      String sourceName = data["name"];
      bool program = data["program"];
      bool preview = data["preview"];
      bool recording = data["recording"] | false;
      bool streaming = data["streaming"] | false;

      if (isAssigned && assignedSource.length() > 0 && sourceId == assignedSource)
      {
        isProgram = program;
        isPreview = preview;
        isRecording = recording;
        isStreaming = streaming;
        if (customDisplayName.length() == 0)
          currentSource = cleanSourceName(sourceName);
        if (program)
          updateStatus("PROGRAM");
        else if (preview)
          updateStatus("PREVIEW");
        else
          updateStatus("IDLE");
      }
    }
    else
    {
      String sourceId = doc["sourceId"];
      bool program = doc["program"];
      bool preview = doc["preview"];
      bool recording = doc["recording"] | false;
      bool streaming = doc["streaming"] | false;

      if (isAssigned && assignedSource.length() > 0 && sourceId == assignedSource)
      {
        isProgram = program;
        isPreview = preview;
        isRecording = recording;
        isStreaming = streaming;
        if (program)
          updateStatus("PROGRAM");
        else if (preview)
          updateStatus("PREVIEW");
        else
          updateStatus("IDLE");
      }
    }
  }
  else if (type == "assignment")
  {
    if (doc["data"].is<JsonObject>())
    {
      JsonObject data = doc["data"];
      String newSource = data["sourceId"];
      String sourceName = data["sourceName"];
      String mode = data["mode"];

      if (mode == "assigned")
      {
        assignedSource = newSource;
        assignedSourceName = sourceName;
        isAssigned = true;
        currentSource = (customDisplayName.length() == 0) ? cleanSourceName(sourceName) : customDisplayName;
        saveConfiguration();
        showingAssignmentConfirmation = true;
        assignmentConfirmationStart = millis();
        confirmationSourceName = cleanSourceName(sourceName);
        confirmationIsAssigned = true;
        isProgram = isPreview = isRecording = isStreaming = false;
      }
      else
      {
        assignedSource = "";
        assignedSourceName = "";
        currentSource = "";
        customDisplayName = "";
        isAssigned = false;
        saveConfiguration();
        showingAssignmentConfirmation = true;
        assignmentConfirmationStart = millis();
        confirmationSourceName = "";
        confirmationIsAssigned = false;
        isProgram = isPreview = isRecording = isStreaming = false;
      }
    }
    else
    {
      String newSource = doc["sourceId"];
      if (newSource != assignedSource)
      {
        if (newSource.length() > 0)
        {
          assignedSource = newSource;
          isAssigned = true;
          saveConfiguration();
          showingAssignmentConfirmation = true;
          assignmentConfirmationStart = millis();
          confirmationSourceName = cleanSourceName(newSource);
          confirmationIsAssigned = true;
        }
        else
        {
          assignedSource = "";
          isAssigned = false;
          saveConfiguration();
          showingAssignmentConfirmation = true;
          assignmentConfirmationStart = millis();
          confirmationSourceName = "";
          confirmationIsAssigned = false;
        }
        isProgram = isPreview = isRecording = isStreaming = false;
      }
    }
  }
  else if (type == "register_required")
  {
    showingRegistrationStatus = true;
    registrationStatusStart = millis();
    registrationStatusMessage = "Re-register";
    registrationStatusColor = TH_YELLOW;
    registerDevice();
  }
  else if (type == "registered")
  {
    isRegisteredWithHub = true;
    hubConnectionAttempts = 0;
    if (!isAssigned || assignedSource.length() == 0)
      updateStatus("READY");
    showingRegistrationStatus = true;
    registrationStatusStart = millis();
    registrationStatusMessage = "Connected";
    registrationStatusColor = TH_GREEN;
  }
  else if (type == "heartbeat_ack")
  {
    hubConnectionAttempts = 0;
  }
  else if (type == "admin_message")
  {
    const char *txt = doc["text"] | "";
    if (strlen(txt) > 0)
    {
      adminMessageText = String(txt);
      adminMessageId = doc["id"].isNull() ? String("") : String((const char *)doc["id"].as<const char *>());
      unsigned long dur = doc["duration"].isNull() ? 8000UL : (unsigned long)doc["duration"].as<unsigned long>();
      if (dur < 1000UL)
        dur = 1000UL;
      if (dur > 30000UL)
        dur = 30000UL;
      adminMessageExpire = millis() + dur;
      adminMessageBg = TH_BLUE;
      if (!doc["color"].isNull())
      {
        String c = doc["color"].as<String>();
        if (c.startsWith("#"))
          c.remove(0, 1);
        if (c.length() == 6)
        {
          char rHex[3] = {c[0], c[1], '\0'};
          char gHex[3] = {c[2], c[3], '\0'};
          char bHex[3] = {c[4], c[5], '\0'};
          int r = strtol(rHex, nullptr, 16);
          int g = strtol(gHex, nullptr, 16);
          int b = strtol(bHex, nullptr, 16);
          adminMessageBg = leds.Color(r, g, b);
        }
      }
      adminMessageActive = true;
      updateLED();
    }
  }
}

void updateLED()
{
  if (adminMessageActive && millis() > adminMessageExpire)
  {
    adminMessageActive = false;
    adminMessageText = "";
  }

  // Admin message: blink the message color instead of showing text
  if (adminMessageActive && !showingAssignmentConfirmation && !showingRegistrationStatus)
  {
    setLedColor((millis() / 250) % 2 == 0 ? adminMessageBg : TH_BLACK);
    return;
  }

  // Assignment confirmation: solid green (assigned) or red (unassigned) for 2s
  if (showingAssignmentConfirmation)
  {
    if (millis() - assignmentConfirmationStart < 2000)
    {
      setLedColor(confirmationIsAssigned ? TH_GREEN : TH_RED);
      return;
    }
    else
    {
      showingAssignmentConfirmation = false;
    }
  }

  // Registration/reconnection status: brief blink
  if (showingRegistrationStatus)
  {
    unsigned long displayDuration = (registrationStatusMessage == "Re-register") ? 500 : 1000;
    if (millis() - registrationStatusStart < displayDuration)
    {
      setLedColor((millis() / 150) % 2 == 0 ? registrationStatusColor : TH_BLACK);
      return;
    }
    else
    {
      showingRegistrationStatus = false;
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    setLedColor((millis() / 400) % 2 == 0 ? TH_RED : TH_BLACK);
    return;
  }

  if (!isRegisteredWithHub)
  {
    unsigned long timeSinceLastResponse = millis() - lastHubResponse;
    if ((timeSinceLastResponse > HUB_TIMEOUT && lastHubResponse > 0) ||
        (lastHubResponse == 0 && millis() > 30000))
    {
      setLedColor((millis() / 400) % 2 == 0 ? TH_RED : TH_BLACK);
    }
    else
    {
      setLedColor((millis() / 400) % 2 == 0 ? TH_BLUE : TH_BLACK);
    }
    return;
  }

  if (!isAssigned)
  {
    setLedColor(TH_WHITE);
    return;
  }

  if (currentStatus == "PROGRAM")
  {
    setLedColor(TH_RED);
  }
  else if (currentStatus == "PREVIEW")
  {
    setLedColor(TH_GREEN);
  }
  else if (currentStatus == "IDLE")
  {
    setLedColor(TH_BLUE);
  }
  else if (currentStatus == "NO_WIFI")
  {
    setLedColor((millis() / 400) % 2 == 0 ? TH_RED : TH_BLACK);
  }
  else if (currentStatus == "HUB_LOST")
  {
    setLedColor((millis() / 400) % 2 == 0 ? TH_RED : TH_BLACK);
  }
  else
  {
    setLedColor(TH_BLACK);
  }
}

void updateStatus(const String &status)
{
  currentStatus = status;
}

void handleRoot()
{
  String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Tally Configuration</title>";
  html += "<style>body{font-family:-apple-system,BlinkMacSystemFont,system-ui,sans-serif;";
  html += "background:#F2F2F7;margin:0;padding:2rem 1rem;} .container{max-width:480px;margin:0 auto;}";
  html += ".card{background:#fff;border-radius:16px;padding:1.5rem;margin-bottom:1.5rem;box-shadow:0 2px 10px rgba(0,0,0,0.08);}";
  html += "h1{font-size:22px;margin-bottom:1rem;} .form-group{margin-bottom:1rem;}";
  html += "label{font-size:13px;font-weight:600;margin-bottom:0.5rem;display:block;}";
  html += "input{border:1px solid #C7C7CC;border-radius:6px;padding:0.6rem 0.75rem;font-size:14px;width:100%;box-sizing:border-box;}";
  html += ".btn{border:none;padding:0.75rem 1.25rem;border-radius:10px;font-size:15px;font-weight:600;cursor:pointer;width:100%;margin-bottom:0.75rem;color:#fff;}";
  html += ".btn-primary{background:#007AFF;} .btn-secondary{background:#8E8E93;} .btn-danger{background:#FF3B30;}";
  html += "</style></head><body><div class='container'>";
  html += "<div class='card'><h1>ESP32 Tally Light</h1>";
  html += "<p>Device: " + deviceName + "</p><p>IP: " + WiFi.localIP().toString() + "</p>";
  html += "<p>Hub: " + hubIP + ":" + String(hubPort) + "</p></div>";
  html += "<div class='card'><h1>WiFi/Hub Configuration</h1>";
  html += "<form action='/save' method='post'>";
  html += "<div class='form-group'><label>Device Name</label>";
  html += "<input type='text' name='device_name' value='" + deviceName + "' required></div>";
  html += "<div class='form-group'><label>Hub Server IP</label>";
  html += "<input type='text' name='hub_ip' value='" + hubIP + "' required></div>";
  html += "<div class='form-group'><label>Hub Server Port</label>";
  html += "<input type='number' name='hub_port' value='" + String(hubPort) + "' min='1' max='65535' required></div>";
  html += "<div class='form-group'><label>Device ID</label>";
  html += "<input type='text' name='device_id' value='" + deviceID + "' required></div>";
  html += "<button type='submit' class='btn btn-primary'>Save Configuration</button></form></div>";
  html += "<div class='card'><h1>Device Actions</h1>";
  html += "<button onclick=\"window.location='/sources'\" class='btn btn-secondary'>Manage Sources</button>";
  html += "<button onclick=\"window.location='/status'\" class='btn btn-secondary'>Device Status</button>";
  html += "<button onclick='restart()' class='btn btn-secondary'>Restart Device</button>";
  html += "<button onclick='resetConfig()' class='btn btn-danger'>Factory Reset</button></div></div>";
  html += "<script>function restart(){if(confirm('Restart the device now?')){fetch('/restart',{method:'POST'}).then(()=>{alert('Device is restarting...');});}}";
  html += "function resetConfig(){if(confirm('WARNING: This will erase ALL settings!')){fetch('/reset',{method:'POST'}).then(()=>{alert('Factory reset complete.');});}}</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfig() { handleRoot(); }

void handleSave()
{
  deviceName = server.arg("device_name");
  hubIP = server.arg("hub_ip");
  hubPort = server.arg("hub_port").toInt();
  deviceID = server.arg("device_id");
  saveConfiguration();

  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Configuration Saved</title></head><body style='font-family:sans-serif;text-align:center;padding:2rem;'>";
  html += "<h1>Configuration Saved!</h1><p>Restarting and connecting to " + hubIP + ":" + String(hubPort) + "...</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
  delay(1500);
  ESP.restart();
}

void handleRestart()
{
  server.send(200, "text/html", "<h2>Device Restarting</h2><p>Please wait...</p>");
  delay(1000);
  ESP.restart();
}

String formatUptime()
{
  unsigned long uptime = millis() - bootTime;
  unsigned long seconds = uptime / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  String result = "";
  if (days > 0)
    result += String(days) + "d ";
  if (hours % 24 > 0)
    result += String(hours % 24) + "h ";
  if (minutes % 60 > 0)
    result += String(minutes % 60) + "m ";
  result += String(seconds % 60) + "s";
  return result;
}

String cleanSourceName(String sourceName)
{
  String cleaned = sourceName;
  if (cleaned.startsWith("obs-"))
    cleaned = cleaned.substring(4);
  if (cleaned.startsWith("vmix-"))
    cleaned = cleaned.substring(5);
  if (cleaned.startsWith("source-"))
    cleaned = cleaned.substring(7);
  if (cleaned.startsWith("scene-"))
    cleaned = cleaned.substring(6);
  return cleaned;
}

void checkWiFiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
    reconnectWiFi();
}

void reconnectWiFi()
{
  static unsigned long lastReconnectAttempt = 0;
  static int reconnectAttempts = 0;
  const unsigned long RECONNECT_INTERVAL = 30000;
  const int MAX_RECONNECT_ATTEMPTS = 10;

  if (millis() - lastReconnectAttempt < RECONNECT_INTERVAL)
    return;
  lastReconnectAttempt = millis();
  reconnectAttempts++;

  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  unsigned long connectStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - connectStart < 15000)
  {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    ipAddress = WiFi.localIP().toString();
    reconnectAttempts = 0;
    restartUDP();
    isConnected = false;
    isRegisteredWithHub = false;
    lastHubResponse = 0;
    hubConnectionAttempts = 0;
  }
  else if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS)
  {
    ESP.restart();
  }
}

void restartUDP()
{
  udp.stop();
  delay(100);
  udp.begin(7411);
}

void ensureUDPConnection()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    static unsigned long lastUDPTest = 0;
    const unsigned long UDP_TEST_INTERVAL = 300000;
    if (millis() - lastUDPTest > UDP_TEST_INTERVAL)
    {
      JsonDocument doc;
      doc["type"] = "ping";
      doc["deviceId"] = deviceID;
      doc["timestamp"] = millis();
      String message;
      serializeJson(doc, message);
      int result = udp.beginPacket(hubIP.c_str(), hubPort);
      if (result == 1)
      {
        udp.print(message);
        udp.endPacket();
      }
      lastUDPTest = millis();
    }
  }
}

void handleSources()
{
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Source Management</title><style>body{font-family:sans-serif;background:#F2F2F7;padding:2rem;}";
  html += ".card{background:#fff;border-radius:16px;padding:1.5rem;margin-bottom:1.5rem;}";
  html += "input{width:100%;padding:0.6rem;box-sizing:border-box;margin-bottom:0.5rem;}";
  html += ".btn{padding:0.75rem 1.5rem;border:none;border-radius:10px;color:#fff;cursor:pointer;margin:0.25rem;}";
  html += ".btn-primary{background:#007AFF;} .btn-secondary{background:#8E8E93;} .btn-danger{background:#FF3B30;}";
  html += "</style></head><body>";
  html += "<div class='card'><h2>Current Assignment</h2>";
  html += "<p>Source ID: " + (assignedSource.length() > 0 ? assignedSource : "None") + "</p>";
  html += "<p>Custom Display Name: " + (customDisplayName.length() > 0 ? customDisplayName : "None") + "</p>";
  html += "<p>Current Source: " + (currentSource.length() > 0 ? currentSource : "None") + "</p></div>";
  html += "<div class='card'><h2>Custom Display Name</h2>";
  html += "<form action='/save_display_name' method='post'>";
  html += "<input type='text' name='display_name' value='" + customDisplayName + "' maxlength='20' placeholder='Leave empty to use source name'>";
  html += "<button type='submit' class='btn btn-primary'>Save Display Name</button></form></div>";
  html += "<div class='card'><h2>Manual Assignment</h2>";
  html += "<form action='/assign' method='post'>";
  html += "<input type='text' name='source' value='" + assignedSource + "' placeholder='Enter source ID'>";
  html += "<button type='submit' class='btn btn-primary'>Assign Source</button></form>";
  html += "<form action='/unassign' method='post' style='margin-top:1rem;'>";
  html += "<button type='submit' class='btn btn-danger'>Unassign Device</button></form></div>";
  html += "<div class='card'><a href='/' class='btn btn-secondary'>Back to Main</a></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleAssign()
{
  String sourceId = server.arg("source");
  if (sourceId.length() > 0)
  {
    assignedSource = sourceId;
    isAssigned = true;
    saveConfiguration();
    server.send(200, "text/html", "<h2>Source Assigned</h2><p>" + sourceId + "</p><script>setTimeout(()=>{window.location='/sources';},2000);</script>");
  }
  else
  {
    server.send(400, "text/plain", "Missing source parameter");
  }
}

void handleUnassign()
{
  assignedSource = "";
  isAssigned = false;
  customDisplayName = "";
  saveConfiguration();
  server.send(200, "text/html", "<h2>Device Unassigned</h2><script>setTimeout(()=>{window.location='/sources';},2000);</script>");
}

void handleSaveDisplayName()
{
  String displayName = server.arg("display_name");
  customDisplayName = displayName;
  if (customDisplayName.length() > 0)
    currentSource = customDisplayName;
  else if (assignedSource.length() > 0)
    currentSource = cleanSourceName(assignedSource);
  else
    currentSource = "";
  saveConfiguration();
  server.send(200, "text/html", "<h2>Display Name Saved</h2><script>setTimeout(()=>{window.location='/sources';},2000);</script>");
}

void handleReset()
{
  preferences.begin("tally", false);
  preferences.clear();
  preferences.end();
  server.send(200, "text/html", "<h2>Factory Reset Complete</h2><p>Device will restart in configuration mode...</p>");
  delay(1500);
  ESP.restart();
}

void handleStatus()
{
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Device Status</title></head><body style='font-family:sans-serif;padding:2rem;'>";
  html += "<h1>Device Status</h1>";
  html += "<p>Registration: " + String(isRegisteredWithHub ? "Registered" : "Not Registered") + "</p>";
  html += "<p>Tally State: " + String(isProgram ? "Program" : (isPreview ? "Preview" : "Off")) + "</p>";
  html += "<p>Uptime: " + formatUptime() + "</p>";
  html += "<a href='/'>Back to Main</a></body></html>";
  server.send(200, "text/html", html);
}

void handleNotFound()
{
  String message = "File Not Found\n\nURI: " + server.uri() + "\n";
  server.send(404, "text/plain", message);
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

#define LED_PIN 4 // D2
#define LED_COUNT 1
#define BRIGHTNESS 50

#define EEPROM_SIZE 256
#define ADDR_DEVICEID 0
#define ADDR_DEVICENAME 40
#define ADDR_TAHOST 80
#define ADDR_TAPORT 120

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define ARB_WHITE leds.Color(255, 255, 255)
#define ARB_BLACK leds.Color(0, 0, 0)

String listenerDeviceName = "tally-arbiter";
char tallyarbiter_host[40] = "192.168.1.2";
char tallyarbiter_port[6] = "4455";

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

void eepromWriteString(int addr, String data, int maxLen)
{
  int len = min((int)data.length(), maxLen - 1);
  for (int i = 0; i < len; i++)
    EEPROM.write(addr + i, data[i]);
  EEPROM.write(addr + len, 0);
  EEPROM.commit();
}

String eepromReadString(int addr, int maxLen)
{
  char buf[maxLen];
  int i = 0;
  for (; i < maxLen - 1; i++)
  {
    char c = EEPROM.read(addr + i);
    if (c == 0 || c == 255)
      break;
    buf[i] = c;
  }
  buf[i] = 0;
  return String(buf);
}

void setColor(uint32_t color)
{
  for (int i = 0; i < leds.numPixels(); i++)
  {
    leds.setPixelColor(i, color);
  }
  leds.show();
}

String stripQuot(String str)
{
  if (str[0] == '"')
    str.remove(0, 1);
  if (str.endsWith("\""))
    str.remove(str.length() - 1, 1);
  return str;
}

void wsEmit(String event, const char *payload = NULL)
{
  String msg = payload ? ("[\"" + event + "\"," + payload + "]") : ("[\"" + event + "\"]");
  socket.sendEVENT(msg);
}

String getBusTypeById(String busId)
{
  for (int i = 0; i < BusOptions.length(); i++)
  {
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["type"]);
  }
  return "invalid";
}

String getBusColorById(String busId)
{
  for (int i = 0; i < BusOptions.length(); i++)
  {
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["color"]);
  }
  return "invalid";
}

int getBusPriorityById(String busId)
{
  for (int i = 0; i < BusOptions.length(); i++)
  {
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["priority"]).toInt();
  }
  return 0;
}

void evaluateMode()
{
  if (actualType == prevType)
    return;
  actualColor.replace("#", "");
  long number = actualType != "" ? strtol(actualColor.c_str(), NULL, 16) : 0;
  int r = (number >> 16) & 0xFF;
  int g = (number >> 8) & 0xFF;
  int b = number & 0xFF;
  setColor(actualType != "" ? leds.Color(r, g, b) : ARB_BLACK);
  prevType = actualType;
}

void setDeviceName()
{
  for (int i = 0; i < Devices.length(); i++)
  {
    if (JSON.stringify(Devices[i]["id"]) == "\"" + DeviceId + "\"")
    {
      String strDevice = JSON.stringify(Devices[i]["name"]);
      DeviceName = strDevice.substring(1, strDevice.length() - 1);
      break;
    }
  }
  eepromWriteString(ADDR_DEVICENAME, DeviceName, 40);
  evaluateMode();
}

void processTallyData()
{
  bool typeChanged = false;
  for (int i = 0; i < DeviceStates.length(); i++)
  {
    if (DeviceStates[i]["sources"].length() > 0)
    {
      typeChanged = true;
      actualType = getBusTypeById(JSON.stringify(DeviceStates[i]["busId"]));
      actualColor = getBusColorById(JSON.stringify(DeviceStates[i]["busId"]));
      actualPriority = getBusPriorityById(JSON.stringify(DeviceStates[i]["busId"]));
    }
  }
  if (!typeChanged)
  {
    actualType = "";
    actualColor = "";
    actualPriority = 0;
  }
  evaluateMode();
}

void socketFlash()
{
  leds.setBrightness(255);
  for (int i = 0; i < 3; i++)
  {
    setColor(ARB_WHITE);
    delay(300);
    setColor(ARB_BLACK);
    delay(300);
  }
  leds.setBrightness(BRIGHTNESS);
  evaluateMode();
}

void socketReassign(String payload)
{
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

void socketConnected()
{
  String deviceObj = "{\"deviceId\": \"" + DeviceId + "\", \"listenerType\": \"" + listenerDeviceName + "\", \"canBeReassigned\": true, \"canBeFlashed\": true, \"supportsChat\": false }";
  wsEmit("listenerclient_connect", deviceObj.c_str());
}

void socketEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case sIOtype_CONNECT:
    socketConnected();
    break;
  case sIOtype_EVENT:
  {
    String msg = (char *)payload;
    String evt = msg.substring(2, msg.indexOf("\"", 2));
    String content = msg.substring(evt.length() + 4);
    content.remove(content.length() - 1);

    if (evt == "bus_options")
      BusOptions = JSON.parse(content);
    if (evt == "reassign")
      socketReassign(content);
    if (evt == "flash")
      socketFlash();
    if (evt == "deviceId")
    {
      DeviceId = content.substring(1, content.length() - 1);
      setDeviceName();
    }
    if (evt == "devices")
    {
      Devices = JSON.parse(content);
      setDeviceName();
    }
    if (evt == "device_states")
    {
      DeviceStates = JSON.parse(content);
      processTallyData();
    }
    break;
  }
  default:
    break;
  }
}

void saveParamCallback()
{
  String str_taHost = wm.server->hasArg("taHostIP") ? wm.server->arg("taHostIP") : "";
  String str_taPort = wm.server->hasArg("taHostPort") ? wm.server->arg("taHostPort") : "";
  eepromWriteString(ADDR_TAHOST, str_taHost, 40);
  eepromWriteString(ADDR_TAPORT, str_taPort, 6);
}

void connectToNetwork()
{
  WiFi.mode(WIFI_STA);

  gotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &)
                                         { networkConnected = true; });
  disconnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &)
                                                     { networkConnected = false; });

  WiFiManagerParameter custom_taServer("taHostIP", "Tally Arbiter Server", tallyarbiter_host, 40);
  WiFiManagerParameter custom_taPort("taHostPort", "Port", tallyarbiter_port, 6);
  wm.addParameter(&custom_taServer);
  wm.addParameter(&custom_taPort);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setConfigPortalTimeout(120);

  networkConnected = wm.autoConnect(listenerDeviceName.c_str());
}

void connectToServer()
{
  socket.onEvent(socketEvent);
  socket.begin(tallyarbiter_host, atoi(tallyarbiter_port));
}

void setup()
{
  leds.begin();
  leds.setBrightness(BRIGHTNESS);
  setColor(leds.Color(0, 0, 255));

  EEPROM.begin(EEPROM_SIZE);

  listenerDeviceName = "tally-" + String(ESP.getChipId(), HEX);

  String storedId = eepromReadString(ADDR_DEVICEID, 40);
  String storedName = eepromReadString(ADDR_DEVICENAME, 40);
  String storedHost = eepromReadString(ADDR_TAHOST, 40);
  String storedPort = eepromReadString(ADDR_TAPORT, 6);
  if (storedId.length() > 0)
    DeviceId = storedId;
  if (storedName.length() > 0)
    DeviceName = storedName;
  if (storedHost.length() > 0)
    storedHost.toCharArray(tallyarbiter_host, 40);
  if (storedPort.length() > 0)
    storedPort.toCharArray(tallyarbiter_port, 6);

  connectToNetwork();
  while (!networkConnected)
  {
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

void loop()
{
  ArduinoOTA.handle();
  socket.loop();
}
#endif

// ====================================================================
// --- RX CONFIGURATION ESP WEB_SOCKET ---
// ====================================================================

#ifdef MODE_RX_WEBSOCKET_32
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 5
#define NUM_LEDS 1

Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Warna Primer & Sekunder
#define BLACK leds.Color(0, 0, 0)
#define RED leds.Color(255, 0, 0)
#define GREEN leds.Color(0, 255, 0)
#define BLUE leds.Color(0, 0, 255)
#define WHITE leds.Color(255, 255, 255)
#define YELLOW leds.Color(255, 255, 0)
#define CYAN leds.Color(0, 255, 255)
#define MAGENTA leds.Color(255, 0, 255)

// Variasi Warna Populer
#define ORANGE leds.Color(255, 165, 0)
#define PINK leds.Color(255, 192, 203)
#define UNGU leds.Color(128, 0, 128)
#define GOLD leds.Color(255, 215, 0)
#define TOSCA leds.Color(72, 209, 204)
#define LIME leds.Color(50, 205, 50)

// Warna Redup (Hemat Daya & Tidak Silau)
#define REDUP_MERAH leds.Color(32, 0, 0)
#define REDUP_HIJAU leds.Color(0, 32, 0)
#define REDUP_BIRU leds.Color(0, 0, 32)
#define REDUP_PUTIH leds.Color(32, 32, 32)

void handleEvent();

WiFiManagerParameter custom_vmix_ip("vmix_ip", "IP Server WebSocket", "192.168.1.100", 16);
WiFiManagerParameter custom_id("id", "Cam ID", "1", 2);
WiFiManager wm;
WebSocketsClient webSocket;
ArduinoOTAClass ArduinoOTA;

String PGM = None;
String PVW = None;
unsigned long lastReconnectwebSocketAttempt = 0;
char vmix_ip[40] = "192.168.1.100";
const int deviceID = 0;
const int ledPin = 5;
int myID = String(custom_id.getValue()).toInt();

void updateLed(uint32_t color)
{
  leds.setPixelColor(0, color);
  leds.show();
}

void handlewebSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    Serial.println("Terhubung ke Server!");
    break;
  case WStype_BIN:
    handleEvent(payload, length);
    break;
  }
}

void saveConfigUserCallback()
{
  strncpy(vmix_ip, custom_vmix_ip.getValue(), 40);
}

void configWiFiUserModeCallback(WiFiManager *myWiFiManager)
{
  updateLed(UNGU);
}

void handleEvent(uint8_t *payload, size_t length)
{
  if (length == 2) {
    uint8_t PGM = payload[0];
    uint8_t PVW = payload[1];
  }
  if (PGM == myID)
  {
    updateLed(RED);
  }
  else if (PVW == myID)
  {
    updateLed(GREEN);
  }
  else
  {
    updateLed(BLUE);
  }
}

void setup()
{
  leds.begin();
  leds.show();

  wm.addParameter(&custom_vmix_ip);
  wm.setSaveConfigCallback(saveConfigUserCallback);
  wm.setAPCallback(configWiFiUserModeCallback);

  if (!wm.autoConnect("ESP-Portal-WebSocket"))
  {
    updateLed(YELLOW);
    delay(500);
    updateLed(BLACK);
    ESP.restart();
  }

  strncpy(vmix_ip, custom_vmix_ip.getValue(), 40);

  ArduinoOTA.setHostname("ESP32-WebSocket");
  ArduinoOTA.begin();
  webSocket.begin(vmix_ip, 8080, "/");
  webSocket.onEvent(handlewebSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop()
{
  webSocket.loop();
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED)
  {
    updateLed(UNGU);
  }
  else if (!webSocket.isConnected())
  {
    updateLed(YELLOW);
  }
  else
  {
  }
}
#endif