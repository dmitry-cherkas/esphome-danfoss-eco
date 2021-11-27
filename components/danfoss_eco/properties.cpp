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

        bool DeviceProperty::read_request(esphome::ble_client::BLEClient *client)
        {
            auto status = esp_ble_gattc_read_char(client->gattc_if,
                                                  client->conn_id,
                                                  this->handle,
                                                  ESP_GATT_AUTH_REQ_NONE);
            if (status != ESP_OK)
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, status=%01x", client->address_str().c_str(), status);

            return status == ESP_OK;
        }

        bool DeviceProperty::write_request(esphome::ble_client::BLEClient *client, uint8_t *data, uint16_t data_len)
        {
            ESP_LOGD(TAG, "[%s] write_request: handle=%#04x, data=%s", client->address_str().c_str(), this->handle, hexencode(data, data_len).c_str());

            auto status = esp_ble_gattc_write_char(client->gattc_if,
                                                   client->conn_id,
                                                   this->handle,
                                                   data_len,
                                                   data,
                                                   ESP_GATT_WRITE_TYPE_RSP,
                                                   ESP_GATT_AUTH_REQ_NONE);
            if (status != ESP_OK)
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%01x", client->address_str().c_str(), status);

            return status == ESP_OK;
        }

        bool DeviceProperty::write_request(esphome::ble_client::BLEClient *client)
        {
            uint8_t buff[this->data->length]{0};
            this->data->pack(buff);
            return this->write_request(client, buff, sizeof(buff));
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
            auto t_data = new TemperatureData(value, value_len);
            this->data.reset(t_data);

            ESP_LOGD(TAG, "[%s] Current room temperature: %2.1f°C, Set point temperature: %2.1f°C", component->parent()->address_str().c_str(), t_data->room_temperature, t_data->target_temperature);
            component->temperature()->publish_state(t_data->room_temperature);

            // apply read configuration to the component
            component->action = (t_data->room_temperature > t_data->target_temperature) ? climate::ClimateAction::CLIMATE_ACTION_IDLE : climate::ClimateAction::CLIMATE_ACTION_HEATING;
            component->target_temperature = t_data->target_temperature;
            component->current_temperature = t_data->room_temperature;
            component->publish_state();
        }

        void SettingsProperty::read(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            auto s_data = new SettingsData(value, value_len);
            this->data.reset(s_data);

            const char *addr = component->parent()->address_str().c_str();
            ESP_LOGD(TAG, "[%s] adaptable_regulation: %d", addr, s_data->get_adaptable_regulation());
            ESP_LOGD(TAG, "[%s] vertical_intallation: %d", addr, s_data->get_vertical_intallation());
            ESP_LOGD(TAG, "[%s] display_flip: %d", addr, s_data->get_display_flip());
            ESP_LOGD(TAG, "[%s] slow_regulation: %d", addr, s_data->get_slow_regulation());
            ESP_LOGD(TAG, "[%s] valve_installed: %d", addr, s_data->get_valve_installed());
            ESP_LOGD(TAG, "[%s] lock_control: %d", addr, s_data->get_lock_control());
            ESP_LOGD(TAG, "[%s] temperature_min: %2.1f°C", addr, s_data->temperature_min);
            ESP_LOGD(TAG, "[%s] temperature_max: %2.1f°C", addr, s_data->temperature_max);
            ESP_LOGD(TAG, "[%s] frost_protection_temperature: %2.1f°C", addr, s_data->frost_protection_temperature);
            ESP_LOGD(TAG, "[%s] schedule_mode: %d", addr, s_data->device_mode);
            ESP_LOGD(TAG, "[%s] vacation_temperature: %2.1f°C", addr, s_data->vacation_temperature);
            ESP_LOGD(TAG, "[%s] vacation_from: %d", addr, (int)s_data->vacation_from);
            ESP_LOGD(TAG, "[%s] vacation_to: %d", addr, (int)s_data->vacation_to);

            // apply read configuration to the component
            component->mode = s_data->device_mode;
            component->set_visual_min_temperature_override(s_data->temperature_min);
            component->set_visual_max_temperature_override(s_data->temperature_max);
            component->publish_state();
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