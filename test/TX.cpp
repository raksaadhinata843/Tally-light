#include <esp_now.h>
#include <WiFi.h>

// Replace these with the actual MAC addresses of your 4 TX boards
uint8_t txAddresses[][6] = {
  {0x24, 0x6F, 0x28, 0xAA, 0xAA, 0xAA}, // TX 1
  {0x24, 0x6F, 0x28, 0xBB, 0xBB, 0xBB}, // TX 2
  {0x24, 0x6F, 0x28, 0xCC, 0xCC, 0xCC}, // TX 3
  {0x24, 0x6F, 0x28, 0xDD, 0xDD, 0xDD}  // TX 4
};

const int ledPin = 5;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  bool isLive = *incomingData;
  digitalWrite(ledPin, isLive ? HIGH : LOW);
}

void setup() {
  pinMode(ledPin, OUTPUT);
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {}