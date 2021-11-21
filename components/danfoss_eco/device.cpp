#include "device.h"
#include <xxtea-lib.h>

#ifdef USE_ESP32

namespace esphome
{
  namespace danfoss_eco
  {

    using namespace esphome::climate;

    void Device::dump_config() { LOG_CLIMATE(TAG, "Danfoss Eco eTRV", this); }

    void Device::setup()
    {
      // 1. Initialize device state
      this->state_ = make_unique<DeviceState>();

      // 2. Setup encryption key
      auto status = xxtea_setup_key(this->secret_, 16);
      if (status != XXTEA_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "xxtea_setup_key failed, status: %d", status);
        this->mark_failed();
        return;
      }
    }

    void Device::loop() {}

    void Device::control(const ClimateCall &call)
    {
    }

    void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
    {
      switch (event)
      {
      case ESP_GATTC_OPEN_EVT:
        if (param->open.status == ESP_GATT_OK)
        {
          ESP_LOGI(TAG, "[%s] connected", this->parent()->address_str().c_str());
        }
        break;

      case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(TAG, "[%s] disconnected", this->parent()->address_str().c_str());
        break;

      case ESP_GATTC_SEARCH_CMPL_EVT:
        this->write_pin();
        break;

      case ESP_GATTC_WRITE_CHAR_EVT:
        if (param->write.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "[%s] failed to write characteristic: handle=%02x, status=%d", this->parent()->address_str().c_str(), param->write.handle, param->write.status);
        }
        else if (param->write.handle == this->pin_chr_handle_)
        { // PIN OK
          this->request_name();
          // TODO - request more chars here as well
        }
        break;

      case ESP_GATTC_READ_CHAR_EVT:
        if (param->read.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "[%s] failed to read characteristic: handle=%02x, status=%d", this->parent()->address_str().c_str(), param->read.handle, param->read.status);
        }
        else if (param->read.handle == this->name_char_handle_)
        {
          this->status_clear_warning();
          this->parse_data_(param->read.value, param->read.value_len);

          // TODO - check if any requests pending. if not - disable the parent
          this->parent()->set_enabled(false);
        }
        break;

      default:
        break;
      }
    }

    void Device::write_pin()
    {
      auto pin_chr = this->parent()->get_characteristic(SERVICE_SETTINGS, CHARACTERISTIC_PIN);
      if (pin_chr == nullptr)
      {
        ESP_LOGE(TAG, "[%s] no settings service/PIN characteristic found at device, not a Danfoss Eco?", this->get_name().c_str());
        this->status_set_warning();
        return;
      }

      this->pin_chr_handle_ = pin_chr->handle;
      auto status = esp_ble_gattc_write_char(this->parent()->gattc_if,
                                             this->parent()->conn_id,
                                             this->pin_chr_handle_,
                                             sizeof(pin_code),
                                             pin_code,
                                             ESP_GATT_WRITE_TYPE_RSP,
                                             ESP_GATT_AUTH_REQ_NONE);
      if (status != ESP_OK)
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%02x", this->parent()->address_str().c_str(), status);
    }

    void Device::request_name()
    {
      ESP_LOGI(TAG, "[%s] reading Name characteristic", this->parent()->address_str().c_str());
      auto name_chr = this->parent()->get_characteristic(SERVICE_SETTINGS, CHARACTERISTIC_NAME);
      if (name_chr == nullptr)
      {
        ESP_LOGW(TAG, "[%s] no name characteristic found at device, not a Danfoss Eco?", this->get_name().c_str());
        this->status_set_warning();
        return;
      }
      this->name_char_handle_ = name_chr->handle;
      auto status = esp_ble_gattc_read_char(this->parent()->gattc_if,
                                            this->parent()->conn_id,
                                            name_chr->handle,
                                            ESP_GATT_AUTH_REQ_NONE);
      if (status != ESP_OK)
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, status=%02x", this->parent()->address_str().c_str(), status);
    }

    void Device::parse_data_(uint8_t *value, uint16_t value_len)
    {
      std::string s = hexencode(value, value_len);
      ESP_LOGI(TAG, "[%s] raw value: %s", this->parent()->address_str().c_str(), s.c_str());

      uint8_t buffer[value_len];
      reverse_chunks(value, value_len, buffer);
      std::string s1 = hexencode(buffer, value_len);
      ESP_LOGI(TAG, "[%s] reversed bytes: %s", this->parent()->address_str().c_str(), s1.c_str());

      // Perform Decryption
      auto xxtea_status = xxtea_decrypt(buffer, value_len);
      if (xxtea_status != XXTEA_STATUS_SUCCESS)
      {
        ESP_LOGW(TAG, "[%s] xxtea_decrypt failed, status=%d", this->parent()->address_str().c_str(), xxtea_status);
        return;
      }
      else
      {
        std::string s2 = hexencode(buffer, value_len);
        ESP_LOGI(TAG, "[%s] decrypted bytes: %s", this->parent()->address_str().c_str(), s2.c_str());

        reverse_chunks(buffer, value_len, value);
        ESP_LOGI(TAG, "[%s] parsed value: %s", this->parent()->address_str().c_str(), value);
      }
    }

    // Update is triggered by on defined polling interval (see PollingComponent) to trigger state update report
    void Device::update()
    {
      if (this->node_state != esp32_ble_tracker::ClientState::ESTABLISHED)
      {
        if (!parent()->enabled)
        {
          ESP_LOGI(TAG, "[%s] received update request, reconnecting to device", this->parent()->address_str().c_str());
          parent()->set_enabled(true);
          parent()->connect();
        }
        else
        {
          ESP_LOGI(TAG, "[%s] received update request, connection already in progress", this->parent()->address_str().c_str());
        }
      }
    }

    void Device::set_secret_key(const char *str)
    {
      this->secret_ = parse_hex_str(str, 32);

      std::string hex_str = hexencode(this->secret_, 16);
      ESP_LOGI(TAG, "[%s] secret bytes: %s", this->parent()->address_str().c_str(), hex_str.c_str());
    }

  } // namespace danfoss_eco
} // namespace esphome

#endif
