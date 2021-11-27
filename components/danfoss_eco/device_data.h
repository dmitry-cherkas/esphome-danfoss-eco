#pragma once

namespace esphome
{
    namespace danfoss_eco
    {
        struct DeviceData
        {
            uint16_t length;
            virtual uint8_t *pack() = 0;

            DeviceData(uint16_t l) : length(l) {}
            virtual ~DeviceData() {}
        };

        struct TemperatureData : public DeviceData
        {
            float target_temperature;
            float room_temperature;

            TemperatureData(float target_temperature) : DeviceData(8) { this->target_temperature = target_temperature; }

            TemperatureData(uint8_t *raw_data, uint16_t value_len) : DeviceData(8)
            {
                uint8_t *temperatures = decrypt(raw_data, value_len);
                this->target_temperature = temperatures[0] / 2.0f;
                this->room_temperature = temperatures[1] / 2.0f;
            }

            uint8_t *pack()
            {
                uint8_t buff[length] = {(uint8_t)(target_temperature * 2), 0};
                return encrypt(buff, sizeof(buff));
            }
        };

    } // namespace danfoss_eco
} // namespace esphome
