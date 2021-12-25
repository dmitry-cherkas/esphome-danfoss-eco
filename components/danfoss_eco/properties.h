#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#include "my_component.h"
#include "device_data.h"

namespace esphome
{
    namespace danfoss_eco
    {
        using namespace std;
        using namespace esphome::esp32_ble_tracker;
        using namespace esphome::ble_client;

        static auto SERVICE_SETTINGS = ESPBTUUID::from_raw("10020000-2749-0001-0000-00805f9b042f");
        static auto CHARACTERISTIC_PIN = ESPBTUUID::from_raw("10020001-2749-0001-0000-00805f9b042f");         // 0x24
        static auto CHARACTERISTIC_SETTINGS = ESPBTUUID::from_raw("10020003-2749-0001-0000-00805f9b042f");    // 0x2a
        static auto CHARACTERISTIC_TEMPERATURE = ESPBTUUID::from_raw("10020005-2749-0001-0000-00805f9b042f"); // 0x2d
        static auto CHARACTERISTIC_ERRORS = ESPBTUUID::from_raw("10020009-2749-0001-0000-00805f9b042f");      // 0x39
        static auto CHARACTERISTIC_SECRET_KEY = ESPBTUUID::from_raw("1002000b-2749-0001-0000-00805f9b042f");  // 0x3f

        static auto SERVICE_BATTERY = ESPBTUUID::from_uint32(0x180F);
        static auto CHARACTERISTIC_BATTERY = ESPBTUUID::from_uint32(0x2A19); // 0x10

        const uint16_t INVALID_HANDLE = -1;
        
        const uint8_t SECRET_KEY_LENGTH = 16;
        struct SecretKeyValue
        {
            SecretKeyValue() {}
            SecretKeyValue(uint8_t *val)
            {
                memcpy(this->value, (const char *)val, SECRET_KEY_LENGTH);
            }
            uint8_t value[SECRET_KEY_LENGTH];
        };

        class DeviceProperty
        {
        public:
            unique_ptr<DeviceData> data{nullptr};

            DeviceProperty(shared_ptr<MyComponent> &component, shared_ptr<Xxtea> &xxtea, ESPBTUUID s_uuid, ESPBTUUID c_uuid) : component_(component), xxtea_(xxtea), service_uuid(s_uuid), characteristic_uuid(c_uuid) {}

            virtual void update_state(uint8_t *value, uint16_t value_len){};

            virtual bool init_handle(BLEClient *);
            bool read_request(BLEClient *client);

            uint16_t handle;

        protected:
            shared_ptr<MyComponent> component_{nullptr};
            shared_ptr<Xxtea> xxtea_{nullptr};

            ESPBTUUID service_uuid;
            ESPBTUUID characteristic_uuid;
        };

        class WritableProperty : public DeviceProperty
        {
        public:
            WritableProperty(shared_ptr<MyComponent> &component, shared_ptr<Xxtea> &xxtea, ESPBTUUID s_uuid, ESPBTUUID c_uuid) : DeviceProperty(component, xxtea, s_uuid, c_uuid) {}

            bool write_request(BLEClient *client);
            bool write_request(BLEClient *client, uint8_t *data, uint16_t data_len);
        };

        class BatteryProperty : public DeviceProperty
        {
        public:
            BatteryProperty(shared_ptr<MyComponent> &component, shared_ptr<Xxtea> &xxtea) : DeviceProperty(component, xxtea, SERVICE_BATTERY, CHARACTERISTIC_BATTERY) {}
            void update_state(uint8_t *value, uint16_t value_len) override;
        };

        class TemperatureProperty : public WritableProperty
        {
        public:
            TemperatureProperty(shared_ptr<MyComponent> &component, shared_ptr<Xxtea> &xxtea) : WritableProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_TEMPERATURE) {}
            void update_state(uint8_t *value, uint16_t value_len) override;
        };

        class SettingsProperty : public WritableProperty
        {
        public:
            SettingsProperty(shared_ptr<MyComponent> &component, shared_ptr<Xxtea> &xxtea) : WritableProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_SETTINGS) {}
            void update_state(uint8_t *value, uint16_t value_len) override;
        };

        class ErrorsProperty : public DeviceProperty
        {
        public:
            ErrorsProperty(shared_ptr<MyComponent> &component, shared_ptr<Xxtea> &xxtea) : DeviceProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_ERRORS) {}
            void update_state(uint8_t *value, uint16_t value_len) override;
        };

        class SecretKeyProperty : public DeviceProperty
        {
        public:
            SecretKeyProperty(shared_ptr<MyComponent> &component, shared_ptr<Xxtea> &xxtea) : DeviceProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_SECRET_KEY) {}
            void update_state(uint8_t *value, uint16_t value_len) override;

            bool init_handle(BLEClient *) override;
        };

    } // namespace danfoss_eco
} // namespace esphome