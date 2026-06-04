#include <SPI.h>
#include <LoRa.h>

// ---------------- PINY LoRa ----------------
#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   21
#define LORA_CS      5
#define LORA_RST    14
#define LORA_DIO0   13

/* ---------------- PINY STM32 (UART1) --------------
 * STM TX → ESP32 GPIO38 (RX1)
 * STM RX → ESP32 GPIO40 (TX1)
 */
constexpr uint8_t STM_RX_PIN = 38;
constexpr uint8_t STM_TX_PIN = 40;
HardwareSerial SerialSTM(1);               // UART1 = RX1/TX1

/* ---------------- ZMIENNE GLOBALNE -------------- */
char lineBuf[128] = {0};
uint32_t pingCounter = 0;
unsigned long lastPingMs = 0;

bool readTelemetryLine(Stream &port, char *line, size_t lineSize) {
    if (!port.available()) return false;

    int len = port.readBytesUntil('\n', line, lineSize - 1);
    if (len <= 0) return false;

    line[len] = '\0';
    if (line[len - 1] == '\r') {
        line[len - 1] = '\0';
    }
    return true;
}

void relayLineToLoRa(const char *line) {
    if (!line || !line[0]) return;

    LoRa.beginPacket();
    LoRa.write(reinterpret_cast<const uint8_t *>(line), strlen(line));
    LoRa.write('\n');
    LoRa.endPacket();
}

void echoReceivedLine(const char *source, const char *line) {
    if (!line || !line[0]) return;
    Serial.printf("%s %s\n", source, line);
}

void sendLoRaPing() {
    if (millis() - lastPingMs < 1000UL) return;
    lastPingMs = millis();
    ++pingCounter;

    char pingLine[32];
    snprintf(pingLine, sizeof(pingLine), "PING,%lu", static_cast<unsigned long>(pingCounter));

    LoRa.beginPacket();
    LoRa.write(reinterpret_cast<const uint8_t *>(pingLine), strlen(pingLine));
    LoRa.write('\n');
    LoRa.endPacket();

    Serial.printf("LoRa -> %s\n", pingLine);
}

void pollTelemetryInputs() {
    char line[128];

    if (readTelemetryLine(Serial, line, sizeof(line))) {
        echoReceivedLine("USB <-", line);
        relayLineToLoRa(line);
    }

    if (readTelemetryLine(SerialSTM, line, sizeof(line))) {
        echoReceivedLine("UART1 <-", line);
        relayLineToLoRa(line);
    }
}

void setupRadio() {
    LoRa.setSpreadingFactor(10);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.setPreambleLength(12);
    LoRa.enableCrc();
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);
  unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 2000) { }

  SerialSTM.begin(115200, SERIAL_8N1, STM_RX_PIN, STM_TX_PIN);
  SerialSTM.setTimeout(10);                     // brak blokowania petli

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed");
    while (true) yield();
  }
  setupRadio();
  Serial.println("STATUS,TX_READY,433MHz");
}

void loop() {
  pollTelemetryInputs();
  sendLoRaPing();

  delay(1);
}
