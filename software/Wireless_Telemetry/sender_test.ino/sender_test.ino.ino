/****************************************************************************
 *  ESP32-S3-DEVKITC-1-N8R8 + RA-02 433 MHz
 *  TESTOWY nadajnik LoRa
 *
 *  Bez GPS
 *  Bez UART do STM32
 *
 *  Format ramki zgodny z wersją docelową:
 *  DATA,counter,timestamp,lat,lng,sats,telemetry
 ***************************************************************************/

#include <SPI.h>
#include <LoRa.h>

// ---------------- PINY LoRa ----------------
#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   21
#define LORA_CS      5
#define LORA_RST    14
#define LORA_DIO0   13

// ---------------- GPS WYŁĄCZONY ----------------
// #include <TinyGPS++.h>
// constexpr uint8_t GPS_RX_PIN = 16;
// constexpr uint8_t GPS_TX_PIN = 17;
// TinyGPSPlus gps;
// HardwareSerial SerialGPS(2);

// ---------------- STM32 UART WYŁĄCZONY ----------
// constexpr uint8_t STM_RX_PIN = 38;
// constexpr uint8_t STM_TX_PIN = 40;
// HardwareSerial SerialSTM(1);

// ---------------- ZMIENNE GLOBALNE --------------
uint32_t counter      = 0;
uint32_t lastTxTimeMs = 0;

constexpr uint32_t TX_INTERVAL_MS = 2500;

// Dane testowe GPS
double testLat = 52.406374000;
double testLng = 16.925168000;
int    testSats = 8;

// Bufor telemetrii testowej
char stmBuf[128] = {0};

void setupRadio()
{
  LoRa.setSpreadingFactor(10);              // SF10
  LoRa.setSignalBandwidth(125E3);           // 125 kHz
  LoRa.setCodingRate4(5);                   // CR 4/5
  LoRa.setPreambleLength(12);
  LoRa.enableCrc();

  // RA-02 używa toru PA_BOOST.
  // Uwaga: 20 dBm wymaga odpowiedniego zasilania modułu.
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
}

void setup()
{
  Serial.begin(115200);

  // Na ESP32-S3 z USB CDC to może blokować, jeżeli nie ma monitora portu.
  // Dlatego daję timeout zamiast nieskończonego oczekiwania.
  uint32_t serialStart = millis();
  while (!Serial && millis() - serialStart < 3000) {
    delay(10);
  }

  Serial.println();
  Serial.println("=== START: TEST Sender LoRa ===");
  Serial.println("GPS: disabled");
  Serial.println("STM32 UART: disabled");
  Serial.println("Mode: generated test frames");

  // ---------------- GPS WYŁĄCZONY ----------------
  // SerialGPS.begin(38400, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  // Serial.println("SerialGPS (UART2) on RX=16, TX=17");

  // ---------------- STM32 UART WYŁĄCZONY ----------
  // SerialSTM.begin(115200, SERIAL_8N1, STM_RX_PIN, STM_TX_PIN);
  // SerialSTM.setTimeout(10);
  // Serial.println("SerialSTM (UART1) on RX=38, TX=40");

  // ---------------- LoRa + SPI ----------------
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("!! LoRa init failed !!");
    while (true) {
      delay(1000);
    }
  }

  setupRadio();

  Serial.println("LoRa TX Ready");
  Serial.println("Frequency: 433 MHz");
  Serial.println("SF: 10");
  Serial.println("BW: 125 kHz");
  Serial.println("CR: 4/5");
  Serial.println("TX power: 20 dBm");
  Serial.println("--------------------------------");

  counter = 0;
}

void loop()
{
  // ---------------- GPS WYŁĄCZONY ----------------
  /*
  while (SerialGPS.available()) {
    char c = SerialGPS.read();
    if (gps.encode(c) && gps.location.isUpdated()) {
      Serial.printf("GPS -> lat=%.6f, lng=%.6f\n",
                    gps.location.lat(), gps.location.lng());
    }
  }
  */

  // ---------------- STM32 UART WYŁĄCZONY ----------
  /*
  if (SerialSTM.available()) {
    int len = SerialSTM.readBytesUntil('\n', stmBuf, sizeof(stmBuf) - 1);
    if (len > 0) {
      stmBuf[len] = '\0';
      if (stmBuf[len - 1] == '\r') stmBuf[len - 1] = '\0';
      Serial.printf("STM32 -> \"%s\"\n", stmBuf);
    }
  }
  */

  // ---------------- Wysyłka LoRa co 2,5 s ----------
  if (millis() - lastTxTimeMs < TX_INTERVAL_MS) {
    return;
  }

  lastTxTimeMs = millis();

  counter = (counter + 1) % 1000;

  uint32_t ts = millis();

  // Symulowana telemetria z lokomotywy / STM32
  float speed = 10.0 + (counter % 20) * 0.5;
  float batt  = 24.0 + (counter % 10) * 0.1;
  float temp  = 30.0 + (counter % 15) * 0.2;

  snprintf(stmBuf, sizeof(stmBuf),
           "STM_TEST,speed=%.1f,batt=%.1f,temp=%.1f",
           speed, batt, temp);

  // Format zgodny z wersją docelową:
  // DATA,counter,timestamp,lat,lng,sats,telemetry
  char buf[256];

  int len = snprintf(buf, sizeof(buf),
                     "DATA,%lu,%lu,%.9f,%.9f,%d,%s",
                     static_cast<unsigned long>(counter),
                     static_cast<unsigned long>(ts),
                     testLat,
                     testLng,
                     testSats,
                     stmBuf);

  if (len <= 0 || len >= static_cast<int>(sizeof(buf))) {
    Serial.println("!! Payload build error / buffer overflow risk !!");
    return;
  }

  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<uint8_t*>(buf), len);
  LoRa.endPacket();

  Serial.printf("TX -> %s\n", buf);
}
