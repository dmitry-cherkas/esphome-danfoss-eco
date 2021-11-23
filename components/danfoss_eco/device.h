#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/climate/climate.h"

#include "device_state.h"
#include "helpers.h"

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
      void write_pin();
      uint16_t read_characteristic(espbt::ESPBTUUID service_uuid, espbt::ESPBTUUID characteristic_uuid);
      uint8_t *decrypt(uint8_t *value, uint16_t value_len);

      std::unique_ptr<DeviceState> state_;

      uint8_t *secret_;
      uint8_t *pin_code_;

      sensor::Sensor *battery_level_{nullptr};
      sensor::Sensor *temperature_{nullptr};

      uint16_t pin_chr_handle_;
      uint16_t name_chr_handle_;
      uint16_t battery_chr_handle_;
      uint16_t temperature_chr_handle_;
      uint16_t current_time_chr_handle_;
      uint16_t settings_chr_handle_;

      uint8_t request_counter_ = 0;
    };

  } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32
