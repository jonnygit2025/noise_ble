#include <Arduino.h>
#include <driver/ledc.h>
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
// Buzzer "-" pin. Single mode: held LOW (acts as GND).
// Push-pull mode: driven in antiphase to BUZZER_PIN, doubling the swing on the piezo.
constexpr uint8_t BUZZER_PIN_INV = 6;
// Channels 0 and 1 share LEDC timer 0, so they run at the same frequency.
constexpr uint8_t BUZZER_LEDC_CHANNEL = 0;
constexpr uint8_t BUZZER_LEDC_CHANNEL_INV = 1;
constexpr uint32_t LEDC_HALF_PERIOD = 512;  // ledcWriteTone() uses 10-bit resolution
// Duty per volume level: 50% is the loudest square wave.
constexpr uint32_t DUTY_HIGH = 512;
constexpr uint32_t DUTY_MEDIUM = 128;
constexpr uint32_t DUTY_LOW = 32;
// KY-006/HW-508 passive buzzer: loudest tones found with the T sweep.
constexpr uint32_t HONK_LOW_HZ = 3750;
constexpr uint32_t HONK_HIGH_HZ = 4250;
constexpr uint32_t STROBE_HZ = 4000;
// Single-tone profiles picked with the T sweep (4th and 11th tone).
constexpr uint32_t PROFILE_I_HZ = 2250;
constexpr uint32_t PROFILE_J_HZ = 4000;
// Current sound profile; used by H/B/1 (honk) and S (strobe).
uint32_t honkLowHz = HONK_LOW_HZ;
uint32_t honkHighHz = HONK_HIGH_HZ;
uint32_t strobeHz = STROBE_HZ;
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

// Frequency sweep used to find the buzzer's loudest (resonant) tone.
constexpr uint32_t SWEEP_START_HZ = 1500;
constexpr uint32_t SWEEP_END_HZ = 5000;
constexpr uint32_t SWEEP_STEP_HZ = 250;

// Horror tune: opening of Bach's Toccata and Fugue in D minor (public domain),
// one octave up so it sits in the buzzer's loud 2-4 kHz range. 0 Hz = rest.
struct Note {
  uint16_t hz;
  uint16_t ms;
};
constexpr uint16_t NOTE_A7 = 3520, NOTE_GS7 = 3322, NOTE_G7 = 3136, NOTE_FS7 = 2960,
                   NOTE_F7 = 2794, NOTE_E7 = 2637, NOTE_DS7 = 2489, NOTE_D7 = 2349,
                   NOTE_CS7 = 2217, NOTE_C7 = 2093, NOTE_B6 = 1976, NOTE_AS6 = 1865,
                   NOTE_A6 = 1760, NOTE_GS6 = 1661, NOTE_FS6 = 1480, NOTE_F6 = 1397,
                   NOTE_E6 = 1319, NOTE_D6 = 1175;
constexpr Note HORROR_TUNE[] = {
  {NOTE_A7, 110}, {NOTE_G7, 110}, {NOTE_A7, 700}, {0, 350},
  {NOTE_G7, 120}, {NOTE_F7, 120}, {NOTE_E7, 120}, {NOTE_D7, 120},
  {NOTE_CS7, 500}, {NOTE_D7, 900}, {0, 500},
  {NOTE_A7, 110}, {NOTE_G7, 110}, {NOTE_A7, 700}, {0, 350},
  {NOTE_G7, 120}, {NOTE_F7, 120}, {NOTE_E7, 120}, {NOTE_D7, 120},
  {NOTE_CS7, 500}, {NOTE_D7, 900}, {0, 300},
  {NOTE_D6, 1200},
};

// Operating-room monitor: heartbeat beeps that slow down, then a long flatline.
constexpr uint16_t MONITOR_HZ = 3000;
constexpr Note FLATLINE_TUNE[] = {
  {MONITOR_HZ, 110}, {0, 490}, {MONITOR_HZ, 110}, {0, 490}, {MONITOR_HZ, 110}, {0, 540},
  {MONITOR_HZ, 110}, {0, 590}, {MONITOR_HZ, 110}, {0, 690}, {MONITOR_HZ, 110}, {0, 790},
  {MONITOR_HZ, 110}, {0, 940}, {MONITOR_HZ, 110}, {0, 1140}, {MONITOR_HZ, 110}, {0, 1340},
  {MONITOR_HZ, 4000},
};

// Original tunes written for this project, in the mood of horror film themes
// (not transcriptions of any existing piece).
// "Notte": fast ostinato in 7/8 with a sinking bass note.
constexpr Note NIGHT_TUNE[] = {
  {NOTE_A6, 130}, {NOTE_C7, 130}, {NOTE_E7, 130}, {NOTE_A6, 130}, {NOTE_C7, 130}, {NOTE_F7, 130}, {NOTE_E7, 130},
  {NOTE_A6, 130}, {NOTE_C7, 130}, {NOTE_E7, 130}, {NOTE_A6, 130}, {NOTE_C7, 130}, {NOTE_F7, 130}, {NOTE_E7, 130},
  {NOTE_AS6, 130}, {NOTE_D7, 130}, {NOTE_F7, 130}, {NOTE_AS6, 130}, {NOTE_D7, 130}, {NOTE_FS7, 130}, {NOTE_F7, 130},
  {NOTE_AS6, 130}, {NOTE_D7, 130}, {NOTE_F7, 130}, {NOTE_AS6, 130}, {NOTE_D7, 130}, {NOTE_FS7, 130}, {NOTE_F7, 130},
  {0, 200}, {NOTE_E6, 900},
};
// "Rosso": nervous prog-rock style arpeggio in E minor, then a step down.
constexpr Note RED_TUNE[] = {
  {NOTE_E7, 100}, {NOTE_G7, 100}, {NOTE_E7, 100}, {NOTE_B6, 100}, {NOTE_E7, 100}, {NOTE_G7, 100}, {NOTE_A7, 100}, {NOTE_G7, 100},
  {NOTE_E7, 100}, {NOTE_G7, 100}, {NOTE_E7, 100}, {NOTE_B6, 100}, {NOTE_E7, 100}, {NOTE_G7, 100}, {NOTE_A7, 100}, {NOTE_G7, 100},
  {NOTE_D7, 100}, {NOTE_F7, 100}, {NOTE_D7, 100}, {NOTE_A6, 100}, {NOTE_D7, 100}, {NOTE_F7, 100}, {NOTE_G7, 100}, {NOTE_F7, 100},
  {NOTE_C7, 100}, {NOTE_DS7, 100}, {NOTE_C7, 100}, {NOTE_GS6, 100}, {NOTE_C7, 100}, {NOTE_DS7, 100}, {NOTE_F7, 100}, {NOTE_DS7, 100},
  {0, 150}, {NOTE_B6, 300}, {NOTE_E6, 900},
};
// "Nebbia": slow, dissonant notes (tritones) that hang in the air.
constexpr Note FOG_TUNE[] = {
  {NOTE_D7, 900}, {0, 250}, {NOTE_GS6, 1200}, {0, 350},
  {NOTE_CS7, 600}, {NOTE_C7, 600}, {0, 450}, {NOTE_FS6, 1600},
  {0, 400}, {NOTE_DS7, 500}, {NOTE_A6, 1800},
};

#define TUNE(t) t, sizeof(t) / sizeof(t[0])
constexpr uint32_t NOTE_GAP_MS = 25;  // short silence (LED off) so repeated notes are distinct

enum class Effect : uint8_t { None, HonkLow, HonkGap, HonkHigh, StrobeOn, StrobeOff, SweepOn, SweepOff, TuneNote, TuneGap };
Effect effect = Effect::None;
uint32_t nextTransition = 0;
uint8_t flashes = 0;
uint32_t sweepHz = 0;
const Note* tune = nullptr;
size_t tuneLen = 0;
size_t tuneIndex = 0;
bool ledIsOn = false;
bool soundIsOn = false;

void setLed(bool on) {
  ledIsOn = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

bool pushPull = true;  // louder on the KY-006 (tested)
uint32_t buzzerDuty = DUTY_HIGH;

void applyBuzzerDuty() {
  ledc_set_duty_with_hpoint(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(BUZZER_LEDC_CHANNEL), buzzerDuty, 0);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(BUZZER_LEDC_CHANNEL));
  if (pushPull) {
    // Same pulse, shifted by half a period: GPIO6 is high while GPIO5 is low.
    ledc_set_duty_with_hpoint(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(BUZZER_LEDC_CHANNEL_INV), buzzerDuty, LEDC_HALF_PERIOD);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(BUZZER_LEDC_CHANNEL_INV));
  }
}

void startSound(uint32_t frequency) {
  ledcWriteTone(BUZZER_LEDC_CHANNEL, frequency);  // sets timer 0 frequency
  if (!soundIsOn) {
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    if (pushPull) ledcAttachPin(BUZZER_PIN_INV, BUZZER_LEDC_CHANNEL_INV);
  }
  applyBuzzerDuty();
  soundIsOn = true;
}

void stopSound() {
  if (soundIsOn) {
    // Detach and hold both pins LOW: no voltage left across the piezo.
    ledcDetachPin(BUZZER_PIN);
    if (pushPull) ledcDetachPin(BUZZER_PIN_INV);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(BUZZER_PIN_INV, LOW);
    soundIsOn = false;
  }
}

void cancelEffect() {
  effect = Effect::None;
  stopSound();
}

void printAudioSettings() {
  const char* level = buzzerDuty == DUTY_HIGH ? "alto" : buzzerDuty == DUTY_MEDIUM ? "medio" : "basso";
  Serial.printf("[AUDIO] mode=%s volume=%s\n", pushPull ? "push-pull (GPIO5+GPIO6)" : "single (GPIO6=GND)", level);
}

void startHonk(uint32_t now);

// Settings changes play the honk so they can be heard from the phone.
void setPushPull(bool enabled) {
  cancelEffect();  // pins are re-attached on the next sound
  pushPull = enabled;
  printAudioSettings();
  startHonk(millis());
}

void setSoundProfile(uint32_t low, uint32_t high, uint32_t strobe) {
  cancelEffect();
  honkLowHz = low;
  honkHighHz = high;
  strobeHz = strobe;
  Serial.printf("[AUDIO] honk=%lu/%lu Hz strobe=%lu Hz\n", static_cast<unsigned long>(low),
                static_cast<unsigned long>(high), static_cast<unsigned long>(strobe));
  startHonk(millis());
}

void setVolume(uint32_t duty) {
  cancelEffect();
  buzzerDuty = duty;
  printAudioSettings();
  startHonk(millis());
}

void playSweepStep(uint32_t now) {
  Serial.printf("[SWEEP] %lu Hz\n", static_cast<unsigned long>(sweepHz));
  startSound(sweepHz);
  effect = Effect::SweepOn;
  nextTransition = now + 400;
}

// The nose lights on every note and goes dark on rests and gaps.
void playTuneNote(uint32_t now) {
  const Note& note = tune[tuneIndex];
  if (note.hz) {
    startSound(note.hz);
    setLed(true);
  } else {
    stopSound();
    setLed(false);
  }
  effect = Effect::TuneNote;
  nextTransition = now + note.ms;
}

void startTune(const Note* notes, size_t count, uint32_t now) {
  cancelEffect();
  tune = notes;
  tuneLen = count;
  tuneIndex = 0;
  playTuneNote(now);
}

void startSweep(uint32_t now) {
  cancelEffect();
  sweepHz = SWEEP_START_HZ;
  playSweepStep(now);
}

void startHonk(uint32_t now) {
  cancelEffect();
  startSound(honkLowHz);
  effect = Effect::HonkLow;
  nextTransition = now + 120;
}

void startStrobe(uint32_t now) {
  cancelEffect();
  flashes = 0;
  setLed(true);
  startSound(strobeHz);
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
    case 'M': // Horror tune with the nose blinking on each note; LED off at the end
      startTune(TUNE(HORROR_TUNE), now);
      break;
    case 'F': // Operating-room monitor: slowing heartbeat, then flatline (~13 s)
      startTune(TUNE(FLATLINE_TUNE), now);
      break;
    case 'W': // Original tune "Notte" (7/8 ostinato)
      startTune(TUNE(NIGHT_TUNE), now);
      break;
    case 'R': // Original tune "Rosso" (prog-style arpeggio)
      startTune(TUNE(RED_TUNE), now);
      break;
    case 'Y': // Original tune "Nebbia" (slow dissonant notes)
      startTune(TUNE(FOG_TUNE), now);
      break;
    case 'T': // Buzzer test: 1500-5000 Hz sweep, LED unchanged
      startSweep(now);
      break;
    case 'G': // Buzzer between GPIO5 and GPIO6 held LOW (like GND)
      setPushPull(false);
      break;
    case 'P': // Buzzer driven in antiphase by GPIO5 and GPIO6
      setPushPull(true);
      break;
    case 'I': // Sound profile: everything at 2250 Hz
      setSoundProfile(PROFILE_I_HZ, PROFILE_I_HZ, PROFILE_I_HZ);
      break;
    case 'J': // Sound profile: everything at 4000 Hz
      setSoundProfile(PROFILE_J_HZ, PROFILE_J_HZ, PROFILE_J_HZ);
      break;
    case 'K': // Sound profile: default 3750/4250 Hz honk, 4000 Hz strobe
      setSoundProfile(HONK_LOW_HZ, HONK_HIGH_HZ, STROBE_HZ);
      break;
    case 'A': // Volume: high
      setVolume(DUTY_HIGH);
      break;
    case 'N': // Volume: medium
      setVolume(DUTY_MEDIUM);
      break;
    case 'Q': // Volume: low
      setVolume(DUTY_LOW);
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
      // Short silence so the two tones are heard as "bip ... bip".
      stopSound();
      effect = Effect::HonkGap;
      nextTransition = now + 100;
      break;
    case Effect::HonkGap:
      startSound(honkHighHz);
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
        startSound(strobeHz);
        effect = Effect::StrobeOn;
        nextTransition = now + 40;
      }
      break;
    case Effect::SweepOn:
      stopSound();
      effect = Effect::SweepOff;
      nextTransition = now + 150;
      break;
    case Effect::SweepOff:
      sweepHz += SWEEP_STEP_HZ;
      if (sweepHz > SWEEP_END_HZ) {
        Serial.println("[SWEEP] Done");
        effect = Effect::None;
      } else {
        playSweepStep(now);
      }
      break;
    case Effect::TuneNote:
      stopSound();
      setLed(false);
      effect = Effect::TuneGap;
      nextTransition = now + NOTE_GAP_MS;
      break;
    case Effect::TuneGap:
      if (++tuneIndex >= tuneLen) {
        effect = Effect::None;
      } else {
        playTuneNote(now);
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
  pinMode(BUZZER_PIN_INV, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(BUZZER_PIN_INV, LOW);
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
  Serial.println("Commands: 1=light+horn 0=off H/B=horn L=toggle-light S=strobe M=horror-tune F=flatline W/R/Y=tunes T=tone-sweep G/P=buzzer single/push-pull A/N/Q=volume high/medium/low I/J/K=sound 2250/4000/default");
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
