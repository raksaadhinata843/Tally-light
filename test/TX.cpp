#include <SPI.h>
#include <RF24.h>

RF24 radio(4, 5); // CE, CSN
const byte address[6] = "TALLY";

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(102);
  radio.stopListening();
  Serial.println("Master Siap, mencoba mengirim...");
}

void loop() {
  const char text[] = "Tally On";
  bool ok = radio.write(&text, sizeof(text));
  
  if (ok) {
    Serial.println("Pesan terkirim sukses!");
  } else {
    Serial.println("Gagal kirim (Cek kabel/koneksi NRF)");
  }
  delay(1000);
}