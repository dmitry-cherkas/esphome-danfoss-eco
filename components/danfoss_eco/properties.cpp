#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"

#include "properties.h"
#include "helpers.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco
    {
        bool DeviceProperty::init_handle(BLEClient *client)
        {
            ESP_LOGV(TAG, "[%s] resolving handler for service=%s, characteristic=%s", this->component_->get_name().c_str(), this->service_uuid.to_string().c_str(), this->characteristic_uuid.to_string().c_str());
            auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
            if (chr == nullptr)
            {
                ESP_LOGW(TAG, "[%s] characteristic uuid=%s not found", this->component_->get_name().c_str(), this->characteristic_uuid.to_string().c_str());
                return false;
            }

            this->handle = chr->handle;
            return true;
        }

        bool DeviceProperty::read_request(BLEClient *client)
        {
            auto status = esp_ble_gattc_read_char(client->get_gattc_if(),
                                                  client->get_conn_id(),
                                                  this->handle,
                                                  ESP_GATT_AUTH_REQ_NONE);
            if (status != ESP_OK)
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, handle=%#04x, status=%01x", this->component_->get_name().c_str(), this->handle, status);

            return status == ESP_OK;
        }

        bool WritableProperty::write_request(BLEClient *client, uint8_t *data, uint16_t data_len)
        {
            ESP_LOGD(TAG, "[%s] write_request: handle=%#04x, data=%s", this->component_->get_name().c_str(), this->handle, format_hex_pretty(data, data_len).c_str());

            auto status = esp_ble_gattc_write_char(client->get_gattc_if(),
                                                   client->get_conn_id(),
                                                   this->handle,
                                                   data_len,
                                                   data,
                                                   ESP_GATT_WRITE_TYPE_RSP,
                                                   ESP_GATT_AUTH_REQ_NONE);
            if (status != ESP_OK)
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, handle=%#04x, status=%01x", this->component_->get_name().c_str(), this->handle, status);

            return status == ESP_OK;
        }

        bool WritableProperty::write_request(BLEClient *client)
        {
            WritableData *writableData = static_cast<WritableData *>(this->data.get());
            uint8_t buff[this->data->length]{0};
            writableData->pack(buff);
            return this->write_request(client, buff, sizeof(buff));
        }

        void BatteryProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            uint8_t battery_level = value[0];
            ESP_LOGD(TAG, "[%s] battery level: %d %%", this->component_->get_name().c_str(), battery_level);
            if (this->component_->battery_level() != nullptr)
                this->component_->battery_level()->publish_state(battery_level);
        }

        void TemperatureProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            auto t_data = new TemperatureData(this->xxtea_, value, value_len);
            this->data.reset(t_data);

            ESP_LOGD(TAG, "[%s] Current room temperature: %2.1f°C, Set point temperature: %2.1f°C", this->component_->get_name().c_str(), t_data->room_temperature, t_data->target_temperature);
            if (this->component_->temperature() != nullptr)
                this->component_->temperature()->publish_state(t_data->room_temperature);

            // apply read configuration to the component
            // TODO component->action should consider "open window detection" feature of Danfoss Eco
            this->component_->action = (t_data->room_temperature > t_data->target_temperature) ? climate::ClimateAction::CLIMATE_ACTION_IDLE : climate::ClimateAction::CLIMATE_ACTION_HEATING;
            this->component_->target_temperature = t_data->target_temperature;
            this->component_->current_temperature = t_data->room_temperature;
            this->component_->publish_state();
        }

        void SettingsProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            auto s_data = new SettingsData(this->xxtea_, value, value_len);
            this->data.reset(s_data);

            const char *name = this->component_->get_name().c_str();
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
            this->component_->mode = s_data->device_mode;
            this->component_->set_visual_min_temperature_override(s_data->temperature_min);
            this->component_->set_visual_max_temperature_override(s_data->temperature_max);
            this->component_->publish_state();
        }

        void ErrorsProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            auto e_data = new ErrorsData(this->xxtea_, value, value_len);
            this->data.reset(e_data);

            const char *name = this->component_->get_name().c_str();

            ESP_LOGD(TAG, "[%s] E9_VALVE_DOES_NOT_CLOSE: %d", name, e_data->E9_VALVE_DOES_NOT_CLOSE);
            ESP_LOGD(TAG, "[%s] E10_INVALID_TIME: %d", name, e_data->E10_INVALID_TIME);
            ESP_LOGD(TAG, "[%s] E14_LOW_BATTERY: %d", name, e_data->E14_LOW_BATTERY);
            ESP_LOGD(TAG, "[%s] E15_VERY_LOW_BATTERY: %d", name, e_data->E15_VERY_LOW_BATTERY);

            // TODO: it would be great to add actual error code to binary_sensor state attributes, but I'm not sure how to achieve that
            if (this->component_->problems() != nullptr)
                this->component_->problems()->publish_state(e_data->E9_VALVE_DOES_NOT_CLOSE || e_data->E10_INVALID_TIME || e_data->E14_LOW_BATTERY || e_data->E15_VERY_LOW_BATTERY);
        }

        bool SecretKeyProperty::init_handle(BLEClient *client)
        {
            if (this->xxtea_->status() != XXTEA_STATUS_NOT_INITIALIZED)
            {
                ESP_LOGD(TAG, "[%s] xxtea is initialized, will not request a read of secret_key", this->component_->get_name().c_str());
                return true;
            }

            auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
            if (chr != nullptr)
            {
                this->handle = chr->handle;
                return true;
            }

            ESP_LOGW(TAG, "[%s] Danfoss Eco hardware button was not pressed, unable to read the secret key", this->component_->get_name().c_str());
            this->handle = INVALID_HANDLE;
            return false;
        }

        void SecretKeyProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            if (value_len != SECRET_KEY_LENGTH)
            {
                ESP_LOGE(TAG, "[%s] Unexpected secret_key length: %d", this->component_->get_name().c_str(), value_len);
                return;
            }

            char key_str[SECRET_KEY_LENGTH * 2 + 1];
            encode_hex(value, value_len, key_str);

            ESP_LOGI(TAG, "[%s] Consider adding below line to your danfoss_eco config:", this->component_->get_name().c_str());
            ESP_LOGI(TAG, "[%s] secret_key: %s", this->component_->get_name().c_str(), key_str);
            this->component_->set_secret_key(value, true);
        }

    } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32
