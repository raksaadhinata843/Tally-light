#include <Arduino.h>


#ifdef IS_TX
  #include <esp_now.h>
  #include <WiFi.h>
  uint8_t rxAddress[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D}; 

  bool sendData(bool status) {
    uint8_t dataToSend = status ? 1 : 0;
    return esp_now_send(rxAddress, &dataToSend, sizeof(dataToSend)) == ESP_OK;
  }
#endif

#ifdef IS_RX
  #include <ESP8266WiFi.h>
  #include <espnow.h>
  #define LED_PIN 5
  volatile bool triggered = false;

  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    triggered = true;
  }
#endif


#ifdef IS_TX
  // Masukkan kode TX Anda di sini
  void setup() {
  WiFi.mode(WIFI_STA);
  esp_now_init();

  // Mendaftarkan peer (Target RX)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, rxAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}
  void loop() {
  // Mengirim data status "true" setiap 2 detik
  sendData(true);
  
  delay(2000); 
}
#endif

#ifdef IS_RX
  // Masukkan kode RX Anda di sini
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
#endif