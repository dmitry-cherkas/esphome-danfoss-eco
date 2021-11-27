#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#include "my_component.h"

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
            DeviceProperty(espbt::ESPBTUUID s_uuid, espbt::ESPBTUUID c_uuid)
            {
                this->service_uuid = s_uuid;
                this->characteristic_uuid = c_uuid;
            }

            virtual void read(MyComponent *component, uint8_t *value, uint16_t value_len){};

            bool init_handle(esphome::ble_client::BLEClient *);
            uint16_t handle;

        private:
            espbt::ESPBTUUID service_uuid;
            espbt::ESPBTUUID characteristic_uuid;
        };

        class NameProperty : public DeviceProperty
        {
        public:
            NameProperty() : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_NAME) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        class BatteryProperty : public DeviceProperty
        {
        public:
            BatteryProperty() : DeviceProperty(SERVICE_BATTERY, CHARACTERISTIC_BATTERY) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        struct DeviceData
        {
            uint16_t length;
            virtual uint8_t *pack() = 0;

            DeviceData(uint16_t l) : length(l) {}
            virtual ~DeviceData() {}
        };

        struct TemperatureData : public DeviceData
        {
            float target_temperature;
            float room_temperature;

            TemperatureData(float target_temperature) : DeviceData(8)
            {
                this->target_temperature = target_temperature;
            }

            TemperatureData(uint8_t *raw_data, uint16_t value_len) : DeviceData(8)
            {
                uint8_t *temperatures = decrypt(raw_data, value_len);
                this->target_temperature = temperatures[0] / 2.0f;
                this->room_temperature = temperatures[1] / 2.0f;
            }

            uint8_t *pack()
            {
                uint8_t buff[length] = {(uint8_t) (target_temperature * 2), 0};
                return encrypt(buff, sizeof(buff));
            }
        };

        class TemperatureProperty : public DeviceProperty
        {
        public:
            TemperatureProperty() : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_TEMPERATURE) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        class SettingsProperty : public DeviceProperty
        {
        public:
            enum DeviceMode
            {
                MANUAL = 0,
                SCHEDULED = 1,
                VACATION = 3,
                HOLD = 5 // TODO: what is the meaning of this mode?
            };

            SettingsProperty() : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_SETTINGS) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
            climate::ClimateMode to_climate_mode(DeviceMode);
        };

        class CurrentTimeProperty : public DeviceProperty
        {
        public:
            CurrentTimeProperty() : DeviceProperty(SERVICE_SETTINGS, CHARACTERISTIC_CURRENT_TIME) {}
            void read(MyComponent *component, uint8_t *value, uint16_t value_len) override;
        };

        enum class CommandType
        {
            READ,
            WRITE
        };

        class Command // read/write, target property, value to write
        {
        public:
            Command(CommandType t, DeviceProperty *p) : type(t), property(p) {}
            Command(CommandType t, DeviceProperty *p, DeviceData *d) : type(t), property(p), data(d) {}

            CommandType type; // 0 - read, 1 - write
            DeviceProperty *property;
            std::unique_ptr<DeviceData> data{nullptr};
        };

    } // namespace danfoss_eco
} // namespace esphome