#pragma once

#include "esphome/core/log.h"

namespace esphome
{
    namespace danfoss_eco
    {
        struct DeviceData
        {
            uint16_t length;
            virtual uint8_t *pack() = 0;

            DeviceData(uint16_t l) : length(l) {}
            virtual ~DeviceData()
            {
                ESP_LOGW(TAG, "DeviceData destroyed"); // TODO: remove debug output
            }
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

        struct SettingsData : public DeviceData
        {
            enum DeviceMode
            {
                MANUAL = 0,
                SCHEDULED = 1,
                VACATION = 3,
                HOLD = 5 // TODO: what is the meaning of this mode?
            };

            bool adaptable_regulation;
            bool vertical_intallation;
            bool display_flip;
            bool slow_regulation;
            bool valve_installed;
            bool lock_control;
            float temperature_min;
            float temperature_max;
            float frost_protection_temperature;
            climate::ClimateMode device_mode;
            float vacation_temperature;
            // vacation mode can be enabled directly with schedule_mode, or planned with below dates
            time_t vacation_from; // utc
            time_t vacation_to;   // utc

            SettingsData(uint8_t *raw_data, uint16_t value_len) : DeviceData(16)
            {
                uint8_t *settings = decrypt(raw_data, value_len);
                uint8_t config_bits = settings[0];

                this->adaptable_regulation = parse_bit(config_bits, 0);
                this->vertical_intallation = parse_bit(config_bits, 2);
                this->display_flip = parse_bit(config_bits, 3);
                this->slow_regulation = parse_bit(config_bits, 4);
                this->valve_installed = parse_bit(config_bits, 6);
                this->lock_control = parse_bit(config_bits, 7);
                this->temperature_min = settings[1] / 2.0f;
                this->temperature_max = settings[2] / 2.0f;
                this->frost_protection_temperature = settings[3] / 2.0f;
                this->device_mode = to_climate_mode((DeviceMode)settings[4]);
                this->vacation_temperature = settings[5] / 2.0f;
                this->vacation_from = parse_int(settings, 6);
                this->vacation_to = parse_int(settings, 10);
            }

            climate::ClimateMode to_climate_mode(DeviceMode mode)
            {
                switch (mode)
                {
                case MANUAL:
                    return climate::ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

                case SCHEDULED:
                case VACATION:
                    return climate::ClimateMode::CLIMATE_MODE_AUTO;

                case DeviceMode::HOLD:
                    return climate::ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

                default:
                    ESP_LOGW(TAG, "unexpected schedule_mode: %d", mode);
                    return climate::ClimateMode::CLIMATE_MODE_HEAT; // unknown
                }
            }

            uint8_t *pack()
            {
                uint8_t buff[length] = {0};
                set_bit(buff[0], 0, this->adaptable_regulation);
                set_bit(buff[0], 2, this->vertical_intallation);
                set_bit(buff[0], 3, this->display_flip);
                set_bit(buff[0], 4, this->slow_regulation);
                set_bit(buff[0], 6, this->valve_installed);
                set_bit(buff[0], 7, this->lock_control);

                buff[1] = (uint8_t)(this->temperature_min * 2);
                buff[2] = (uint8_t)(this->temperature_max * 2);
                buff[3] = (uint8_t)(this->frost_protection_temperature * 2);
                buff[4] = device_mode; // FIXME
                buff[5] = (uint8_t)(this->vacation_temperature * 2);

                write_int(buff, 6, this->vacation_from);
                write_int(buff, 10, this->vacation_to);

                return encrypt(buff, sizeof(buff));
            }
        };

        struct NameData : public DeviceData {        };
        struct BatteryData : public DeviceData {        };
        struct CurrentTimeData : public DeviceData {        };
        

    } // namespace danfoss_eco
} // namespace esphome
