#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include "ringbuffer.h"
#include <iostream>

enum class SensorType
{
    BEGIN = 1,
    WaterTemperature = BEGIN,
    WaterTurbidity,
    Dust,
    AtmosphericPressure,
    AtmosphericTemperature,
    AtmosphericHumidity,
    END
};

class SensorData
{
public:
    inline static constexpr size_t sensor_count = static_cast<size_t>(SensorType::END) - static_cast<size_t>(SensorType::BEGIN);
    inline static constexpr size_t buffer_size = 120;
    void add_reading(SensorType sensor, int64_t /*time*/, float reading)
    {
        data[static_cast<size_t>(sensor) - 1].push_back(reading);
    }
    const Ringbuffer<float, buffer_size> operator[](size_t idx) const noexcept
    {
        return data[idx];
    }
    float min_val(size_t idx) const noexcept
    {
        return 0.0f; // TODO: return actual value
    }
    float max_val(size_t idx) const noexcept
    {
        return 1.0f; // TODO: return actual value
    }

private:
    Ringbuffer<float, buffer_size> data[sensor_count];
};

#endif