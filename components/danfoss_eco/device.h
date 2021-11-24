#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/climate/climate.h"

#include "helpers.h"
#include "properties.h"

#ifdef USE_ESP32

#include <esp_gattc_api.h>

namespace esphome
{
  namespace danfoss_eco
  {

    namespace espbt = esphome::esp32_ble_tracker;

    static auto SERVICE_SETTINGS = espbt::ESPBTUUID::from_raw("10020000-2749-0001-0000-00805f9b042f");
    static auto CHARACTERISTIC_PIN = espbt::ESPBTUUID::from_raw("10020001-2749-0001-0000-00805f9b042f");          // 0x24
    static auto CHARACTERISTIC_SETTINGS = espbt::ESPBTUUID::from_raw("10020003-2749-0001-0000-00805f9b042f");     // 0x2a
    static auto CHARACTERISTIC_TEMPERATURE = espbt::ESPBTUUID::from_raw("10020005-2749-0001-0000-00805f9b042f");  // 0x2d
    static auto CHARACTERISTIC_NAME = espbt::ESPBTUUID::from_raw("10020006-2749-0001-0000-00805f9b042f");         // 0x30
    static auto CHARACTERISTIC_CURRENT_TIME = espbt::ESPBTUUID::from_raw("10020008-2749-0001-0000-00805f9b042f"); // 0x36

    static auto SERVICE_BATTERY = espbt::ESPBTUUID::from_uint32(0x180F);
    static auto CHARACTERISTIC_BATTERY = espbt::ESPBTUUID::from_uint32(0x2A19); // 0x10

    static uint8_t default_pin_code[] = {0x30, 0x30, 0x30, 0x30};

    class Device : public climate::Climate, public esphome::ble_client::BLEClientNode, public PollingComponent
    {
    public:
      Device()
      {
        this->p_pin = new DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_PIN);
        this->p_name = new DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_NAME);
        this->p_battery = new DeviceProperty(SERVICE_BATTERY, CHARACTERISTIC_BATTERY);
        this->p_temperature = new DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_TEMPERATURE);
        this->p_settings = new DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_SETTINGS);

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
      void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }
      void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }

    protected:
      void control(const climate::ClimateCall &call) override;

      void connect();
      void read_request(DeviceProperty *);
      void write_request(DeviceProperty *, uint8_t *value);

      void on_read(esp_ble_gattc_cb_param_t::gattc_read_char_evt_param);
      void on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param);

      DeviceProperty *p_pin;
      DeviceProperty *p_name;
      DeviceProperty *p_battery;
      DeviceProperty *p_temperature;
      DeviceProperty *p_settings;
      std::set<DeviceProperty *> properties;

      uint8_t *secret_;
      uint8_t *pin_code_;

      sensor::Sensor *battery_level_{nullptr};
      sensor::Sensor *temperature_{nullptr};

      uint8_t request_counter_ = 0;
      espbt::Queue<Command> commands_;
    };

  } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32
