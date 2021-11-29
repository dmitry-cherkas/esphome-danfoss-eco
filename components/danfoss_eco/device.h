#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/climate/climate.h"

#include "helpers.h"
#include "command.h"
#include "properties.h"
#include "my_component.h"
#include "xxtea.h"

#ifdef USE_ESP32

#include <esp_gattc_api.h>

namespace esphome
{
  namespace danfoss_eco
  {
    static uint8_t SECRET_KEY_LENGTH = 16;

    class Device : public MyComponent, public esphome::ble_client::BLEClientNode
    {
    public:
      void setup() override;
      void loop() override;
      void update() override;
      void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;

      void set_secret_key(const char *);
      void set_pin_code(const std::string &);

    protected:
      void control(const climate::ClimateCall &call) override;

      void connect();
      void disconnect();

      void write_pin();

      void on_read(esp_ble_gattc_cb_param_t::gattc_read_char_evt_param);
      void on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param);

      std::shared_ptr<Xxtea> xxtea{nullptr};

      std::shared_ptr<DeviceProperty> p_pin{nullptr};
      std::shared_ptr<DeviceProperty> p_name{nullptr};
      std::shared_ptr<DeviceProperty> p_battery{nullptr};
      std::shared_ptr<TemperatureProperty> p_temperature{nullptr};
      std::shared_ptr<SettingsProperty> p_settings{nullptr};
      std::shared_ptr<DeviceProperty> p_current_time{nullptr};
      std::set<std::shared_ptr<DeviceProperty>> properties{nullptr};

    private:
      uint8_t *secret_;
      uint32_t pin_code_ = 0;

      uint8_t request_counter_ = 0;
      CommandQueue commands_;
    };

  } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32
