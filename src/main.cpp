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
  #define RED 5
  #define YELLOW 4
  #define GREEN 2
  volatile bool triggered = false;

  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    
    Serial.print("Data received from: ");
    for (int i = 0; i < 6; i++) {
      Serial.print(mac[i], HEX);
      if (i < 5) Serial.print(":");
    }
    uint8_t receivedStatus = incomingData[0];
    
    if (receivedStatus == 0) {
      triggered = 0;
    } else if (receivedStatus == 1) {
      triggered = 1;
    } else {
      triggered = 2;
    }
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
  sendData(0);
  delay(2000); 

  sendData(1);
  delay(2000);

  sendData(2);
  delay(2000);
}
#endif

#ifdef IS_RX
  // Masukkan kode RX Anda di sini
  void setup() {
  Serial.begin(115200);
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  WiFi.mode(WIFI_STA);
  wifi_set_channel(1);
  delay(100); // Beri waktu untuk WiFi stabil
  Serial.println("Receiver initializing...");
  Serial.println(WiFi.macAddress());

  
  if (esp_now_init() != 0) return;
  Serial.println("ESP-NOW initialized successfully.");
  
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Receiver ready, waiting for data...");
}
  void loop() {
  if (triggered == 0) {
    digitalWrite(RED, HIGH);
    Serial.println("LED ON");
    delay(1000);
    digitalWrite(RED, LOW);
  } else if (triggered == 1) {
    digitalWrite(YELLOW, HIGH);
    delay(1000);
    digitalWrite(YELLOW, LOW);
  } else {
    digitalWrite(GREEN, HIGH);
    delay(1000);
    digitalWrite(GREEN, LOW);
  }
}
#endif