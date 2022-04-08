#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include "ringbuffer.h"
#include <iostream>
#include <limits>

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
  SensorData() {
    std::fill_n(min_vals, sensor_count, 0);
    std::fill_n(max_vals, sensor_count, 0);
  }
  void add_reading(SensorType sensor, int64_t /*time*/, float reading) {
    const auto idx = static_cast<size_t>(sensor) - 1;
    min_vals[idx] = std::min(min_vals[idx], reading);
    max_vals[idx] = std::max(max_vals[idx], reading);
    data[idx].push_back(reading);
  }
  const Ringbuffer<float, buffer_size> operator[](size_t idx) const noexcept {
    return data[idx];
  }
  float min_val(size_t idx) const noexcept {
    return min_vals[idx];
  }
  float max_val(size_t idx) const noexcept {
    return max_vals[idx];
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
  float min_vals[sensor_count];
  float max_vals[sensor_count];
};

#endif