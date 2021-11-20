#include "eco2.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <xxtea-lib.h>

#ifdef USE_ESP32

namespace esphome
{
  namespace eco2
  {

    static const char *const TAG = "eco2";

    using namespace esphome::climate;

    void DanfossEco2::dump_config() { LOG_CLIMATE("", "Danfoss Eco eTRV", this); }

    void DanfossEco2::setup()
    {
      // 1. Initialize device state
      this->codec_ = make_unique<AnovaCodec>(); // TODO remove
      this->state_ = make_unique<DeviceState>();
      this->current_request_ = 0;

      // 2. Setup encryption key
      std::string sec_hex = hexencode(this->secret_, 16);
      ESP_LOGI(TAG, "[%s] secret bytes: %s", this->parent_->address_str().c_str(), sec_hex.c_str());

      auto status = xxtea_setup_key(this->secret_, 16);
      if (status != XXTEA_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "xxtea_setup_key failed, status: %d", status);
        this->mark_failed();
        return;
      }
    }

    void DanfossEco2::loop() {}

    void DanfossEco2::control(const ClimateCall &call)
    {
    }

    void DanfossEco2::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
    {
      switch (event)
      {
      case ESP_GATTC_CONNECT_EVT:
      {
        break;
      }
      case ESP_GATTC_DISCONNECT_EVT:
      {
        this->current_temperature = NAN;
        this->target_temperature = NAN;
        this->publish_state();
        break;
      }
      case ESP_GATTC_SEARCH_CMPL_EVT:
      {
        auto pinChr = this->parent_->get_characteristic(ECO2_SERVICE_SETTINGS, ECO2_CHARACTERISTIC_PIN);
        if (pinChr == nullptr)
        {
          ESP_LOGW(TAG, "[%s] No settings service found at device, not a Danfoss Eco?", this->get_name().c_str());
          break;
        }

        this->pin_char_handle_ = pinChr->handle;
        auto pinStatus =
            esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, pinChr->handle, sizeof(eco2Pin), eco2Pin, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
        if (pinStatus)
          ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(), pinStatus);
        else
          ESP_LOGI(TAG, "[%s] esp_ble_gattc_write_char PIN, status=%d", this->parent_->address_str().c_str(), pinStatus);

        break;
      }
      case ESP_GATTC_WRITE_CHAR_EVT:
      {
        if (param->write.status == ESP_GATT_OK && param->write.handle == this->pin_char_handle_)
        {
          ESP_LOGW(TAG, "[%s] Reading Name characteristic", this->get_name().c_str());

          auto nameChr = this->parent_->get_characteristic(ECO2_SERVICE_SETTINGS, ECO2_CHARACTERISTIC_NAME);
          if (nameChr == nullptr)
          {
            ESP_LOGW(TAG, "[%s] No name characteristic found at device, not a Danfoss Eco?", this->get_name().c_str());
            break;
          }
          this->name_char_handle_ = nameChr->handle;
          auto status = esp_ble_gattc_read_char(this->parent_->gattc_if, this->parent_->conn_id, nameChr->handle, ESP_GATT_AUTH_REQ_NONE);
          if (status)
            ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, status=%d", this->parent_->address_str().c_str(), status);
        }
        break;
      }
      case ESP_GATTC_READ_CHAR_EVT:
      {
        if (param->read.conn_id != this->parent()->conn_id)
          break;
        if (param->read.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "Error reading char at handle %d, status=%d", param->read.handle, param->read.status);
          break;
        }
        if (param->read.handle == this->name_char_handle_)
        {
          this->status_clear_warning();
          this->parse_data_(param->read.value, param->read.value_len);
        }
        break;
      }
      default:
        break;
      }
    }

    void reverse_chunks(byte data[], int len, byte *reversed_buf)
    {
      for (int i = 0; i < len; i += 4)
      {
        int l = min(4, len - i); // limit for a chunk, 4 or what's left
        for (int j = 0; j < l; j++)
        {
          reversed_buf[i + j] = data[i + (l - 1 - j)];
        }
      }
    }

    void DanfossEco2::parse_data_(uint8_t *value, uint16_t value_len)
    {
      std::string s = hexencode(value, value_len);
      ESP_LOGI(TAG, "[%s] raw value: %s", this->parent_->address_str().c_str(), s.c_str());

      uint8_t buffer[value_len];
      reverse_chunks(value, value_len, buffer);
      std::string s1 = hexencode(buffer, value_len);
      ESP_LOGI(TAG, "[%s] reversed bytes: %s", this->parent_->address_str().c_str(), s1.c_str());

      // Perform Decryption
      auto xxtea_status = xxtea_decrypt(buffer, value_len);
      if (xxtea_status != XXTEA_STATUS_SUCCESS)
      {
        ESP_LOGW(TAG, "[%s] xxtea_decrypt failed, status=%d", this->parent_->address_str().c_str(), xxtea_status);
        return;
      }
      else
      {
        std::string s2 = hexencode(buffer, value_len);
        ESP_LOGI(TAG, "[%s] decrypted bytes: %s", this->parent_->address_str().c_str(), s2.c_str());

        reverse_chunks(buffer, value_len, value);
        ESP_LOGI(TAG, "[%s] parsed value: %s", this->parent_->address_str().c_str(), value);
      }
    }

    void DanfossEco2::update()
    { /*
      // Poller is asking us for sensor data, initiate the connection
      if (this->node_state != espbt::ClientState::ESTABLISHED)
      {
        // initiate the connection sequence
        this->client->connect();
        // remember to request the data
        this->state_->set_pending_state_request(true);
        return;
      }
      else
      {
        // we are connected, so let's request the state data right away
      }*/
    }

    void DanfossEco2::set_secret_key(const char *str)
    {
      this->secret_ = this->codec_->bytesFromHexStr(str, 32);
    }

  } // namespace eco2
} // namespace esphome

#endif
