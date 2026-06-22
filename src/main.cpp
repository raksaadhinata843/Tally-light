#include <Arduino.h>


#ifdef IS_TX
  #include <esp_now.h>
  #include <WiFi.h>
  uint8_t rxAddress[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D}; 

  bool sendData(bool status) {
    uint8_t dataToSend = status ? 1 : 0;
    return esp_now_send(rxAddress, &dataToSend, sizeof(dataToSend)) == ESP_OK;
  }

  typedef struct __attribute__((packed)) struct_message {
    int32_t cameraID;
    int32_t state;
} struct_message;

  struct_message myData;
#endif

#ifdef IS_RX
  #include <ESP8266WiFi.h>
  #include <espnow.h>
  #define RED 5
  #define BLUE 14
  #define GREEN 4

  int triggered = 0;
  typedef struct __attribute__((packed)) struct_message {
    int32_t cameraID;
    int32_t state;
} struct_message;

  struct_message myData;

 void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    memcpy(&myData, incomingData, sizeof(myData));
    
    // Now you know exactly what to do:
    Serial.print("Camera: "); Serial.print(myData.cameraID);
    Serial.print(" State: "); Serial.println(myData.state);
    
    if (myData.cameraID == 3) { 
        triggered = myData.state;
    }
  }
#endif

#ifdef IS_TX
  // Masukkan kode TX Anda di sini
  void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Mendaftarkan peer (Target RX)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, rxAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}
  void loop() {
  // Inside TX loop()
if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Format: "ID:State" e.g., "1:1"
    int separatorIndex = command.indexOf(':');
    if (separatorIndex > 0) {
        myData.cameraID = command.substring(0, separatorIndex).toInt();
        myData.state = command.substring(separatorIndex + 1).toInt();
        
        esp_now_send(rxAddress, (uint8_t *) &myData, sizeof(myData));
        Serial.print("Sent ID: "); Serial.print(myData.cameraID);
        Serial.print(" State: "); Serial.println(myData.state);
    }
  }
}
#endif

#ifdef IS_RX
  // Masukkan kode RX Anda di sini
  void setup() {
  Serial.begin(115200);

  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  digitalWrite(RED, LOW);
  digitalWrite(BLUE, LOW);  
  digitalWrite(GREEN, LOW);
  
  WiFi.mode(WIFI_STA);
  delay(100); 
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  wifi_set_channel(1);
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

  void loop() {
  // Directly control LEDs based on the 'triggered' variable
  // No delays! This makes it instant.
  digitalWrite(RED, (triggered == 1) ? HIGH : LOW);
  digitalWrite(BLUE, (triggered == 0) ? HIGH : LOW);
  digitalWrite(GREEN, (triggered == 2) ? HIGH : LOW);
}
#endif