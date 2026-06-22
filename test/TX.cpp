#include <esp_now.h>
#include <WiFi.h>

// MAC Address RX (NodeMCU V3)
uint8_t rxAddress[] = {0x24, 0xD7, 0xEB, 0xCD, 0x27, 0x3D}; 

// Fungsi khusus pengiriman data
bool sendData(bool status) {
  uint8_t dataToSend = status ? 1 : 0;
  
  // esp_now_send langsung mengirim data ke alamat MAC target
  esp_err_t result = esp_now_send(rxAddress, &dataToSend, sizeof(dataToSend));
  
  return (result == ESP_OK); // Mengembalikan true jika sukses, false jika gagal
}

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