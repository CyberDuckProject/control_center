#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <iostream>

enum class SensorType
{
    WaterTemperature = 1,
    WaterTurbidity,
    Dust,
    AtmosphericPressure,
    AtmosphericTemperature,
    AtmosphericHumidity
};
class SensorData
{
public:
    void add_reading(SensorType sensor, int64_t time, float reading)
    {
        // TODO: implement
        std::cout << "reading added: " << (int)sensor << " " << time << " " << reading << '\n';
    }
};

#endif