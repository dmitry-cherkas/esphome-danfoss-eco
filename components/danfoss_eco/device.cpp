#include "device.h"

#ifdef USE_ESP32

namespace esphome
{
  namespace danfoss_eco
  {
    void Device::setup()
    {
      // Setup encryption key
      this->xxtea = std::make_shared<Xxtea>(this->secret_, SECRET_KEY_LENGTH);
      if (xxtea->status() != XXTEA_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "xxtea_setup_key failed, status: %d", xxtea->status());
        this->mark_failed();
        return;
      }

      this->p_pin = std::make_shared<WritableProperty>(xxtea, SERVICE_SETTINGS, CHARACTERISTIC_PIN);
      this->p_battery = std::make_shared<BatteryProperty>(xxtea);
      this->p_temperature = std::make_shared<TemperatureProperty>(xxtea);
      this->p_settings = std::make_shared<SettingsProperty>(xxtea);
      this->p_errors = std::make_shared<ErrorsProperty>(xxtea);

      this->properties = {this->p_pin, this->p_battery, this->p_temperature, this->p_settings, this->p_errors};

      // pretend, we have already discovered the device
      copy_address(this->parent()->address, this->parent()->remote_bda);
    }

    void Device::loop()
    {
      if (this->status_has_error())
      {
        this->disconnect();
        this->status_clear_error();
      }

      if (this->node_state != espbt::ClientState::ESTABLISHED)
        return;

      Command *cmd = this->commands_.pop();
      while (cmd != nullptr)
      {
        if (cmd->execute(this->parent()))
          this->request_counter_++;

        delete cmd;
        cmd = this->commands_.pop();
      }

      // once we are done with pending commands - check to see if there are any pending requests
      // if there are no pending requests - we are done with the device for now and should disconnect
      if (this->request_counter_ == 0)
        this->disconnect();
    }

    void Device::update()
    {
      this->connect();

      this->commands_.push(new Command(CommandType::READ, this->p_battery));
      this->commands_.push(new Command(CommandType::READ, this->p_temperature));
      this->commands_.push(new Command(CommandType::READ, this->p_settings));
      this->commands_.push(new Command(CommandType::READ, this->p_errors));
    }

    void Device::control(const ClimateCall &call)
    {
      if (call.get_target_temperature().has_value())
      {
        TemperatureData &t_data = (TemperatureData &)(*this->p_temperature->data);
        t_data.target_temperature = *call.get_target_temperature();

        this->commands_.push(new Command(CommandType::WRITE, this->p_temperature));
        // initiate connection to the device
        this->connect();
      }

      if (call.get_mode().has_value())
      {
        SettingsData &s_data = (SettingsData &)(*this->p_settings->data);
        s_data.device_mode = *call.get_mode();

        // update state immediately to avoid delays in HA UI
        this->mode = s_data.device_mode;
        this->publish_state();

        this->commands_.push(new Command(CommandType::WRITE, this->p_settings));
        // initiate connection to the device
        this->connect();
      }
    }

    void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
    {
      switch (event)
      {
      case ESP_GATTC_CONNECT_EVT:
        if (memcmp(param->connect.remote_bda, this->parent()->remote_bda, 6) != 0)
          return; // event does not belong to this client, exit gattc_event_handler

        ESP_LOGI(TAG, "[%s] connect, conn_id=%d", this->parent()->address_str().c_str(), param->connect.conn_id);
        break;

      case ESP_GATTC_OPEN_EVT:
        if (param->open.status == ESP_GATT_OK)
          ESP_LOGD(TAG, "[%s] open, conn_id=%d", this->parent()->address_str().c_str(), param->open.conn_id);
        else
          ESP_LOGW(TAG, "[%s] failed to open, conn_id=%d, status=%#04x", this->parent()->address_str().c_str(), param->open.conn_id, param->open.status);
        break;

      case ESP_GATTC_CLOSE_EVT:
        if (param->close.status == ESP_GATT_OK)
          ESP_LOGD(TAG, "[%s] close, conn_id=%d, reason=%d", this->parent()->address_str().c_str(), param->close.conn_id, param->close.reason);
        else
          ESP_LOGW(TAG, "[%s] failed to close, conn_id=%d, status=%#04x", this->parent()->address_str().c_str(), param->close.conn_id, param->close.status);
        break;

      case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(TAG, "[%s] disconnect, conn_id=%d, reason=%#04x", this->parent()->address_str().c_str(), param->disconnect.conn_id, (int)param->disconnect.reason);
        break;

      case ESP_GATTC_SEARCH_CMPL_EVT:
        for (auto p : this->properties)
          p->init_handle(this->parent());

        write_pin();
        break;

      case ESP_GATTC_WRITE_CHAR_EVT:
        if (param->write.handle == this->p_pin->handle)
        {
          if (param->write.status != ESP_GATT_OK)
          {
            ESP_LOGE(TAG, "[%s] pin FAILED, status=%#04x", this->parent()->address_str().c_str(), param->write.status);
            this->disconnect();
            this->mark_failed();
          }
          else
          {
            ESP_LOGD(TAG, "[%s] pin OK", this->parent()->address_str().c_str());
            this->node_state = espbt::ClientState::ESTABLISHED;
          }
          break;
        }

        this->request_counter_--;
        if (param->write.status != ESP_GATT_OK)
          ESP_LOGW(TAG, "[%s] failed to write characteristic: handle=%#04x, status=%#04x", this->parent()->address_str().c_str(), param->write.handle, param->write.status);
        else // write request ACK
          this->on_write(param->write);
        break;

      case ESP_GATTC_READ_CHAR_EVT:
        this->request_counter_--;
        if (param->read.status != ESP_GATT_OK)
          ESP_LOGW(TAG, "[%s] failed to read characteristic: handle=%#04x, status=%#04x", this->parent()->address_str().c_str(), param->read.handle, param->read.status);
        else
          this->on_read(param->read);
        break;

      default:
        ESP_LOGV(TAG, "[%s] unhandled event: event=%d, gattc_if=%d", this->parent()->address_str().c_str(), (int)event, gattc_if);
        break;
      }
    }

    void Device::write_pin()
    {
      ESP_LOGD(TAG, "[%s] writing pin", this->parent()->address_str().c_str());

      uint8_t pin_bytes[sizeof(uint32_t)];
      write_int(pin_bytes, 0, this->pin_code_);

      if (!this->p_pin->write_request(this->parent(), pin_bytes, sizeof(pin_bytes)))
        this->status_set_error();
    }

    void Device::on_read(esp_ble_gattc_cb_param_t::gattc_read_char_evt_param param)
    {
      auto device_property = std::find_if(properties.begin(), properties.end(),
                                          [&param](std::shared_ptr<DeviceProperty> p)
                                          { return p->handle == param.handle; });

      if (device_property != properties.end())
        (*device_property)->update_state(this, param.value, param.value_len);
      else
        ESP_LOGW(TAG, "[%s] unknown property with handle=%#04x", this->parent()->address_str().c_str(), param.handle);
    }

    void Device::on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param param)
    {
      if (param.handle != this->p_pin->handle)
        update();
    }

    void Device::connect()
    {
      if (this->node_state == espbt::ClientState::ESTABLISHED)
      {
        return;
      }

      if (!parent()->enabled)
      {
        ESP_LOGD(TAG, "[%s] re-enabling ble_client", this->parent()->address_str().c_str());
        parent()->set_enabled(true);
      }
      // gap scanning interferes with connection attempts, which results in esp_gatt_status_t::ESP_GATT_ERROR (0x85)
      esp_ble_gap_stop_scanning();
      this->parent()->set_state(espbt::ClientState::DISCOVERED); // this will cause ble_client to attempt connect() from its loop()
    }

    void Device::disconnect()
    {
      this->parent()->set_enabled(false);
      this->node_state = espbt::ClientState::IDLE;
    }

    void Device::set_pin_code(const std::string &str)
    {
      if (str.length() > 0)
        this->pin_code_ = atoi((const char *)str.c_str());
      ESP_LOGD(TAG, "[%s] PIN: %04d", this->parent()->address_str().c_str(), this->pin_code_);
    }

    void Device::set_secret_key(const char *str)
    {
      this->secret_ = parse_hex_str(str, 32);

      std::string hex_str = hexencode(this->secret_, 16);
      ESP_LOGD(TAG, "[%s] secret bytes: %s", this->parent()->address_str().c_str(), hex_str.c_str());
    }

  } // namespace danfoss_eco
} // namespace esphome

#endif
