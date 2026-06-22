#include <ESP8266WiFi.h>
#include <espnow.h>
#define LED_PIN 5

// Replace with the MAC address of the TX board
uint8_t mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 

volatile bool triggered = false; // Flag untuk loop

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  triggered = true; // Hanya set flag
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  WiFi.mode(WIFI_STA);
  wifi_set_channel(1); 
  
  if (esp_now_init() != 0) return;
  
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (triggered) {
    digitalWrite(LED_PIN, HIGH);
    delay(100); 
    digitalWrite(LED_PIN, LOW);
    triggered = false;
  }
}