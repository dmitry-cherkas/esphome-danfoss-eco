#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"
#include "properties.h"
#include "helpers.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco
    {
        bool DeviceProperty::init_handle(esphome::ble_client::BLEClient *client)
        {
            ESP_LOGD(TAG, "[%s] resolving handler for service=%s, characteristic=%s", client->address_str().c_str(), this->service_uuid.to_string().c_str(), this->characteristic_uuid.to_string().c_str());
            auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
            if (chr == nullptr)
            {
                ESP_LOGW(TAG, "[%s] characteristic uuid=%s not found", client->address_str().c_str(), this->characteristic_uuid.to_string().c_str());
                return false;
            }

            this->handle = chr->handle;
            return true;
        }

        void NameProperty::read(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            uint8_t *name = decrypt(value, value_len);
            ESP_LOGD(TAG, "[%s] device name: %s", component->parent()->address_str().c_str(), name);
            // std::string name_str((char *)name);
            // this->set_name(name_str); TODO - this is too late
        }

        void BatteryProperty::read(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            uint8_t battery_level = value[0];
            ESP_LOGD(TAG, "[%s] battery level: %d %%", component->parent()->address_str().c_str(), battery_level);
            component->battery_level()->publish_state(battery_level);
        }

        void TemperatureProperty::read(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            auto data = TemperatureData(value, value_len);
            ESP_LOGD(TAG, "[%s] Current room temperature: %2.1f°C, Set point temperature: %2.1f°C", component->parent()->address_str().c_str(), data.room_temperature, data.target_temperature);
            component->temperature()->publish_state(data.room_temperature);

            // apply read configuration to the component
            component->action = (data.room_temperature > data.target_temperature) ? climate::ClimateAction::CLIMATE_ACTION_IDLE : climate::ClimateAction::CLIMATE_ACTION_HEATING;
            component->target_temperature = data.target_temperature;
            component->current_temperature = data.room_temperature;
            component->publish_state();
        }

        void SettingsProperty::read(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            uint8_t *settings = decrypt(value, value_len);
            uint8_t config_bits = settings[0];

            bool adaptable_regulation = parse_bit(config_bits, 0);
            ESP_LOGD(TAG, "[%s] adaptable_regulation: %d", component->parent()->address_str().c_str(), adaptable_regulation);

            bool vertical_intallation = parse_bit(config_bits, 2);
            ESP_LOGD(TAG, "[%s] vertical_intallation: %d", component->parent()->address_str().c_str(), vertical_intallation);

            bool display_flip = parse_bit(config_bits, 3);
            ESP_LOGD(TAG, "[%s] display_flip: %d", component->parent()->address_str().c_str(), display_flip);

            bool slow_regulation = parse_bit(config_bits, 4);
            ESP_LOGD(TAG, "[%s] slow_regulation: %d", component->parent()->address_str().c_str(), slow_regulation);

            bool valve_installed = parse_bit(config_bits, 6);
            ESP_LOGD(TAG, "[%s] valve_installed: %d", component->parent()->address_str().c_str(), valve_installed);

            bool lock_control = parse_bit(config_bits, 7);
            ESP_LOGD(TAG, "[%s] lock_control: %d", component->parent()->address_str().c_str(), lock_control);

            float temperature_min = settings[1] / 2.0f;
            ESP_LOGD(TAG, "[%s] temperature_min: %2.1f°C", component->parent()->address_str().c_str(), temperature_min);

            float temperature_max = settings[2] / 2.0f;
            ESP_LOGD(TAG, "[%s] temperature_max: %2.1f°C", component->parent()->address_str().c_str(), temperature_max);

            float frost_protection_temperature = settings[3] / 2.0f;
            ESP_LOGD(TAG, "[%s] frost_protection_temperature: %2.1f°C", component->parent()->address_str().c_str(), frost_protection_temperature);
            // TODO add frost_protection_temperature sensor (OR CONTROL?)

            DeviceMode schedule_mode = (DeviceMode)settings[4];
            ESP_LOGD(TAG, "[%s] schedule_mode: %d", component->parent()->address_str().c_str(), schedule_mode);
            component->mode = to_climate_mode(schedule_mode);

            float vacation_temperature = settings[5] / 2.0f;
            ESP_LOGD(TAG, "[%s] vacation_temperature: %2.1f°C", component->parent()->address_str().c_str(), vacation_temperature);
            // TODO add vacation_temperature sensor (OR CONTROL?)

            // vacation mode can be enabled directly with schedule_mode, or planned with below dates
            int vacation_from = parse_int(settings, 6);
            ESP_LOGD(TAG, "[%s] vacation_from: %d", component->parent()->address_str().c_str(), vacation_from);

            int vacation_to = parse_int(settings, 10);
            ESP_LOGD(TAG, "[%s] vacation_to: %d", component->parent()->address_str().c_str(), vacation_to);

            // apply read configuration to the component
            component->set_visual_min_temperature_override(temperature_min);
            component->set_visual_max_temperature_override(temperature_max);
            component->publish_state();
        }

        climate::ClimateMode SettingsProperty::to_climate_mode(DeviceMode schedule_mode)
        {
            switch (schedule_mode)
            {
            case DeviceMode::MANUAL:
                return climate::ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

            case DeviceMode::VACATION:
            case DeviceMode::SCHEDULED:
                return climate::ClimateMode::CLIMATE_MODE_AUTO;

            case DeviceMode::HOLD:
                return climate::ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

            default:
                ESP_LOGW(TAG, "unexpected schedule_mode: %d", schedule_mode);
                return climate::ClimateMode::CLIMATE_MODE_HEAT; // unknown
            }
        }

        void CurrentTimeProperty::read(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            uint8_t *current_time = decrypt(value, value_len);
            int local_time = parse_int(current_time, 0);
            int time_offset = parse_int(current_time, 4);
            ESP_LOGD(TAG, "[%s] local_time: %d, time_offset: %d", component->parent()->address_str().c_str(), local_time, time_offset);
        }
    } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32