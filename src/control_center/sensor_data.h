#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include "ringbuffer.h"
#include <iostream>

enum class SensorType {
  BEGIN = 1,
  WaterTemperature = BEGIN,
  WaterTurbidity,
  Dust,
  AtmosphericPressure,
  AtmosphericTemperature,
  AtmosphericHumidity,
  BatteryVoltage,
  END
};

class SensorData {
public:
  inline static constexpr size_t sensor_count =
      static_cast<size_t>(SensorType::END) -
      static_cast<size_t>(SensorType::BEGIN);
  inline static constexpr size_t buffer_size = 120;
  void add_reading(SensorType sensor, int64_t /*time*/, float reading) {
    data[static_cast<size_t>(sensor) - 1].push_back(reading);
  }
  const Ringbuffer<float, buffer_size> operator[](size_t idx) const noexcept {
    return data[idx];
  }
  float min_val(size_t idx) const noexcept {
    switch (idx) {
    case 0:
      return 20.0f;
    case 1:
      return 0.0f;
    case 2:
      return 80.0f;
    case 3:
      return 100700.0f;
    case 4:
      return 23.0f;
    case 5:
      return 31.0f;
    case 6:
      return 0.0f;
    }
    return 0.0f;
  }
  float max_val(size_t idx) const noexcept {
    switch (idx) {
    case 0:
      return 22.0f;
    case 1:
      return 0.0f;
    case 2:
      return 105.0f;
    case 3:
      return 100800.0f;
    case 4:
      return 24.0f;
    case 5:
      return 32.0f;
    case 6:
      return 0.0f;
    }
    return 0.0f;
  }
  const char *name(size_t idx) const noexcept {
    switch (idx) {
    case 0:
      return "Water Temperature";
    case 1:
      return "Water Turbidity";
    case 2:
      return "Dust";
    case 3:
      return "Atmospheric Pressure";
    case 4:
      return "Atmospheric Temperature";
    case 5:
      return "Humidity";
    case 6:
      return "Battery Voltage";
    }
    return nullptr;
  }

private:
  Ringbuffer<float, buffer_size> data[sensor_count];
};

#endif