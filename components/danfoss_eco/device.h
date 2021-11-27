#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/climate/climate.h"

#include "helpers.h"
#include "properties.h"
#include "my_component.h"

#ifdef USE_ESP32

#include <esp_gattc_api.h>

namespace esphome
{
  namespace danfoss_eco
  {
    static uint8_t default_pin_code[] = {0x30, 0x30, 0x30, 0x30};

    class Device : public MyComponent
    {
    public:
      Device()
      {
        this->p_pin = new DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_PIN);
        this->p_name = new NameProperty();
        this->p_battery = new BatteryProperty();
        this->p_temperature = new TemperatureProperty();
        this->p_settings = new SettingsProperty();
        this->p_current_time = new CurrentTimeProperty();

        this->properties = {this->p_pin, this->p_name, this->p_battery, this->p_temperature, this->p_settings};
      }

      void setup() override;
      void loop() override;
      void update() override;
      void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;
      void dump_config() override;
      float get_setup_priority() const override { return setup_priority::DATA; }

      climate::ClimateTraits traits() override;

      void set_secret_key(const char *);
      void set_pin_code(const std::string &);

    protected:
      void control(const climate::ClimateCall &call) override;

      void connect();

      void on_read(esp_ble_gattc_cb_param_t::gattc_read_char_evt_param);
      void on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param);

      DeviceProperty *p_pin{nullptr};
      DeviceProperty *p_name{nullptr};
      DeviceProperty *p_battery{nullptr};
      TemperatureProperty *p_temperature{nullptr};
      SettingsProperty *p_settings{nullptr};
      DeviceProperty *p_current_time{nullptr};
      std::set<DeviceProperty *> properties{nullptr};

      uint8_t *secret_;
      uint8_t *pin_code_;

      uint8_t request_counter_ = 0;
      esphome::esp32_ble_tracker::Queue<Command> commands_;
    };

  } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32
