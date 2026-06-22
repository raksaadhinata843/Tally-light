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
  #define RED 1
  #define YELLOW 4
  #define GREEN 2

  int triggered = 0;

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
  if (Serial.available() > 0) {
    // Read the full string sent by Python
    String command = Serial.readStringUntil('\n');
    
    // Example format expected: "1" (Active), "2" (Preview), "0" (Off)
    // We only need the first character if you are sending "1", "2", or "0"
    if (command.length() > 0) {
      uint8_t dataToSend = command.charAt(0) - '0';
      
      // Send via ESP-NOW
      esp_now_send(rxAddress, &dataToSend, sizeof(dataToSend));
      
      // Optional: Debugging
      Serial.print("Sending to RX: ");
      Serial.println(dataToSend);
    }
  }
}
#endif

#ifdef IS_RX
  // Masukkan kode RX Anda di sini
  void setup() {
  Serial.begin(115200);

  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  digitalWrite(RED, LOW);
  digitalWrite(YELLOW, LOW);  
  digitalWrite(GREEN, LOW);
  
  WiFi.mode(WIFI_STA);
  wifi_set_channel(1);
  delay(100); 
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) return;
  
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

  void loop() {
  // Directly control LEDs based on the 'triggered' variable
  // No delays! This makes it instant.
  digitalWrite(RED, (triggered == 1) ? HIGH : LOW);
  digitalWrite(YELLOW, (triggered == 0) ? HIGH : LOW);
  digitalWrite(GREEN, (triggered == 2) ? HIGH : LOW);
}
#endif