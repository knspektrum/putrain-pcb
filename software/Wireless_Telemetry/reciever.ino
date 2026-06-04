#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   21
#define CS_PIN       5
#define RST_PIN     14
#define DIO0_PIN    13

constexpr uint8_t UART1_RX_PIN = 38;
constexpr uint8_t UART1_TX_PIN = 40;
constexpr uint32_t UART1_BAUD = 115200;

HardwareSerial SerialUART1(1);

unsigned long lastPingMs = 0;
uint32_t pingCounter = 0;

static void pollUart1() {
  while (SerialUART1.available()) {
    int c = SerialUART1.read();
    if (c >= 0) {
      Serial.printf("UART1 <- %c\n", static_cast<char>(c));
    }
  }
}

void setup() {
  Serial.begin(115200);
  unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 2000) { }

  SerialUART1.begin(UART1_BAUD, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
  SerialUART1.setTimeout(10);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, CS_PIN);
  LoRa.setPins(CS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init fail");
    while (1) yield();
  }
  setupRadio();
  Serial.println("STATUS,RX_READY,433MHz");
  LoRa.receive();
}

void loop() {
  pollUart1();

  if (millis() - lastPingMs >= 1000UL) {
    lastPingMs = millis();
    ++pingCounter;
    char buf[32];
    snprintf(buf, sizeof(buf), "PING,%lu", static_cast<unsigned long>(pingCounter));
    SerialUART1.print(buf);
    SerialUART1.print("\r\n");
    Serial.printf("UART1 -> %s\n", buf);
  }

  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  while (LoRa.available()) {
    Serial.write(static_cast<uint8_t>(LoRa.read()));
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
