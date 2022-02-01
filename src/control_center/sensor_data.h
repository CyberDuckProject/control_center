#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

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
private:
    inline static constexpr size_t sensor_count = static_cast<size_t>(SensorType::END) - static_cast<size_t>(SensorType::BEGIN);
public:
    void add_reading(SensorType sensor, int64_t time, float reading)
    {
        // TODO: implement
        std::cout << "reading added: " << (int)sensor << " " << time << " " << reading << '\n';
    }
};

#endif