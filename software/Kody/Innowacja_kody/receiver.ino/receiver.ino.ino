#include <SPI.h>
#include <LoRa.h>

// ---------------- PINY LoRa RA-02 ----------------
#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   21
#define LORA_CS      5
#define LORA_RST    14
#define LORA_DIO0   13

long lastCounter = -1;

void setupRadio()
{
  LoRa.setSpreadingFactor(10);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(12);
  LoRa.enableCrc();
}

void setup()
{
  Serial.begin(115200);

  // Dla ESP32-S3 przez USB czasem warto chwilę poczekać,
  // ale nie blokować programu na zawsze.
  delay(1500);

  Serial.println("=== LoRa RX -> USB Serial GUI ===");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("ERROR,LORA_INIT_FAIL");
    while (true) {
      delay(1000);
    }
  }

  setupRadio();

  Serial.println("STATUS,LORA_RX_READY,433MHz,SF10,BW125kHz,CR4_5");

  LoRa.receive();
}

void loop()
{
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  String payload = "";

  while (LoRa.available()) {
    payload += (char)LoRa.read();
  }

  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  processPacket(payload, rssi, snr);

  LoRa.receive();
}

void processPacket(const String& payload, int rssi, float snr)
{
  // Oczekiwany format:
  // DATA,<counter>,<ts>,<lat>,<lng>,<sats>,<stmBuf>

  if (!payload.startsWith("DATA,")) {
    Serial.print("RAW,");
    Serial.print(rssi);
    Serial.print(",");
    Serial.print(snr, 2);
    Serial.print(",");
    Serial.println(payload);
    return;
  }

  int p[6];

  p[0] = payload.indexOf(',');

  for (int i = 1; i < 6; i++) {
    p[i] = payload.indexOf(',', p[i - 1] + 1);
  }

  for (int i = 0; i < 6; i++) {
    if (p[i] < 0) {
      Serial.print("ERROR,BAD_FORMAT,");
      Serial.println(payload);
      return;
    }
  }

  String type = payload.substring(0, p[0]);
  long counter = payload.substring(p[0] + 1, p[1]).toInt();
  unsigned long ts = payload.substring(p[1] + 1, p[2]).toInt();
  double lat = payload.substring(p[2] + 1, p[3]).toDouble();
  double lng = payload.substring(p[3] + 1, p[4]).toDouble();
  int sats = payload.substring(p[4] + 1, p[5]).toInt();
  String stmData = payload.substring(p[5] + 1);

  bool duplicate = false;
  bool missing = false;
  long expected = lastCounter + 1;

  if (lastCounter >= 0) {
    if (counter == lastCounter) {
      duplicate = true;
    }

    if (counter > lastCounter + 1) {
      missing = true;
    }
  }

  if (!duplicate) {
    lastCounter = counter;
  }

  /*
   * WYJŚCIE DLA GUI
   *
   * Format CSV:
   * GUI,DATA,counter,tx_timestamp_ms,lat,lng,sats,rssi,snr,missing,expected_counter,stm_data
   */

  Serial.print("GUI,DATA,");
  Serial.print(counter);
  Serial.print(",");
  Serial.print(ts);
  Serial.print(",");
  Serial.print(lat, 9);
  Serial.print(",");
  Serial.print(lng, 9);
  Serial.print(",");
  Serial.print(sats);
  Serial.print(",");
  Serial.print(rssi);
  Serial.print(",");
  Serial.print(snr, 2);
  Serial.print(",");
  Serial.print(missing ? 1 : 0);
  Serial.print(",");
  Serial.print(expected);
  Serial.print(",");
  Serial.println(stmData);
}