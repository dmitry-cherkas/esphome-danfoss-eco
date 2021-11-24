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
        if (cmd->type == CommandType::READ)
          read_request(cmd->property);
        else
          write_request(cmd->property, cmd->value);

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
        // this is new temprature we want to set
        float new_target_temperature = *call.get_target_temperature();

        // TODO make sure we are connected to eTRV
        // TODO prepare write command to set the target temperature
        /*auto pkt = this->codec_->get_set_target_temp_request(*call.get_target_temperature());
        auto status = esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->char_handle_,
                                               pkt->length, pkt->data, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
        if (status)
          ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(), status);*/
      }

      // TODO add mode switch (AUTO -> HEAT) - toggle between schedule and manual modes
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
        this->write_request(this->p_pin, this->pin_code_);
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
      if (param.handle == this->p_name->handle)
      {
        uint8_t *name = decrypt(param.value, param.value_len);
        ESP_LOGD(TAG, "[%s] device name: %s", this->parent()->address_str().c_str(), name);
        std::string name_str((char *)name);
        // this->set_name(name_str); TODO - this is too late
      }
      else if (param.handle == this->p_battery->handle)
      {
        uint8_t battery_level = param.value[0];
        ESP_LOGD(TAG, "[%s] battery level: %d %%", this->parent()->address_str().c_str(), battery_level);
        this->battery_level_->publish_state(battery_level);
      }
      else if (param.handle == this->p_temperature->handle)
      {
        uint8_t *temperatures = decrypt(param.value, param.value_len);
        float set_point_temperature = temperatures[0] / 2.0f;
        float room_temperature = temperatures[1] / 2.0f;
        ESP_LOGD(TAG, "[%s] Current room temperature: %2.1f°C, Set point temperature: %2.1f°C", this->parent()->address_str().c_str(), room_temperature, set_point_temperature);
        temperature_->publish_state(room_temperature);

        // apply read configuration to the component
        this->action = (room_temperature > set_point_temperature) ? ClimateAction::CLIMATE_ACTION_IDLE : ClimateAction::CLIMATE_ACTION_HEATING;
        this->target_temperature = set_point_temperature;
        this->current_temperature = room_temperature;
        this->publish_state();
      }
      /*else if (param.handle == this->current_time_chr_handle_)
      {
        uint8_t *current_time = decrypt(param.value, param.value_len);
        int local_time = parse_int(current_time, 0);
        int time_offset = parse_int(current_time, 4);
        ESP_LOGD(TAG, "[%s] local_time: %d, time_offset: %d", this->parent()->address_str().c_str(), local_time, time_offset);
      } */
      else if (param.handle == this->p_settings->handle)
      {
        uint8_t *settings = decrypt(param.value, param.value_len);
        uint8_t config_bits = settings[0];

        bool adaptable_regulation = parse_bit(config_bits, 0);
        ESP_LOGD(TAG, "[%s] adaptable_regulation: %d", this->parent()->address_str().c_str(), adaptable_regulation);

        bool vertical_intallation = parse_bit(config_bits, 2);
        ESP_LOGD(TAG, "[%s] vertical_intallation: %d", this->parent()->address_str().c_str(), vertical_intallation);

        bool display_flip = parse_bit(config_bits, 3);
        ESP_LOGD(TAG, "[%s] display_flip: %d", this->parent()->address_str().c_str(), display_flip);

        bool slow_regulation = parse_bit(config_bits, 4);
        ESP_LOGD(TAG, "[%s] slow_regulation: %d", this->parent()->address_str().c_str(), slow_regulation);

        bool valve_installed = parse_bit(config_bits, 6);
        ESP_LOGD(TAG, "[%s] valve_installed: %d", this->parent()->address_str().c_str(), valve_installed);

        bool lock_control = parse_bit(config_bits, 7);
        ESP_LOGD(TAG, "[%s] lock_control: %d", this->parent()->address_str().c_str(), lock_control);

        float temperature_min = settings[1] / 2.0f;
        ESP_LOGD(TAG, "[%s] temperature_min: %2.1f°C", this->parent()->address_str().c_str(), temperature_min);

        float temperature_max = settings[2] / 2.0f;
        ESP_LOGD(TAG, "[%s] temperature_max: %2.1f°C", this->parent()->address_str().c_str(), temperature_max);

        float frost_protection_temperature = settings[3] / 2.0f;
        ESP_LOGD(TAG, "[%s] frost_protection_temperature: %2.1f°C", this->parent()->address_str().c_str(), frost_protection_temperature);
        // TODO add frost_protection_temperature sensor (OR CONTROL?)

        DeviceMode schedule_mode = (DeviceMode)settings[4];
        ESP_LOGD(TAG, "[%s] schedule_mode: %d", this->parent()->address_str().c_str(), schedule_mode);
        this->mode = from_device_mode(schedule_mode);

        float vacation_temperature = settings[5] / 2.0f;
        ESP_LOGD(TAG, "[%s] vacation_temperature: %2.1f°C", this->parent()->address_str().c_str(), vacation_temperature);
        // TODO add vacation_temperature sensor (OR CONTROL?)

        // vacation mode can be enabled directly with schedule_mode, or planned with below dates
        int vacation_from = parse_int(settings, 6);
        ESP_LOGD(TAG, "[%s] vacation_from: %d", this->parent()->address_str().c_str(), vacation_from);

        int vacation_to = parse_int(settings, 10);
        ESP_LOGD(TAG, "[%s] vacation_to: %d", this->parent()->address_str().c_str(), vacation_to);

        // apply read configuration to the component
        this->set_visual_min_temperature_override(temperature_min);
        this->set_visual_max_temperature_override(temperature_max);
        this->publish_state();
      }
    }

    void Device::on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param param)
    {
    }

    // Update is triggered by on defined polling interval (see PollingComponent) to trigger state update report
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
        ESP_LOGD(TAG, "[%s] received update request, re-enabling ble_client", this->parent()->address_str().c_str());
        parent()->set_enabled(true);
      }
      // gap scanning interferes with connection attempts, which results in esp_gatt_status_t::ESP_GATT_ERROR (0x85)
      esp_ble_gap_stop_scanning();
      this->parent()->set_state(espbt::ClientState::DISCOVERED); // this will cause ble_client to attempt connect() from its loop()
    }

    void Device::read_request(DeviceProperty *property)
    {
      auto status = esp_ble_gattc_read_char(this->parent()->gattc_if,
                                            this->parent()->conn_id,
                                            property->handle,
                                            ESP_GATT_AUTH_REQ_NONE);
      if (status != ESP_OK)
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, status=%01x", this->parent()->address_str().c_str(), status);
      else
        this->request_counter_++;
    }

    void Device::write_request(DeviceProperty *property, uint8_t *value)
    {
      auto status = esp_ble_gattc_write_char(this->parent()->gattc_if,
                                             this->parent()->conn_id,
                                             property->handle,
                                             sizeof(value),
                                             value,
                                             ESP_GATT_WRITE_TYPE_RSP,
                                             ESP_GATT_AUTH_REQ_NONE);
      if (status != ESP_OK)
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%01x", this->parent()->address_str().c_str(), status);
      else
        this->request_counter_++;
    }

    void Device::set_pin_code(const std::string &str)
    {
      if (str.length() > 0)
      {
        this->pin_code_ = (uint8_t *)str.c_str();
      }
      else
      {
        this->pin_code_ = default_pin_code;
      }
      std::string hex_str = hexencode(this->pin_code_, sizeof(this->pin_code_));
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
