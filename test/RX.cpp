#include <esp_now.h>
#include <WiFi.h>

// Replace with the MAC address of the RX Tally Light board
uint8_t receiverAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
const int tallyPin = 2; // Connected to AVMATRIX GPIO

void setup() {
  pinMode(tallyPin, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  esp_now_init();
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  esp_now_add_peer(&peerInfo);
}

void loop() {
  bool isLive = (digitalRead(tallyPin) == LOW);
  esp_now_send(receiverAddress, (uint8_t *) &isLive, sizeof(isLive));
  delay(100); 
}