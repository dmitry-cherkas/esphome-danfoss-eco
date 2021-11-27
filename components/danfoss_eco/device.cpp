#include "device.h"
#include <xxtea-lib.h>

#ifdef USE_ESP32

namespace esphome
{
  namespace danfoss_eco
  {

    using namespace esphome::climate;

    void Device::dump_config()
    {
      LOG_CLIMATE(TAG, "Danfoss Eco eTRV", this);
      LOG_SENSOR(TAG, "Battery Level", this->battery_level_);
      LOG_SENSOR(TAG, "Room Temperature", this->temperature_);
    }

    void Device::setup()
    {
      // Setup encryption key
      auto status = xxtea_setup_key(this->secret_, 16);
      if (status != XXTEA_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "xxtea_setup_key failed, status: %d", status);
        this->mark_failed();
        return;
      }

      // pretend, we have already discovered the device
      copy_address(this->parent()->address, this->parent()->remote_bda);
    }

    void Device::loop()
    {
      if (this->node_state != espbt::ClientState::ESTABLISHED)
      {
        return;
      }

      Command *cmd = this->commands_.pop();
      while (cmd != nullptr)
      {
        bool success;
        if (cmd->type == CommandType::WRITE)
          success = cmd->property->write_request(this->parent());
        else
          success = cmd->property->read_request(this->parent());

        if (success)
          this->request_counter_++;

        delete cmd;
        cmd = this->commands_.pop();
      }

      // once we are done with pending commands - check to see if there are any pending requests
      if (this->request_counter_ == 0)
      {
        // if there are no pending requests - we are done with the device for now, disconnecting
        this->parent()->set_enabled(false);
        this->node_state = espbt::ClientState::IDLE;
      }
    }

    void Device::control(const ClimateCall &call)
    {
      if (call.get_target_temperature().has_value())
      {
        TemperatureData &t_data = (TemperatureData &)(*this->p_temperature->data);
        t_data.target_temperature = *call.get_target_temperature();

        this->commands_.push(new Command(CommandType::WRITE, this->p_temperature));
        // initiate connection to the device
        connect();
      }

      if (call.get_mode().has_value())
      {
        SettingsData &s_data = (SettingsData &)(*this->p_settings->data);
        s_data.device_mode = *call.get_mode();

        this->commands_.push(new Command(CommandType::WRITE, this->p_settings));
        // initiate connection to the device
        connect();
      }
    }

    ClimateTraits Device::traits()
    {
      auto traits = ClimateTraits();
      traits.set_supports_current_temperature(true);

      traits.set_supported_modes(std::set<ClimateMode>({ClimateMode::CLIMATE_MODE_HEAT, ClimateMode::CLIMATE_MODE_AUTO}));
      traits.set_visual_temperature_step(0.5);

      traits.set_supports_current_temperature(true); // supports reporting current temperature
      traits.set_supports_action(true);              // supports reporting current action
      return traits;
    }

    void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
    {
      switch (event)
      {
      case ESP_GATTC_CONNECT_EVT:
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

        ESP_LOGI(TAG, "[%s] writing pin", this->parent()->address_str().c_str());
        if (this->p_pin->write_request(this->parent(), this->pin_code_, PIN_CODE_LENGTH)) // FIXME: when PIN is enabled, this fails
          this->request_counter_++;
        break;

      case ESP_GATTC_WRITE_CHAR_EVT:
        this->request_counter_--;
        if (param->write.status != ESP_GATT_OK)
          ESP_LOGW(TAG, "[%s] failed to write characteristic: handle=%#04x, status=%#04x", this->parent()->address_str().c_str(), param->write.handle, param->write.status);
        else if (param->write.handle == this->p_pin->handle)
        {
          ESP_LOGI(TAG, "[%s] pin OK", this->parent()->address_str().c_str());
          this->node_state = espbt::ClientState::ESTABLISHED;
        }
        else
        { // write request ACK
          this->on_write(param->write);
        }
        break;

      case ESP_GATTC_READ_CHAR_EVT:
        this->request_counter_--;
        if (param->read.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "[%s] failed to read characteristic: handle=%#04x, status=%#04x", this->parent()->address_str().c_str(), param->read.handle, param->read.status);
          break;
        }
        else
        {
          this->status_clear_warning();
          this->on_read(param->read);
        }
        break;

      default:
        ESP_LOGD(TAG, "[%s] unhandled event: event=%d, gattc_if=%d", this->parent()->address_str().c_str(), (int)event, gattc_if);
        break;
      }
    }

    void Device::on_read(esp_ble_gattc_cb_param_t::gattc_read_char_evt_param param)
    {
      auto device_property = std::find_if(properties.begin(), properties.end(),
                                          [&param](DeviceProperty *p)
                                          { return p->handle == param.handle; });

      if (device_property != properties.end())
        (*device_property)->read(this, param.value, param.value_len);
      else
        ESP_LOGW(TAG, "[%s] unknown property with handle=%#04x", this->parent()->address_str().c_str(), param.handle);
    }

    void Device::on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param param)
    {
      if (param.handle != this->p_pin->handle)
        update();
    }

    void Device::update()
    {
      this->connect();

      this->commands_.push(new Command(CommandType::READ, this->p_name));
      this->commands_.push(new Command(CommandType::READ, this->p_battery));
      this->commands_.push(new Command(CommandType::READ, this->p_temperature));
      this->commands_.push(new Command(CommandType::READ, this->p_settings));
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

    void Device::set_pin_code(const std::string &str)
    {
      if (str.length() > 0)
      {
        this->pin_code_ = (uint8_t *)malloc(str.length());
        memcpy(this->pin_code_, (const char *)str.c_str(), str.length());
      }
      else
      {
        this->pin_code_ = default_pin_code;
      }
      std::string hex_str = hexencode(this->pin_code_, str.length());
      ESP_LOGD(TAG, "[%s] PIN bytes: %s", this->parent()->address_str().c_str(), hex_str.c_str());
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
