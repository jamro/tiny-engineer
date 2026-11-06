#include "serial/link.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <cobs.h>
#include <cstring>

#if SOC_USB_SERIAL_JTAG_SUPPORTED
#include <HWCDC.h>

#include "hal/usb_serial_jtag_ll.h"
#endif

#include "animation.h"
#include "api/commands.h"
#include "firmware_version.h"
#include "network/wifi_connect.h"
#include "serial/frame.h"
#include "serial/play.h"
#include "serial/protocol.h"
#include "settings/settings.h"

namespace {

bool g_ready = false;
bool g_usbReset = false;
SerialFrameDecoder g_decoder;
uint32_t g_lastUsbRearmMs = 0;

#if SOC_USB_SERIAL_JTAG_SUPPORTED
// Bytes already in the FIFO do not raise SERIAL_OUT_RECV again. Drop them so
// the next host packet is an empty→data edge the ISR can see.
void discardHardwareRxFifo() {
  uint8_t dump[64];
  while (usb_serial_jtag_ll_rxfifo_data_available()) {
    if (usb_serial_jtag_ll_read_rxfifo(dump, sizeof(dump)) <= 0) {
      break;
    }
  }
  usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
}

void recoverHardwareRx() {
  const bool fifo = usb_serial_jtag_ll_rxfifo_data_available() != 0;
  const bool irq =
    (usb_serial_jtag_ll_get_intsts_mask() & USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT) != 0;
  const bool armed =
    (usb_serial_jtag_ll_get_intr_ena_status() & USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT) != 0;

  // A live ISR owns the FIFO while its status bit is set and the mask is armed.
  if (fifo && !(irq && armed)) {
    uint8_t buf[64];
    while (usb_serial_jtag_ll_rxfifo_data_available()) {
      const int n = usb_serial_jtag_ll_read_rxfifo(buf, sizeof(buf));
      if (n <= 0) {
        break;
      }
      if (g_ready) {
        g_decoder.feed(buf, static_cast<size_t>(n));
      }
    }
    if (irq) {
      usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
    }
  }

  if (!armed) {
    const uint32_t now = millis();
    if (now - g_lastUsbRearmMs >= 1000) {
      g_lastUsbRearmMs = now;
      Serial.begin(115200);
      Serial.setTxTimeoutMs(0);
    }
  }
}
#endif

uint8_t g_encodeBuf[1 + kSerialMaxPayload + ((kSerialMaxPayload + 253) / 254) + 1];

void writeFrame(const uint8_t* payload, size_t payloadLen) {
  const size_t encoded = serialFrameEncode(payload, payloadLen, g_encodeBuf, sizeof(g_encodeBuf));
  if (encoded == 0) {
    return;
  }
  Serial.write(g_encodeBuf, encoded);
}

void sendPacked(const SerialHeader& header, const uint8_t* payload, size_t payloadLen) {
  // File-scope would be ideal; static keeps ~2 KB off the Arduino loop-task stack.
  static uint8_t message[kSerialHeaderSize + kSerialMaxPayload];
  const size_t n = serialPackMessage(header, payload, payloadLen, message, sizeof(message));
  if (n == 0) {
    return;
  }
  writeFrame(message, n);
}

void sendJsonRes(uint16_t id, int code, const char* error, JsonDocument& body) {
  body["ok"] = error == nullptr;
  if (error != nullptr) {
    body["error"] = error;
  }
  body["code"] = code;

  char json[1024];
  const size_t len = serializeJson(body, json, sizeof(json));
  if (len == 0 || len >= sizeof(json)) {
    return;
  }

  SerialHeader header;
  header.type = SerialMsgType::Res;
  header.ver = kSerialProtoVersion;
  header.id = id;
  sendPacked(header, reinterpret_cast<const uint8_t*>(json), len);
}

void handleSync(uint16_t id) {
  char json[160];
  const size_t len = serialFormatSyncJson(
    wifiProvisioningMode() || !settingsProvisioned(),
    settingsLinkMode(),
    json,
    sizeof(json)
  );
  if (len == 0) {
    return;
  }

  SerialHeader header;
  header.type = SerialMsgType::Sync;
  header.ver = kSerialProtoVersion;
  header.id = id;
  sendPacked(header, reinterpret_cast<const uint8_t*>(json), len);
}

void handleReq(uint16_t id, uint8_t ver, const uint8_t* payload, size_t payloadLen) {
  JsonDocument body;

  if (ver != kSerialProtoVersion) {
    sendJsonRes(id, 400, "unsupported protocol version", body);
    return;
  }

  JsonDocument req;
  if (deserializeJson(req, payload, payloadLen) || !req.is<JsonObject>()) {
    sendJsonRes(id, 400, "invalid json", body);
    return;
  }

  const char* op = req["op"] | "";
  if (op[0] == '\0') {
    sendJsonRes(id, 400, "missing op", body);
    return;
  }

  const bool provisioning = wifiProvisioningMode() || !settingsProvisioned();
  if (provisioning &&
      strcmp(op, "ping") != 0 &&
      strcmp(op, "sync") != 0 &&
      strcmp(op, "servos_park") != 0) {
    sendJsonRes(id, 503, "provisioning", body);
    return;
  }

  if (strcmp(op, "ping") == 0 || strcmp(op, "sync") == 0) {
    handleSync(id);
    return;
  }

  if (strcmp(op, "servos_park") == 0) {
    const ApiResult result = apiParkServos();
    if (!result.ok) {
      sendJsonRes(id, result.code, result.error, body);
      return;
    }
    body["park"] = "servos";
    body["hold"] = true;
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  if (strcmp(op, "anim") == 0) {
    const char* name = req["name"] | static_cast<const char*>(nullptr);
    const ApiResult result = apiSetAnimation(name);
    if (!result.ok) {
      sendJsonRes(id, result.code, result.error, body);
      return;
    }
    body["animation"] = animationName(getAnimation());
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  if (strcmp(op, "factory_reset") == 0) {
    const ApiResult result = apiFactoryReset();
    sendJsonRes(id, result.code, result.error, body);
    return;
  }

  if (strcmp(op, "test") == 0) {
    const char* name = req["name"] | static_cast<const char*>(nullptr);
    const ApiResult result = apiRunHardwareTest(name);
    if (!result.ok) {
      sendJsonRes(id, result.code, result.error, body);
      return;
    }
    body["test"] = name;
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  if (strcmp(op, "servo") == 0) {
    if (!req["index"].is<int>() || !req["angle"].is<float>()) {
      sendJsonRes(id, 400, "index and angle required", body);
      return;
    }
    const ApiResult result = apiMoveServo(req["index"].as<int>(), req["angle"].as<float>());
    if (!result.ok) {
      sendJsonRes(id, result.code, result.error, body);
      return;
    }
    body["index"] = req["index"];
    body["angle"] = req["angle"];
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  if (strcmp(op, "settings.get") == 0) {
    body["sleep_timeout"] = settingsSleepTimeoutMin();
    body["hostname"] = settingsHostname();
    body["volume"] = settingsVolume();
    body["welcome"] = settingsWelcomeEnabled();
    body["serial_log"] = settingsSerialLogEnabled();
    body["continuous_timeout"] = settingsContinuousTimeoutMin();
    body["loading"] = settingsLoading();
    body["eyes_style"] = settingsEyesStyle();
    body["access_token_set"] = settingsAccessTokenSet();
    body["link_mode"] = settingsLinkMode();
    body["wifi_configured"] = settingsWifiConfigured();

    JsonArray servoMins = body["servo_mins"].to<JsonArray>();
    JsonArray servoMaxs = body["servo_maxs"].to<JsonArray>();
    for (size_t i = 0; i < SETTINGS_SERVO_COUNT; i++) {
      servoMins.add(static_cast<unsigned>(settingsServoMin(i)));
      servoMaxs.add(static_cast<unsigned>(settingsServoMax(i)));
    }

    body["rgb_order"] = settingsRgbOrder();
    body["oled_rotate_180"] = settingsOledRotate180();
    body["reboot_required"] = settingsRebootRequired();
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  if (strcmp(op, "settings.set") == 0) {
    SettingsPatch patch;
    bool any = false;

    if (!req["volume"].isUnbound()) {
      if (!req["volume"].is<uint8_t>()) {
        sendJsonRes(id, 400, "invalid volume", body);
        return;
      }
      patch.volume = req["volume"].as<uint8_t>();
      any = true;
    }
    if (!req["serial_log"].isUnbound()) {
      if (!req["serial_log"].is<bool>()) {
        sendJsonRes(id, 400, "invalid serial_log", body);
        return;
      }
      patch.serialLog = req["serial_log"].as<bool>();
      any = true;
    }
    if (!req["welcome"].isUnbound()) {
      if (!req["welcome"].is<bool>()) {
        sendJsonRes(id, 400, "invalid welcome", body);
        return;
      }
      patch.welcome = req["welcome"].as<bool>();
      any = true;
    }
    if (!req["link_mode"].isUnbound()) {
      if (!req["link_mode"].is<const char*>()) {
        sendJsonRes(id, 400, "invalid link_mode", body);
        return;
      }
      patch.linkMode = req["link_mode"].as<const char*>();
      any = true;
    }
    if (!req["eyes_style"].isUnbound()) {
      if (!req["eyes_style"].is<const char*>()) {
        sendJsonRes(id, 400, "invalid eyes_style", body);
        return;
      }
      patch.eyesStyle = req["eyes_style"].as<const char*>();
      any = true;
    }
    if (!req["hostname"].isUnbound()) {
      if (!req["hostname"].is<const char*>()) {
        sendJsonRes(id, 400, "invalid hostname", body);
        return;
      }
      patch.hostname = req["hostname"].as<const char*>();
      any = true;
    }

    if (!any) {
      sendJsonRes(id, 400, "send at least one setting", body);
      return;
    }

    const ApiResult result = apiSaveSettings(patch);
    sendJsonRes(id, result.code, result.error, body);
    return;
  }

  if (strcmp(op, "health") == 0) {
    body["version"] = FW_VERSION;
    body["uptime_ms"] = millis();
    body["free_heap"] = ESP.getFreeHeap();
    body["link_mode"] = settingsLinkMode();
    body["wifi_configured"] = settingsWifiConfigured();
    body["provisioning"] = wifiProvisioningMode();
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  if (strcmp(op, "play") == 0) {
    const char* name = req["name"] | "talking";
    uint8_t streamId = 0;
    uint16_t credits = 0;
    const char* error = nullptr;
    if (!serialPlayBegin(name, streamId, credits, error)) {
      const int code = (error != nullptr && strcmp(error, "busy") == 0) ? 503 : 400;
      sendJsonRes(id, code, error != nullptr ? error : "play failed", body);
      return;
    }
    body["stream_id"] = streamId;
    body["credits"] = credits;
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  if (strcmp(op, "play_end") == 0) {
    if (!req["stream_id"].is<uint8_t>() && !req["stream_id"].is<int>()) {
      sendJsonRes(id, 400, "stream_id required", body);
      return;
    }
    const uint8_t streamId = static_cast<uint8_t>(req["stream_id"].as<int>());
    uint32_t playedMs = 0;
    const char* error = nullptr;
    if (!serialPlayEnd(streamId, playedMs, error)) {
      sendJsonRes(id, 400, error != nullptr ? error : "play_end failed", body);
      return;
    }
    body["played_ms"] = playedMs;
    sendJsonRes(id, 200, nullptr, body);
    return;
  }

  sendJsonRes(id, 404, "unknown op", body);
}

void sendStreamCredit(uint8_t streamId, uint16_t seq, uint16_t credits) {
  uint8_t payload[kSerialStreamHdrSize + 2];
  payload[0] = streamId;
  payload[1] = kSerialStreamKindCredit;
  payload[2] = static_cast<uint8_t>(seq & 0xFF);
  payload[3] = static_cast<uint8_t>((seq >> 8) & 0xFF);
  payload[4] = static_cast<uint8_t>(credits & 0xFF);
  payload[5] = static_cast<uint8_t>((credits >> 8) & 0xFF);

  SerialHeader header;
  header.type = SerialMsgType::Stream;
  header.ver = kSerialProtoVersion;
  header.id = 0;
  sendPacked(header, payload, sizeof(payload));
}

void sendStreamError(uint8_t streamId, uint16_t seq, const char* error) {
  const char* text = error != nullptr ? error : "stream error";
  const size_t textLen = strnlen(text, 80);
  uint8_t payload[kSerialStreamHdrSize + 80];
  payload[0] = streamId;
  payload[1] = kSerialStreamKindError;
  payload[2] = static_cast<uint8_t>(seq & 0xFF);
  payload[3] = static_cast<uint8_t>((seq >> 8) & 0xFF);
  if (textLen > 0) {
    std::memcpy(payload + kSerialStreamHdrSize, text, textLen);
  }

  SerialHeader header;
  header.type = SerialMsgType::Stream;
  header.ver = kSerialProtoVersion;
  header.id = 0;
  sendPacked(header, payload, kSerialStreamHdrSize + textLen);
}

void handleStream(uint8_t ver, const uint8_t* payload, size_t payloadLen) {
  if (ver != kSerialProtoVersion || payloadLen < kSerialStreamHdrSize) {
    return;
  }

  const uint8_t streamId = payload[0];
  const uint8_t kind = payload[1];
  const uint16_t seq = static_cast<uint16_t>(payload[2] | (static_cast<uint16_t>(payload[3]) << 8));
  const uint8_t* data = payload + kSerialStreamHdrSize;
  const size_t dataLen = payloadLen - kSerialStreamHdrSize;

  if (kind == kSerialStreamKindData) {
    uint16_t credits = 0;
    const char* error = nullptr;
    if (!serialPlayFeed(streamId, data, dataLen, credits, error)) {
      sendStreamError(streamId, seq, error);
      return;
    }
    sendStreamCredit(streamId, seq, credits);
    return;
  }

  if (kind == kSerialStreamKindEnd) {
    uint32_t playedMs = 0;
    const char* error = nullptr;
    serialPlayEnd(streamId, playedMs, error);
    (void)playedMs;
    (void)error;
  }
}

void onFrame(const uint8_t* payload, size_t length, void* /*user*/) {
  SerialHeader header;
  size_t headerBytes = 0;
  if (!serialParseHeader(payload, length, header, headerBytes)) {
    return;
  }

  const uint8_t* body = payload + headerBytes;
  const size_t bodyLen = header.len;

  switch (header.type) {
    case SerialMsgType::Sync:
      handleSync(header.id);
      break;
    case SerialMsgType::Req:
      handleReq(header.id, header.ver, body, bodyLen);
      break;
    case SerialMsgType::Stream:
      handleStream(header.ver, body, bodyLen);
      break;
    default:
      break;
  }
}

}  // namespace

#if SOC_USB_SERIAL_JTAG_SUPPORTED
extern "C" void tinyEngineerOnUsbBusReset(void*, esp_event_base_t, int32_t, void*) {
  // Host restart and USB replug reset the bus without resetting the CPU.
  // begin() arms the RX interrupt again; poll drops a torn frame.
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  g_usbReset = true;
}
#endif

void serialLinkWatchUsb() {
#if SOC_USB_SERIAL_JTAG_SUPPORTED
  while (Serial.available() > 0) {
    Serial.read();
  }
  discardHardwareRxFifo();
  Serial.onEvent(ARDUINO_HW_CDC_BUS_RESET_EVENT, tinyEngineerOnUsbBusReset);
#endif
}

void serialLinkBegin() {
  g_decoder.reset();
  g_decoder.setHandler(onFrame, nullptr);
  g_ready = true;
}

void serialLinkPoll() {
  if (!g_ready) {
    return;
  }

#if SOC_USB_SERIAL_JTAG_SUPPORTED
  if (g_usbReset) {
    g_usbReset = false;
    while (Serial.available() > 0) {
      Serial.read();
    }
    g_decoder.reset();
  }
  recoverHardwareRx();
#endif

  while (Serial.available() > 0) {
    const int byte = Serial.read();
    if (byte < 0) {
      break;
    }
    const uint8_t value = static_cast<uint8_t>(byte);
    g_decoder.feed(&value, 1);
  }
}

bool serialLinkReady() {
  return g_ready;
}

void serialLinkEmitLog(const char* text) {
  if (!g_ready || text == nullptr || !settingsSerialLogEnabled()) {
    return;
  }

  char json[512];
  const size_t len = serialFormatLogJson(text, json, sizeof(json));
  if (len == 0) {
    return;
  }

  SerialHeader header;
  header.type = SerialMsgType::Log;
  header.ver = kSerialProtoVersion;
  header.id = 0;
  sendPacked(header, reinterpret_cast<const uint8_t*>(json), len);
}

void serialLinkEmitEvent(const char* json, size_t jsonLen) {
  if (!g_ready || json == nullptr || jsonLen == 0 || jsonLen > kSerialMaxPayload) {
    return;
  }

  SerialHeader header;
  header.type = SerialMsgType::Evt;
  header.ver = kSerialProtoVersion;
  header.id = 0;

  static uint8_t message[kSerialHeaderSize + kSerialMaxPayload];
  const size_t n =
    serialPackMessage(header, reinterpret_cast<const uint8_t*>(json), jsonLen, message, sizeof(message));
  if (n == 0) {
    return;
  }

  const size_t encoded = serialFrameEncode(message, n, g_encodeBuf, sizeof(g_encodeBuf));
  if (encoded == 0) {
    return;
  }

  // Drop rather than block when the CDC TX queue cannot take the whole frame.
  if (static_cast<size_t>(Serial.availableForWrite()) < encoded) {
    return;
  }

  Serial.write(g_encodeBuf, encoded);
}
