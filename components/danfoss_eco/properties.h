#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#include "my_component.h"
#include "device_data.h"

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

        class DeviceProperty
        {
        public:
            std::unique_ptr<DeviceData> data{nullptr};

            DeviceProperty(espbt::ESPBTUUID s_uuid, espbt::ESPBTUUID c_uuid, std::shared_ptr<Xxtea> &xxtea) : service_uuid(s_uuid), characteristic_uuid(c_uuid), xxtea_(xxtea) {}

            virtual void read(MyComponent *component, uint8_t *value, uint16_t value_len){};

            bool init_handle(esphome::ble_client::BLEClient *);
            bool read_request(esphome::ble_client::BLEClient *client);
            bool write_request(esphome::ble_client::BLEClient *client);
            bool write_request(esphome::ble_client::BLEClient *client, uint8_t *data, uint16_t data_len);

            uint16_t handle;

        private:
            espbt::ESPBTUUID service_uuid;
            espbt::ESPBTUUID characteristic_uuid;

        protected:
            std::shared_ptr<Xxtea> xxtea_{nullptr};
        };

        class NameProperty : public DeviceProperty
        {
        public:
            NameProperty(std::shared_ptr<Xxtea> &xxtea) : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_NAME, xxtea) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        class BatteryProperty : public DeviceProperty
        {
        public:
            BatteryProperty(std::shared_ptr<Xxtea> &xxtea) : DeviceProperty(SERVICE_BATTERY, CHARACTERISTIC_BATTERY, xxtea) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        class TemperatureProperty : public DeviceProperty
        {
        public:
            TemperatureProperty(std::shared_ptr<Xxtea> &xxtea) : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_TEMPERATURE, xxtea) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        class SettingsProperty : public DeviceProperty
        {
        public:
            SettingsProperty(std::shared_ptr<Xxtea> &xxtea) : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_SETTINGS, xxtea) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        class CurrentTimeProperty : public DeviceProperty
        {
        public:
            CurrentTimeProperty(std::shared_ptr<Xxtea> &xxtea) : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_CURRENT_TIME, xxtea) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };
    } // namespace danfoss_eco
} // namespace esphome