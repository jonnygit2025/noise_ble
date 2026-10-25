#include <Arduino.h>
#ifndef SIMULATION_MODE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#endif

// GPIO4/5 avoid GPIO2 boot-strap; physical XIAO: D2=GPIO4, D3=GPIO5.
constexpr uint8_t LED_PIN = 4;
constexpr uint8_t BUZZER_PIN = 5;
constexpr char DEVICE_NAME[] = "NasoClown-Halloween";
constexpr char SERVICE_UUID[] = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
constexpr char CHARACTERISTIC_UUID[] = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

#ifndef SIMULATION_MODE
QueueHandle_t commandQueue = nullptr;
void enqueueCommand(char command) {
  if (commandQueue) xQueueSend(commandQueue, &command, 0);
}

class ServerCallbacks final : public BLEServerCallbacks {
  void onDisconnect(BLEServer* server) override {
    // Fail-safe: turn everything off after disconnect.
    enqueueCommand('0');
    BLEDevice::startAdvertising();
  }
};

class CharacteristicCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    // Ignore empty BLE writes. Only first byte is a command.
    std::string payload = characteristic->getValue();
    if (!payload.empty()) enqueueCommand(payload[0]);
  }
};
#endif

enum class Effect : uint8_t { None, HonkLow, HonkHigh, StrobeOn, StrobeOff };
Effect effect = Effect::None;
uint32_t nextTransition = 0;
uint8_t flashes = 0;
bool ledIsOn = false;

void setLed(bool on) {
  ledIsOn = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void stopSound() { noTone(BUZZER_PIN); }

void cancelEffect() {
  effect = Effect::None;
  stopSound();
}

void startHonk(uint32_t now) {
  cancelEffect();
  tone(BUZZER_PIN, 440);
  effect = Effect::HonkLow;
  nextTransition = now + 120;
}

void startStrobe(uint32_t now) {
  cancelEffect();
  flashes = 0;
  setLed(true);
  tone(BUZZER_PIN, 1200);
  effect = Effect::StrobeOn;
  nextTransition = now + 40;
}

void processCommand(char command, uint32_t now) {
  switch (command) {
    case '0': // Emergency stop; LED and audio off
      cancelEffect();
      setLed(false);
      break;
    case '1': // Steady light and horn
      setLed(true);
      startHonk(now);
      break;
    case 'H':
    case 'B': // Horn only, preserve light
      startHonk(now);
      break;
    case 'L': // Toggle light (and cancel any ongoing effect)
      cancelEffect();
      setLed(!ledIsOn);
      break;
    case 'S': // 6 short flashes, end with LED off
      startStrobe(now);
      break;
    default:
      Serial.printf("[WARN] Unknown command: %c\n", command);
      break;
  }
}

void updateEffect(uint32_t now) {
  if (effect == Effect::None || (int32_t)(now - nextTransition) < 0) return;
  switch (effect) {
    case Effect::HonkLow:
      tone(BUZZER_PIN, 880);
      effect = Effect::HonkHigh;
      nextTransition = now + 200;
      break;
    case Effect::HonkHigh:
      stopSound();
      effect = Effect::None;
      break;
    case Effect::StrobeOn:
      stopSound();
      setLed(false);
      effect = Effect::StrobeOff;
      nextTransition = now + 40;
      break;
    case Effect::StrobeOff:
      ++flashes;
      if (flashes >= 6) {
        effect = Effect::None;
      } else {
        setLed(true);
        tone(BUZZER_PIN, 1200);
        effect = Effect::StrobeOn;
        nextTransition = now + 40;
      }
      break;
    case Effect::None:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setLed(false);
  stopSound();
#ifndef SIMULATION_MODE
  commandQueue = xQueueCreate(12, sizeof(char));
  if (!commandQueue) {
    Serial.println("[ERROR] No memory for BLE command queue");
    return;
  }
  BLEDevice::init(DEVICE_NAME);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService* service = server->createService(SERVICE_UUID);
  BLECharacteristic* characteristic = service->createCharacteristic(
    CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  characteristic->setCallbacks(new CharacteristicCallbacks());
  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();
  Serial.println("[BLE] Ready: NasoClown-Halloween");
#else
  Serial.println("[SIMULATION] BLE disabled; use serial commands.");
#endif
  Serial.println("Commands: 1=light+horn 0=off H/B=horn L=toggle-light S=strobe");
}

void loop() {
  // Serial works on hardware and in Wokwi.
  while (Serial.available()) {
    char command = static_cast<char>(Serial.read());
    if (command != '\n' && command != '\r') processCommand(command, millis());
  }
#ifndef SIMULATION_MODE
  char command;
  while (commandQueue && xQueueReceive(commandQueue, &command, 0) == pdTRUE) {
    processCommand(command, millis());
  }
#endif
  updateEffect(millis());
  delay(1);
}
