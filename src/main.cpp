#include <Arduino.h>

// Common Structure
typedef struct __attribute__((packed)) struct_message {
    int32_t cameraID;
    int32_t state;
} struct_message;

struct_message myData;

// --- TX CONFIGURATION (ESP32) ---
#ifdef MODE_TX_ESP32
  #include <esp_now.h>
  #include <WiFi.h>

  if 
  uint8_t rxAddress[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D};
  else
  const uint8_t PGM_PINS[6] = {A3, A2, A1, A0, 4, 3};
  const uint8_t PVW_PINS[6] = {1, 5, 6, 7, 8, 2};

  void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
      return;
    }
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, rxAddress, 6);
    peerInfo.channel = 1;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }

  void loop() {
    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      int separatorIndex = command.indexOf(':');
      if (separatorIndex > 0) {
        myData.cameraID = command.substring(0, separatorIndex).toInt();
        myData.state = command.substring(separatorIndex + 1).toInt();
        esp_now_send(rxAddress, (uint8_t *) &myData, sizeof(myData));
      }
    }
  }
#endif

// --- RX CONFIGURATION (ESP8266) ---
#ifdef MODE_RX_ESP8266 
  #include <ESP8266WiFi.h>
  #include <espnow.h>
  #define RED 5
  #define BLUE 14
  #define GREEN 4

  int triggered = 0;
  unsigned long lastRecvTime = 0;

  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    memcpy(&myData, incomingData, sizeof(myData));
    if (myData.cameraID == 1) { 
        triggered = myData.state;
    }
    else if (myData.cameraID == 2) {
        triggered = myData.state;
    }
    else if (myData.cameraID == 3) {
        triggered = myData.state;
    }
  }

  void setup() {
    Serial.begin(115200);
    pinMode(RED, OUTPUT); pinMode(BLUE, OUTPUT); pinMode(GREEN, OUTPUT);
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0) return;
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
  }

  void loop() {
    digitalWrite(RED, (triggered == 1) ? HIGH : LOW);
    digitalWrite(BLUE, (triggered == 0) ? HIGH : LOW);
    digitalWrite(GREEN, (triggered == 2) ? HIGH : LOW);
  }
#endif

// --- RX CONFIGURATION (ESP32) ---
#ifdef MODE_RX_ESP32
  #include <WiFi.h>
  #include <esp_now.h>
  #define RED 23
  #define GREEN 22
  #define BLUE 21

  int triggered = 0;
  unsigned long lastRecvTime = 0;

  void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len >= sizeof(myData)) {
    memcpy(&myData, incomingData, sizeof(myData));
  }
}

  void setup() {
    Serial.begin(115200);
    pinMode(RED, OUTPUT); pinMode(BLUE, OUTPUT); pinMode(GREEN, OUTPUT);
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0) return;
  }

  void loop() {
    analogWrite(RED, (triggered == 1) ? HIGH : LOW);
    analogWrite(BLUE, (triggered == 0) ? HIGH : LOW);
    analogWrite(GREEN, (triggered == 2) ? HIGH : LOW);
  }
#endif