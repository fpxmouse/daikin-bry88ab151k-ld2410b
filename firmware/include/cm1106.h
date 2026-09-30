#pragma once

// Source adapted from:
// https://github.com/louliangsheng/daikin-air-sensor/blob/main/header/cm1106.h
#include "esphome.h"

class CM1106 : public UARTDevice {
 public:
  explicit CM1106(UARTComponent *parent) : UARTDevice(parent) {}

  void set_co2_calib_value(uint16_t ppm = 400) {
    uint8_t command[6];
    memcpy(command, SET_CO2_CALIB, sizeof(command));
    command[3] = ppm >> 8;
    command[4] = ppm & 0xFF;
    uint8_t response[4] = {0};
    if (!send_command(command, sizeof(command), response, sizeof(response))) {
      ESP_LOGW(TAG, "Writing calibration to CM1106 failed");
      return;
    }
    if (memcmp(response, SET_CO2_CALIB_RESPONSE, sizeof(response)) != 0) {
      ESP_LOGW(TAG, "Unexpected calibration response: %02X %02X %02X %02X",
               response[0], response[1], response[2], response[3]);
    }
  }

  int16_t get_co2_ppm() {
    uint8_t response[8] = {0};
    if (!send_command(GET_CO2, sizeof(GET_CO2), response, sizeof(response))) {
      ESP_LOGW(TAG, "Reading data from CM1106 failed");
      return -1;
    }
    if (response[0] != 0x16 || response[1] != 0x05 || response[2] != 0x01) {
      ESP_LOGW(TAG, "Unexpected CM1106 response: %02X %02X %02X %02X",
               response[0], response[1], response[2], response[3]);
      return -1;
    }
    if (response[7] != checksum(response, sizeof(response))) {
      ESP_LOGW(TAG, "CM1106 checksum mismatch");
      return -1;
    }
    return static_cast<int16_t>(response[3] << 8 | response[4]);
  }

 private:
  static constexpr const char *TAG = "cm1106";
  uint8_t GET_CO2[4] = {0x11, 0x01, 0x01, 0xED};
  uint8_t SET_CO2_CALIB[6] = {0x11, 0x03, 0x03, 0x00, 0x00, 0x00};
  uint8_t SET_CO2_CALIB_RESPONSE[4] = {0x16, 0x01, 0x03, 0xE6};

  uint8_t checksum(const uint8_t *data, size_t length) {
    uint8_t value = 0;
    for (size_t i = 0; i + 1 < length; i++) value -= data[i];
    return value;
  }

  bool send_command(uint8_t *command, size_t command_length,
                    uint8_t *response = nullptr, size_t response_length = 0) {
    while (available()) read();
    command[command_length - 1] = checksum(command, command_length);
    write_array(command, command_length);
    flush();
    return response == nullptr || read_array(response, response_length);
  }
};

class CM1106Sensor : public PollingComponent, public Sensor {
 public:
  CM1106Sensor(UARTComponent *parent, uint32_t update_interval)
      : PollingComponent(update_interval), sensor_(parent) {}

  float get_setup_priority() const override { return setup_priority::DATA; }

  void update() override {
    int16_t ppm = sensor_.get_co2_ppm();
    if (ppm >= 0) publish_state(ppm);
  }

 private:
  CM1106 sensor_;
};

class CM1106CalibrateSwitch : public Component, public Switch {
 public:
  explicit CM1106CalibrateSwitch(UARTComponent *parent) : sensor_(parent) {}

  void write_state(bool state) override {
    publish_state(state);
    if (state) {
      sensor_.set_co2_calib_value();
      turn_off();
    }
  }

 private:
  CM1106 sensor_;
};
