#include <Arduino.h>

// Common Structure
typedef struct __attribute__((packed)) struct_message {
    int32_t cameraID;
    int32_t state;
} struct_message;

struct_message myData;

// --- TX CONFIGURATION (ESP32) ---
#ifdef MODE_TX
  #include <esp_now.h>
  #include <WiFi.h>
  uint8_t rxAddress[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D}; 

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
#ifdef MODE_RX
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