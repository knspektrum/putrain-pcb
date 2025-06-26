#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  21
#define CS_PIN      5
#define RST_PIN    14
#define DIO0_PIN   11
String last_packet = "";

void setup() {
  Serial.begin(115200);
  while (!Serial);
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, CS_PIN);
  LoRa.setPins(CS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed");
    while (1) yield();
  }
  setupRadio();
  Serial.println("Repeater Ready (433 MHz)");
  LoRa.receive();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  // Odbieramy cały payload (np. "DATA,42,237084,50.062339,19.940288,8")
  String buf;
  while (LoRa.available()) buf += (char)LoRa.read();
  int rssi = LoRa.packetRssi();

  if (buf != last_packet) {
    Serial.printf("← RX pkt: \"%s\"   RSSI=%d dBm\n", buf.c_str(), rssi);
  // Nadajemy dokładnie ten sam ciąg dalej
  LoRa.beginPacket();
  LoRa.print(buf);
  LoRa.endPacket();
  Serial.println("→ Re-TX");
  }
  else Serial.println("Packet repetition.\n");
  last_packet = buf;
  LoRa.receive();
}
void setupRadio()
{
    LoRa.setSpreadingFactor(10);     // SF10  (range ↑)
    LoRa.setSignalBandwidth(125E3);  // 125 kHz keeps sensitivity high
    LoRa.setCodingRate4(5);          // 4/5   (leave it, CR 4/8 would double airtime)
    LoRa.setPreambleLength(12);      // a bit longer → better sync in weak SNR
    LoRa.enableCrc();                // 1 B overhead, worth it
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN); // 14 dBm ≈ 25 mW ERP (legal in EU433)
}
