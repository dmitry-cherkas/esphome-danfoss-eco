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
      uint64_t addr = this->parent()->address;
      this->parent()->remote_bda[0] = (addr >> 40) & 0xFF;
      this->parent()->remote_bda[1] = (addr >> 32) & 0xFF;
      this->parent()->remote_bda[2] = (addr >> 24) & 0xFF;
      this->parent()->remote_bda[3] = (addr >> 16) & 0xFF;
      this->parent()->remote_bda[4] = (addr >> 8) & 0xFF;
      this->parent()->remote_bda[5] = (addr >> 0) & 0xFF;
    }

    void Device::loop() {}

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
        ESP_LOGI(TAG, "[%s] writing pin", this->parent()->address_str().c_str());
        this->write_pin();
        break;

      case ESP_GATTC_WRITE_CHAR_EVT:
        if (param->write.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "[%s] failed to write characteristic: handle=%#04x, status=%#04x", this->parent()->address_str().c_str(), param->write.handle, param->write.status);
        }
        else if (param->write.handle == this->pin_chr_handle_)
        { // PIN OK
          ESP_LOGI(TAG, "[%s] pin OK", this->parent()->address_str().c_str());
          this->node_state = espbt::ClientState::ESTABLISHED;

          this->name_chr_handle_ = this->read_characteristic(SERVICE_SETTINGS, CHARACTERISTIC_NAME);
          this->battery_chr_handle_ = this->read_characteristic(SERVICE_BATTERY, CHARACTERISTIC_BATTERY);
          this->temperature_chr_handle_ = this->read_characteristic(SERVICE_SETTINGS, CHARACTERISTIC_TEMPERATURE);
          this->current_time_chr_handle_ = this->read_characteristic(SERVICE_SETTINGS, CHARACTERISTIC_CURRENT_TIME);
          this->settings_chr_handle_ = this->read_characteristic(SERVICE_SETTINGS, CHARACTERISTIC_SETTINGS);
        }
        break;

      case ESP_GATTC_READ_CHAR_EVT:
        if (param->read.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "[%s] failed to read characteristic: handle=%#04x, status=%#04x", this->parent()->address_str().c_str(), param->read.handle, param->read.status);
          break;
        }
        this->status_clear_warning();

        if (param->read.handle == this->name_chr_handle_)
        {
          uint8_t *name = this->decrypt(param->read.value, param->read.value_len);
          ESP_LOGD(TAG, "[%s] device name: %s", this->parent()->address_str().c_str(), name);
          std::string name_str((char *)name);
          // this->set_name(name_str); TODO - this is too late
        }
        else if (param->read.handle == this->battery_chr_handle_)
        {
          uint8_t battery_level = param->read.value[0];
          ESP_LOGD(TAG, "[%s] battery level: %d %%", this->parent()->address_str().c_str(), battery_level);
          this->battery_level_->publish_state(battery_level);
        }
        else if (param->read.handle == this->temperature_chr_handle_)
        {
          uint8_t *temperatures = this->decrypt(param->read.value, param->read.value_len);
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
        else if (param->read.handle == this->current_time_chr_handle_)
        {
          uint8_t *current_time = this->decrypt(param->read.value, param->read.value_len);
          int local_time = parse_int(current_time, 0);
          int time_offset = parse_int(current_time, 4);
          ESP_LOGD(TAG, "[%s] local_time: %d, time_offset: %d", this->parent()->address_str().c_str(), local_time, time_offset);
        }
        else if (param->read.handle == this->settings_chr_handle_)
        {
          uint8_t *settings = this->decrypt(param->read.value, param->read.value_len);
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
          this->mode = this->from_device_mode(schedule_mode);

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

        if (--this->request_counter_ == 0)
        {
          this->parent()->set_enabled(false);
          this->node_state = espbt::ClientState::IDLE;
        }
        break;

      default:
        ESP_LOGD(TAG, "[%s] unhandled event: event=%d, gattc_if=%d", this->parent()->address_str().c_str(), (int)event, gattc_if);
        break;
      }
    }

    climate::ClimateMode Device::from_device_mode(DeviceMode schedule_mode)
    {
      switch (schedule_mode)
      {
      case DeviceMode::MANUAL:
        return ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

      case DeviceMode::VACATION:
      case DeviceMode::SCHEDULED:
        return ClimateMode::CLIMATE_MODE_AUTO;

      case DeviceMode::HOLD:
        return ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

      default:
        ESP_LOGW(TAG, "[%s] unexpected schedule_mode: %d", this->parent()->address_str().c_str(), schedule_mode);
        return ClimateMode::CLIMATE_MODE_HEAT; // unknown
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
                                             sizeof(this->pin_code_),
                                             this->pin_code_,
                                             ESP_GATT_WRITE_TYPE_RSP,
                                             ESP_GATT_AUTH_REQ_NONE);
      if (status != ESP_OK)
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%01x", this->parent()->address_str().c_str(), status);
    }

    uint16_t Device::read_characteristic(espbt::ESPBTUUID service_uuid, espbt::ESPBTUUID characteristic_uuid)
    {
      ESP_LOGI(TAG, "[%s] reading characteristic uuid=%s", this->parent()->address_str().c_str(), characteristic_uuid.to_string().c_str());
      auto chr = this->parent()->get_characteristic(service_uuid, characteristic_uuid);
      if (chr == nullptr)
      {
        ESP_LOGW(TAG, "[%s] characteristic uuid=%s not found", this->get_name().c_str(), characteristic_uuid.to_string().c_str());
        this->status_set_warning();
        return NAN_HANDLE;
      }

      auto status = esp_ble_gattc_read_char(this->parent()->gattc_if,
                                            this->parent()->conn_id,
                                            chr->handle,
                                            ESP_GATT_AUTH_REQ_NONE);
      if (status != ESP_OK)
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, status=%01x", this->parent()->address_str().c_str(), status);

      // increment request counter to keep track of pending requests
      this->request_counter_++;

      return chr->handle;
    }

    uint8_t *Device::decrypt(uint8_t *value, uint16_t value_len)
    {
      std::string s = hexencode(value, value_len);
      ESP_LOGD(TAG, "[%s] raw value: %s", this->parent()->address_str().c_str(), s.c_str());

      uint8_t buffer[value_len];
      reverse_chunks(value, value_len, buffer);
      std::string s1 = hexencode(buffer, value_len);
      ESP_LOGV(TAG, "[%s] reversed bytes: %s", this->parent()->address_str().c_str(), s1.c_str());

      // Perform Decryption
      auto xxtea_status = xxtea_decrypt(buffer, value_len);
      if (xxtea_status != XXTEA_STATUS_SUCCESS)
      {
        ESP_LOGW(TAG, "[%s] xxtea_decrypt failed, status=%d", this->parent()->address_str().c_str(), xxtea_status);
      }
      else
      {
        std::string s2 = hexencode(buffer, value_len);
        ESP_LOGV(TAG, "[%s] decrypted reversed bytes: %s", this->parent()->address_str().c_str(), s2.c_str());

        reverse_chunks(buffer, value_len, value);
        ESP_LOGD(TAG, "[%s] decrypted value: %s", this->parent()->address_str().c_str(), value);
      }
      return value;
    }

    // Update is triggered by on defined polling interval (see PollingComponent) to trigger state update report
    void Device::update()
    {
      if (this->node_state != espbt::ClientState::ESTABLISHED)
      {
        if (!parent()->enabled)
        {
          ESP_LOGD(TAG, "[%s] received update request, re-enabling ble_client", this->parent()->address_str().c_str());
          parent()->set_enabled(true);
        }
        // gap scanning interferes with connection attempts, which results in esp_gatt_status_t::ESP_GATT_ERROR (0x85)
        esp_ble_gap_stop_scanning();
        this->parent()->set_state(espbt::ClientState::DISCOVERED); // this will cause client to attempt connect() from its loop()
      }
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
