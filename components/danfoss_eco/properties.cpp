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
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, handle=%#04x, status=%01x", client->address_str().c_str(), this->handle, status);

            return status == ESP_OK;
        }

        bool WritableProperty::write_request(esphome::ble_client::BLEClient *client, uint8_t *data, uint16_t data_len)
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
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, handle=%#04x, status=%01x", client->address_str().c_str(), this->handle, status);

            return status == ESP_OK;
        }

        bool WritableProperty::write_request(esphome::ble_client::BLEClient *client)
        {
            WritableData* writableData = static_cast<WritableData*>(this->data.get());
            uint8_t buff[this->data->length]{0};
            writableData->pack(buff);
            return this->write_request(client, buff, sizeof(buff));
        }

        void BatteryProperty::update_state(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            uint8_t battery_level = value[0];
            ESP_LOGD(TAG, "[%s] battery level: %d %%", component->get_name().c_str(), battery_level);
            if (component->battery_level() != nullptr)
                component->battery_level()->publish_state(battery_level);
        }

        void TemperatureProperty::update_state(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            auto t_data = new TemperatureData(this->xxtea_, value, value_len);
            this->data.reset(t_data);

            ESP_LOGD(TAG, "[%s] Current room temperature: %2.1f°C, Set point temperature: %2.1f°C", component->get_name().c_str(), t_data->room_temperature, t_data->target_temperature);
            if (component->temperature() != nullptr)
                component->temperature()->publish_state(t_data->room_temperature);

            // apply read configuration to the component
            // TODO component->action should consider "open window detection" feature of Danfoss Eco
            component->action = (t_data->room_temperature > t_data->target_temperature) ? climate::ClimateAction::CLIMATE_ACTION_IDLE : climate::ClimateAction::CLIMATE_ACTION_HEATING;
            component->target_temperature = t_data->target_temperature;
            component->current_temperature = t_data->room_temperature;
            component->publish_state();
        }

        void SettingsProperty::update_state(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            auto s_data = new SettingsData(this->xxtea_, value, value_len);
            this->data.reset(s_data);

            const char *name = component->get_name().c_str();
            ESP_LOGD(TAG, "[%s] adaptable_regulation: %d", name, s_data->get_adaptable_regulation());
            ESP_LOGD(TAG, "[%s] vertical_intallation: %d", name, s_data->get_vertical_intallation());
            ESP_LOGD(TAG, "[%s] display_flip: %d", name, s_data->get_display_flip());
            ESP_LOGD(TAG, "[%s] slow_regulation: %d", name, s_data->get_slow_regulation());
            ESP_LOGD(TAG, "[%s] valve_installed: %d", name, s_data->get_valve_installed());
            ESP_LOGD(TAG, "[%s] lock_control: %d", name, s_data->get_lock_control());
            ESP_LOGD(TAG, "[%s] temperature_min: %2.1f°C", name, s_data->temperature_min);
            ESP_LOGD(TAG, "[%s] temperature_max: %2.1f°C", name, s_data->temperature_max);
            ESP_LOGD(TAG, "[%s] frost_protection_temperature: %2.1f°C", name, s_data->frost_protection_temperature);
            ESP_LOGD(TAG, "[%s] schedule_mode: %d", name, s_data->device_mode);
            ESP_LOGD(TAG, "[%s] vacation_temperature: %2.1f°C", name, s_data->vacation_temperature);
            ESP_LOGD(TAG, "[%s] vacation_from: %d", name, (int)s_data->vacation_from);
            ESP_LOGD(TAG, "[%s] vacation_to: %d", name, (int)s_data->vacation_to);

            // apply read configuration to the component
            component->mode = s_data->device_mode;
            component->set_visual_min_temperature_override(s_data->temperature_min);
            component->set_visual_max_temperature_override(s_data->temperature_max);
            component->publish_state();
        }

        void ErrorsProperty::update_state(MyComponent *component, uint8_t *value, uint16_t value_len)
        {
            auto e_data = new ErrorsData(this->xxtea_, value, value_len);
            this->data.reset(e_data);

            const char *name = component->get_name().c_str();

            ESP_LOGD(TAG, "[%s] E9_VALVE_DOES_NOT_CLOSE: %d", name, e_data->E9_VALVE_DOES_NOT_CLOSE);
            ESP_LOGD(TAG, "[%s] E10_INVALID_TIME: %d", name, e_data->E10_INVALID_TIME);
            ESP_LOGD(TAG, "[%s] E14_LOW_BATTERY: %d", name, e_data->E14_LOW_BATTERY);
            ESP_LOGD(TAG, "[%s] E15_VERY_LOW_BATTERY: %d", name, e_data->E15_VERY_LOW_BATTERY);
        }

    } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32