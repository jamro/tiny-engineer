#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <ESP_I2S.h>
#include <esp32-hal-rgb-led.h>
#include <Adafruit_PWMServoDriver.h>

#include "pins.h"
#include "servos.h"

// PCBA session on the golden module. Product src/ is not linked.

namespace {

constexpr uint16_t kFullOff = 4096;
constexpr int kToneLowHz = 500;
constexpr int kToneHighHz = 750;
constexpr int16_t kToneAmplitude = 11000;
// Index 5 is the bench default. Peak of both partials stays inside int16.
constexpr int16_t kToneLevels[] = {
    2000, 4000, 6000, 8000, 9500, 11000, 12500, 14000, 15200, 16300,
};
constexpr int kToneLevelCount = sizeof(kToneLevels) / sizeof(kToneLevels[0]);
constexpr int kToneLevelDefault = 5;
// 441 frames closes both partials, so the loop junction stays continuous.
constexpr int kToneFrames = 441;

enum Phase : uint8_t { PhaseStart, PhaseWait, PhaseMenu, PhaseResult, PhaseStop };

struct StepDef {
  const char* name;
  int channel;
};

// channel < 0 is the speaker. kAllChannels pulses every servo output together.
// kOeOff releases the PCA9685 outputs while the same pulse keeps running.
constexpr int kAllChannels = -2;
constexpr int kOeOff = -3;

constexpr StepDef kSteps[] = {
    {"tone", -1},
    {"servos", kAllChannels},
    {"oe", kOeOff},
};

constexpr int kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);

static_assert(kStepCount == 3, "PCBA step list drifted");
static_assert(kToneLevelCount == 10, "volume card is 1..10");
static_assert(kToneLevels[kToneLevelDefault] == kToneAmplitude, "default level drifted");

I2SClass I2S;
Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS);

Phase gPhase = PhaseStart;
int gStep = 0;
bool gFailed[kStepCount] = {};
bool gPcaOk = false;
bool gToneOn = false;
int gToneLevel = kToneLevelDefault;
bool gI2sReady = false;
uint32_t gPulseAt = 0;
uint32_t gPromptAt = 0;
int16_t gTonePcm[kToneFrames * 2];

static_assert(kToneFrames * kToneLowHz % SAMPLE_RATE == 0, "low partial must close");
static_assert(kToneFrames * kToneHighHz % SAMPLE_RATE == 0, "high partial must close");

void setOe(bool enabled) {
  if (!PCA9685_OE_WIRED) {
    return;
  }
  digitalWrite(PCA9685_OE_PIN, enabled ? LOW : HIGH);
}

void writeChannel(int channel, bool high) {
  if (high) {
    pwm.setPWM(channel, kFullOff, 0);
  } else {
    pwm.setPWM(channel, 0, kFullOff);
  }
}

void allOff() {
  for (int channel = 0; channel < SERVO_COUNT; channel++) {
    writeChannel(channel, false);
  }
}

void writeDuty(int channel, uint16_t duty) {
  if (duty == 0) {
    pwm.setPWM(channel, 0, kFullOff);
    return;
  }
  pwm.setPWM(channel, 0, duty);
}

void servicePulse() {
  if (!gPcaOk || (gPhase != PhaseWait && gPhase != PhaseMenu)) {
    return;
  }
  const int channel = kSteps[gStep].channel;
  if (channel != kAllChannels && channel != kOeOff) {
    return;
  }
  const uint32_t now = millis();
  if (gPulseAt != 0 && now - gPulseAt < 20) {
    return;
  }
  gPulseAt = now;
  constexpr uint32_t kHalf = 500;
  const uint32_t t = now % (kHalf * 2);
  const uint32_t rise = t < kHalf ? t : (kHalf * 2 - t);
  const uint16_t duty = static_cast<uint16_t>(rise * 4095 / kHalf);
  for (int channel = 0; channel < SERVO_COUNT; channel++) {
    writeDuty(channel, duty);
  }
}

bool probePca() {
  Wire.beginTransmission(PCA9685_ADDRESS);
  return Wire.endTransmission() == 0;
}

void printResult() {
  Serial.print("RESULT ");
  bool any = false;
  for (int index = 0; index < kStepCount; index++) {
    if (gFailed[index]) {
      any = true;
      break;
    }
  }
  if (!any) {
    Serial.println("PASS");
    return;
  }
  Serial.print("FAIL");
  for (int index = 0; index < kStepCount; index++) {
    if (!gFailed[index]) {
      continue;
    }
    Serial.print(' ');
    Serial.print(kSteps[index].name);
  }
  Serial.println();
}

void prepareTone();
void printVolume();

void enterStep(int step) {
  gStep = step;
  gToneOn = false;
  const StepDef& def = kSteps[step];
  if (def.channel == kOeOff) {
    setOe(false);
    gPulseAt = 0;
  } else if (def.channel == kAllChannels) {
    setOe(true);
    gPulseAt = 0;
  } else {
    setOe(true);
    allOff();
    gToneOn = true;
  }
  Serial.print("STEP ");
  Serial.println(def.name);
  if (gToneOn) {
    printVolume();
  }
  gPhase = PhaseWait;
}

void finishBoard() {
  gToneOn = false;
  printResult();
  gPhase = PhaseResult;
}

void acceptStep() {
  if (gStep + 1 >= kStepCount) {
    finishBoard();
    return;
  }
  enterStep(gStep + 1);
}

void failStep() {
  gFailed[gStep] = true;
  gToneOn = false;
  Serial.print("FAIL ");
  Serial.println(kSteps[gStep].name);
  gPhase = PhaseMenu;
}

void restartBoard() {
  for (int index = 0; index < kStepCount; index++) {
    gFailed[index] = false;
  }
  gToneOn = false;
  gStep = 0;
  allOff();
  setOe(true);
  Serial.println("READY");
  gPhase = PhaseStart;
}

void printVolume() {
  Serial.print("VOL ");
  Serial.println(gToneLevel + 1);
}

void changeVolume(int delta) {
  int next = gToneLevel + delta;
  if (next < 0) {
    next = 0;
  } else if (next >= kToneLevelCount) {
    next = kToneLevelCount - 1;
  }
  if (next != gToneLevel) {
    gToneLevel = next;
    prepareTone();
  }
  printVolume();
}

void onWaitKey(char key) {
  if (key == '\n') {
    acceptStep();
    return;
  }
  if (key == 'n') {
    failStep();
    return;
  }
  if (!gToneOn) {
    return;
  }
  if (key == '+' || key == '=') {
    changeVolume(1);
    return;
  }
  if (key == '-') {
    changeVolume(-1);
  }
}

void onMenuKey(char key) {
  if (key == 'r') {
    gFailed[gStep] = false;
    enterStep(gStep);
    return;
  }
  if (key == 'c') {
    if (gStep + 1 >= kStepCount) {
      finishBoard();
      return;
    }
    enterStep(gStep + 1);
    return;
  }
  if (key == 'a') {
    restartBoard();
  }
}

void onResultKey(char key) {
  if (key == '?') {
    if (!gPcaOk) {
      Serial.println("FAIL pca");
      Serial.println("RESULT FAIL pca");
    }
    return;
  }
  if (key == 'q') {
    Serial.println("STOP");
    gPhase = PhaseStop;
  }
}

void onStartKey(char key) {
  if (key == '\n') {
    enterStep(0);
  }
}

void onKey(char key) {
  if (gPhase == PhaseStart) {
    onStartKey(key);
  } else if (gPhase == PhaseWait) {
    onWaitKey(key);
  } else if (gPhase == PhaseMenu) {
    onMenuKey(key);
  } else if (gPhase == PhaseResult) {
    onResultKey(key);
  }
}

void pollKeys() {
  while (Serial.available() > 0) {
    onKey(static_cast<char>(Serial.read()));
  }
}

void prepareTone() {
  for (int frame = 0; frame < kToneFrames; frame++) {
    const float low = sinf(2.0f * PI * frame * kToneLowHz / SAMPLE_RATE);
    const float high = sinf(2.0f * PI * frame * kToneHighHz / SAMPLE_RATE);
    float raw = (low + high) * kToneLevels[gToneLevel];
    if (raw > 32767.0f) {
      raw = 32767.0f;
    } else if (raw < -32767.0f) {
      raw = -32767.0f;
    }
    const int16_t sample = static_cast<int16_t>(raw);
    gTonePcm[frame * 2] = sample;
    gTonePcm[frame * 2 + 1] = sample;
  }
}

void writeSamples(const int16_t* samples, size_t bytes) {
  if (!gI2sReady || bytes == 0) {
    return;
  }
  I2S.write(reinterpret_cast<uint8_t*>(const_cast<int16_t*>(samples)), bytes);
}

void writeSilenceMs(int ms) {
  int16_t zeros[128 * 2] = {};
  int frames = SAMPLE_RATE * ms / 1000;
  while (frames > 0) {
    const int count = frames > 128 ? 128 : frames;
    writeSamples(zeros, static_cast<size_t>(count) * 2 * sizeof(int16_t));
    frames -= count;
  }
}

void playNote(int hz, int frames) {
  constexpr int16_t kChimeAmplitude = 9000;
  static int16_t buffer[882 * 2];
  for (int frame = 0; frame < frames; frame++) {
    const float raw = sinf(2.0f * PI * frame * hz / SAMPLE_RATE) * kChimeAmplitude;
    const int16_t sample = static_cast<int16_t>(raw);
    buffer[frame * 2] = sample;
    buffer[frame * 2 + 1] = sample;
  }
  writeSamples(buffer, static_cast<size_t>(frames) * 2 * sizeof(int16_t));
}

void playPowerChime() {
  // Ready signal after PCA ACK: power is up and the board can be tested.
  // Not the tone step. 40 ms notes a fifth apart. 882 frames closes both.
  playNote(800, 882);
  writeSilenceMs(20);
  playNote(1200, 882);
  writeSilenceMs(20);
}

void serviceTone() {
  if (!gToneOn || !gI2sReady || gPhase != PhaseWait) {
    return;
  }
  writeSamples(gTonePcm, sizeof(gTonePcm));
}

void startI2s() {
  I2S.setPins(I2S_BCLK, I2S_LRC, I2S_DIN);
  gI2sReady = I2S.begin(
      I2S_MODE_STD,
      SAMPLE_RATE,
      I2S_DATA_BIT_WIDTH_16BIT,
      I2S_SLOT_MODE_STEREO);
}

void printPrompt() {
  if (gPhase == PhaseStart) {
    Serial.println("READY");
  } else if (gPhase == PhaseWait) {
    Serial.print("STEP ");
    Serial.println(kSteps[gStep].name);
  } else if (gPhase == PhaseMenu) {
    Serial.print("FAIL ");
    Serial.println(kSteps[gStep].name);
  }
}

void servicePrompt() {
  if (gPhase != PhaseStart && gPhase != PhaseWait && gPhase != PhaseMenu) {
    return;
  }
  const uint32_t now = millis();
  if (gPromptAt != 0 && now - gPromptAt < 200) {
    return;
  }
  gPromptAt = now;
  printPrompt();
}

void beginSession() {
  Serial.println("PCBA-TEST v1");
  Wire.begin(I2C_SDA, I2C_SCL);
  gPcaOk = probePca();
  if (!gPcaOk) {
    Serial.println("FAIL pca");
    Serial.println("RESULT FAIL pca");
    gPhase = PhaseResult;
    return;
  }

  pwm.begin();
  pwm.setPWMFreq(200);
  allOff();
  setOe(true);
  const uint32_t drainUntil = millis() + 50;
  while (millis() < drainUntil) {
    while (Serial.available() > 0) {
      Serial.read();
    }
    delay(5);
  }
  gPhase = PhaseStart;
  gToneOn = false;
  playPowerChime();
}

}  // namespace

void setup() {
  // Equal channels stay white for every WS2812 wire order. Leave it on.
  constexpr uint8_t kModuleWhite = 64;
  rgbLedWrite(RGB_LED_PIN, kModuleWhite, kModuleWhite, kModuleWhite);

  Serial.begin(115200);
  if (PCA9685_OE_WIRED) {
    pinMode(PCA9685_OE_PIN, OUTPUT);
    digitalWrite(PCA9685_OE_PIN, HIGH);
  }
  startI2s();
  prepareTone();
  beginSession();
}

void loop() {
  if (gPhase == PhaseStop) {
    return;
  }
  servicePrompt();
  serviceTone();
  servicePulse();
  pollKeys();
}
