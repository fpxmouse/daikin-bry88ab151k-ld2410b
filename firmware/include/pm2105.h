#pragma once

// Protocol logic based on the PM2105 implementation in:
// https://github.com/louliangsheng/daikin-air-sensor/blob/main/header/pm2105.h
#include "esphome.h"

class pm2005 : public PollingComponent, public Sensor {
 public:
  pm2005() : PollingComponent(1000) {}

  void update() override {
    Wire.requestFrom(0x28, 12);
    if (Wire.available() < 11) {
      ESP_LOGW("pm2105", "Short I2C frame: %d bytes", Wire.available());
      while (Wire.available()) Wire.read();
      return;
    }

    // PM2105 starts directly at P1; unlike PM2005, no prefix is discarded.
    const uint8_t frame_header = Wire.read();
    Wire.read();  // Frame length.
    const uint8_t status = Wire.read();
    Wire.read();  // Reserved high byte.
    Wire.read();  // Reserved low byte.
    const uint16_t pm25 = static_cast<uint16_t>(Wire.read()) << 8 | Wire.read();
    Wire.read();  // PM10 high byte.
    Wire.read();  // PM10 low byte.
    Wire.read();  // Measuring mode high byte.
    Wire.read();  // Measuring mode low byte.

    if (frame_header == 0x16 && status == 0x80) publish_state(pm25);
    while (Wire.available()) Wire.read();
  }
};
