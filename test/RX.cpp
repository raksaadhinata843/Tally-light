#include <SPI.h>
#include <RF24.h>

RF24 radio(4, 5); // CE, CSN
const byte address[6] = "TALLY";

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(102);
  radio.startListening();
  Serial.println("CLient Siap, menunggu pesan...");
}

void loop() {
  if (radio.available()) {
    char text[32];
    radio.read(&text, sizeof(text));
    Serial.print("Pesan diterima: ");
    Serial.println(text);
  }
}