#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   21
#define CS_PIN       5
#define RST_PIN     14
#define DIO0_PIN    13

long lastCounter = -1;



void setup() {
  Serial.begin(115200);
  while (!Serial);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, CS_PIN);
  LoRa.setPins(CS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init fail");
    while (1) yield();
  }
  setupRadio();
  Serial.println("\nLoRa RX Ready (433 MHz)");
  LoRa.receive();
  
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  String payload;
  while (LoRa.available()) payload += (char)LoRa.read();
  int16_t rssi = LoRa.packetRssi();

  // Jeśli to żądanie retransmisji (REQ,<n>), wypisujemy i nie ignorujemy
  if (payload.startsWith("REQ,")) {
    long reqCnt = payload.substring(4).toInt();
    Serial.printf("← RX REQ: cnt=%ld   RSSI=%d dBm\n", reqCnt, rssi);
    // Tu możesz dodać logikę, żeby wysłać do nadawcy / repeatera potwierdzenie
    LoRa.receive();
    return;
  }

  // Zakładamy: "DATA,<counter>,<ts>,<lat>,<lng>,<sats>"
  // Rozbijamy łańcuch po przecinkach:
  int p[7];                      // we need 7 comma positions now
  p[0] = payload.indexOf(',');
  for (int i = 1; i < 6; i++) {  // <-- NOTE: 6 instead of 5
    p[i] = payload.indexOf(',', p[i - 1] + 1);
  }

  String type      = payload.substring(0, p[0]);       // "DATA"
  long   counter   = payload.substring(p[0]+1, p[1]).toInt();
  unsigned long ts = payload.substring(p[1]+1, p[2]).toInt();
  double lat       = payload.substring(p[2]+1, p[3]).toDouble();
  double lng       = payload.substring(p[3]+1, p[4]).toDouble();
  int    sats      = payload.substring(p[4]+1, p[5]).toInt();
  String stmLine   = payload.substring(p[5]+1);        // everything after 6-th comma


  if (type == "DATA") {
    // 1) ignoruj stare lub duplikaty
    if (counter <= lastCounter && counter > lastCounter - 7) {
      Serial.printf("IGNOR DUPL cnt=%ld (last=%ld)\n", counter, lastCounter);
    }
    else {
      // 2) jeśli przeskok większy niż +1, żądaj retransmisji
      if (lastCounter >= 0 && counter > lastCounter + 1) {
        long missing = lastCounter + 1;
        Serial.printf("MISSING %ld → REQ\n", missing);
        requestRetransmit(missing);
      }
      // 3) wyświetl nowy pakiet
      Serial.printf(
        "NEW PKT: cnt=%ld ts=%lums lat=%.6f lng=%.6f sats=%d "
        "RSSI=%d dBm  STM=[%s]\n",
        counter, ts, lat, lng, sats, rssi, stmLine.c_str());
      lastCounter = counter;
    }
  }

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

void requestRetransmit(long missingCounter) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "REQ,%ld", missingCounter);
  LoRa.beginPacket();
  LoRa.write((uint8_t*)buf, len);
  LoRa.endPacket();
  Serial.printf("→ TX REQ for cnt=%ld\n", missingCounter);
  LoRa.receive();
}
