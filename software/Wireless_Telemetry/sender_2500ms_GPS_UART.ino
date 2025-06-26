/****************************************************************************
 *  ESP32-S3-DevKitC-1-N8R8  +  RA-02 433 MHz  +  NEO-M9N GPS
 *  Nadajnik danych GPS + telemetria STM32 wysyłany przez LoRa
 *
 *  >>> WERSJA POPRAWIONA 2025-06-24 <<<
 ***************************************************************************/

#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>

// ---------------- PINY LoRa ----------------
#define LORA_SCK    18   // VSPI_SCK
#define LORA_MISO   19   // VSPI_MISO
#define LORA_MOSI   21   // VSPI_MOSI
#define LORA_CS      5
#define LORA_RST    14
#define LORA_DIO0   13

/* ---------------- PINY GPS (UART2) ----------------
 * GPS TX → ESP32 GPIO16 (RX2)
 * GPS RX → ESP32 GPIO17 (TX2)
 */
constexpr uint8_t GPS_RX_PIN = 16;
constexpr uint8_t GPS_TX_PIN = 17;
TinyGPSPlus gps;
HardwareSerial SerialGPS(2);               // UART2 = RX2/TX2

/* ---------------- PINY STM32 (UART1) --------------
 * STM TX → ESP32 GPIO38 (RX1)
 * STM RX → ESP32 GPIO40 (TX1)
 */
constexpr uint8_t STM_RX_PIN = 38;
constexpr uint8_t STM_TX_PIN = 40;
HardwareSerial SerialSTM(1);               // UART1 = RX1/TX1

/* ---------------- ZMIENNE GLOBALNE -------------- */
char  stmBuf[128] = {0};                   // *** ZMIANA *** inicjalizacja
uint32_t counter      = 0;
uint32_t lastTxTimeMs = 0;
constexpr uint32_t TX_INTERVAL_MS = 2500;  // *** ZMIANA *** ramka co 2,5 s

void setupRadio() {
    LoRa.setSpreadingFactor(10);         // SF10  (fits 2.5 s slot)
    LoRa.setSignalBandwidth(125E3);      // 125 kHz
    LoRa.setCodingRate4(5);              // CR 4/5
    LoRa.setPreambleLength(12);
    LoRa.enableCrc();
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);   // PA-BOOST path on RA-02
}


/* =================================================
 *  SETUP
 * ================================================= */
void setup()
{
  /* USB debug (CDC-USB, nie współdzieli GPIO) */
  Serial.begin(115200);
  while (!Serial) { }                           // poczekaj na port szeregowy
  Serial.println("\n=== START: Sender GPS + LoRa + STM32 ===");

  /* GPS 38400 Bd */
  SerialGPS.begin(38400, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("SerialGPS (UART2) on RX=16, TX=17");

  /* STM32 telemetry 115200 Bd */
  SerialSTM.begin(115200, SERIAL_8N1, STM_RX_PIN, STM_TX_PIN);
  SerialSTM.setTimeout(10);                     // brak blokowania petli
  Serial.println("SerialSTM (UART1) on RX=38, TX=40");

  /* LoRa + SPI */
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("!! LoRa init failed !!");
    while (true) yield();
  }
  setupRadio();
  Serial.println("LoRa TX Ready (433 MHz, SF10, 10 dBm)");

  counter = 0;
}

void loop() {
  /* ---------- I. Odczyt i dekodowanie GPS ---------- */
  while (SerialGPS.available()) {
    char c = SerialGPS.read();
    if (gps.encode(c) && gps.location.isUpdated()) {
      Serial.printf("GPS → lat=%.6f, lng=%.6f",
                    gps.location.lat(), gps.location.lng());
      if (gps.satellites.isValid())
        Serial.printf(", sats=%d\n", gps.satellites.value());
      else
        Serial.println(", sats=---");
    }
  } 

  /* ---------- II. Odczyt jednej linii z STM32 ------ */
  if (SerialSTM.available()) {
    int len = SerialSTM.readBytesUntil('\n', stmBuf, sizeof(stmBuf) - 1);
    if (len > 0) {
      stmBuf[len] = '\0';                          // zakończ C-string
      if (stmBuf[len - 1] == '\r') stmBuf[len - 1] = '\0';
      Serial.printf("STM32 → \"%s\"\n", stmBuf);
    }
  }

  /* ---------- III. Wysyłka LoRa co 2,5 s ---------- */
  if (millis() - lastTxTimeMs < TX_INTERVAL_MS) return;
  lastTxTimeMs = millis();

  /* licznik modulo 1000 */
  counter = (counter + 1) % 1000;

  /* aktualne dane GPS (0 = brak fixu) */
  double lat  = gps.location.isValid() ? gps.location.lat() : 0.0;
  double lng  = gps.location.isValid() ? gps.location.lng() : 0.0;
  int    sats = gps.satellites.isValid() ? gps.satellites.value() : 0;
  uint32_t ts = millis();

  /* zbuduj payload */
  char buf[256];
  int  len = snprintf(buf, sizeof(buf),
                      "DATA,%lu,%lu,%.9f,%.9f,%d,%s",
                      static_cast<unsigned long>(counter),
                      static_cast<unsigned long>(ts),
                      lat, lng, sats,
                      stmBuf);

  /* nadaj przez LoRa */
  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<uint8_t*>(buf), len);
  LoRa.endPacket();

  /*  debug print  */
  Serial.printf("→ TX  cnt=%lu ts=%lu ms lat=%.9f lng=%.9f sats=%d uart=\"%s\"\n",
                static_cast<unsigned long>(counter),
                static_cast<unsigned long>(ts),
                lat, lng, sats,
                stmBuf[0] ? stmBuf : "<none>");

  stmBuf[0] = '\0'; 
}

