#pragma once

#include "esphome/core/log.h"

#include "helpers.h"
#include "xxtea.h"

namespace esphome
{
    namespace danfoss_eco
    {
        using namespace std;
        using namespace climate;

        struct DeviceData
        {
            uint16_t length;

            DeviceData(uint16_t l, shared_ptr<Xxtea> &xxtea) : length(l), xxtea_(xxtea) {}
            virtual ~DeviceData()
            {
            }

        protected:
            shared_ptr<Xxtea> xxtea_;
        };

        struct WritableData : public DeviceData
        {
            WritableData(uint16_t l, shared_ptr<Xxtea> &xxtea) : DeviceData(8, xxtea) {}
            virtual void pack(uint8_t *) = 0;
        };

        struct TemperatureData : public WritableData
        {
            float target_temperature;
            float room_temperature;

            TemperatureData(shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(8, xxtea)
            {
                uint8_t *temperatures = decrypt(this->xxtea_, raw_data, value_len);
                this->target_temperature = temperatures[0] / 2.0f;
                this->room_temperature = temperatures[1] / 2.0f;
            }

            void pack(uint8_t *buff)
            {
                buff[0] = (uint8_t)(target_temperature * 2);
                buff[1] = (uint8_t)(room_temperature * 2);

                encrypt(this->xxtea_, buff, length);
            }
        };

        struct SettingsData : public WritableData
        {
            enum DeviceMode
            {
                MANUAL = 0,
                SCHEDULED = 1,
                VACATION = 3,
                HOLD = 5
            };

            bool get_adaptable_regulation() { return parse_bit(this->settings_[0], 0); }
            bool get_vertical_intallation() { return parse_bit(this->settings_[0], 2); }
            bool get_display_flip() { return parse_bit(this->settings_[0], 3); }
            bool get_slow_regulation() { return parse_bit(this->settings_[0], 4); }
            bool get_valve_installed() { return parse_bit(this->settings_[0], 6); }
            bool get_lock_control() { return parse_bit(this->settings_[0], 7); }

            void set_adaptable_regulation(bool state) { set_bit(this->settings_[0], 0, state); }
            void set_vertical_intallation(bool state) { set_bit(this->settings_[0], 2, state); }
            void set_display_flip(bool state) { set_bit(this->settings_[0], 3, state); }
            void set_slow_regulation(bool state) { set_bit(this->settings_[0], 4, state); }
            void set_valve_installed(bool state) { set_bit(this->settings_[0], 6, state); }
            void set_lock_control(bool state) { set_bit(this->settings_[0], 7, state); }

            ClimateMode device_mode;

            float temperature_min;
            float temperature_max;
            float frost_protection_temperature;
            float vacation_temperature;
            // vacation mode can be enabled directly with schedule_mode, or planned with below dates
            time_t vacation_from; // utc
            time_t vacation_to;   // utc

            SettingsData(shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(16, xxtea)
            {
                uint8_t *settings = decrypt(this->xxtea_, raw_data, value_len);

                this->settings_ = (uint8_t *)malloc(length);
                memcpy(this->settings_, (const char *)settings, length);

                this->temperature_min = settings[1] / 2.0f;
                this->temperature_max = settings[2] / 2.0f;
                this->frost_protection_temperature = settings[3] / 2.0f;
                this->device_mode = to_climate_mode((DeviceMode)settings[4]);
                this->vacation_temperature = settings[5] / 2.0f;

                this->vacation_from = parse_int(settings, 6);
                this->vacation_to = parse_int(settings, 10);
            }

            ~SettingsData() { free(this->settings_); }

            ClimateMode to_climate_mode(DeviceMode mode)
            {
                switch (mode)
                {
                case MANUAL:
                case HOLD: // TODO: not sure, what HOLD represents
                    return ClimateMode::CLIMATE_MODE_HEAT;

                case SCHEDULED:
                case VACATION:
                    return ClimateMode::CLIMATE_MODE_AUTO;

                default:
                    ESP_LOGW(TAG, "unexpected schedule_mode: %d", mode);
                    return ClimateMode::CLIMATE_MODE_HEAT; // reasonable default
                }
            }

            void pack(uint8_t *buff)
            {
                memcpy(buff, (const char *)this->settings_, length);

                buff[1] = (uint8_t)(this->temperature_min * 2);
                buff[2] = (uint8_t)(this->temperature_max * 2);
                buff[3] = (uint8_t)(this->frost_protection_temperature * 2);
                if (this->device_mode == ClimateMode::CLIMATE_MODE_AUTO)
                    buff[4] = DeviceMode::SCHEDULED;
                else
                    buff[4] = DeviceMode::MANUAL;
                buff[5] = (uint8_t)(this->vacation_temperature * 2);

                write_int(buff, 6, this->vacation_from);
                write_int(buff, 10, this->vacation_to);

                encrypt(this->xxtea_, buff, length);
            }

        private:
            uint8_t *settings_;
        };

        struct ErrorsData : public DeviceData
        {
            bool E9_VALVE_DOES_NOT_CLOSE;
            bool E10_INVALID_TIME;
            bool E14_LOW_BATTERY;
            bool E15_VERY_LOW_BATTERY;

            ErrorsData(shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : DeviceData(8, xxtea)
            {
                // unsigned short error;
                // unsigned char padding[6];
                uint16_t errors = parse_short(decrypt(this->xxtea_, raw_data, value_len), 0);

                E9_VALVE_DOES_NOT_CLOSE = parse_bit(errors, 8);
                E10_INVALID_TIME = parse_bit(errors, 9);
                E14_LOW_BATTERY = parse_bit(errors, 13);
                E15_VERY_LOW_BATTERY = parse_bit(errors, 14);
            }
        };

    } // namespace danfoss_eco
} // namespace esphome
