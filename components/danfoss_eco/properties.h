#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome
{
    namespace danfoss_eco
    {
        namespace espbt = esphome::esp32_ble_tracker;

        enum DeviceMode : uint8_t
        {
            MANUAL = 0,
            SCHEDULED = 1,
            VACATION = 3,
            HOLD = 5 // TODO: what is the meaning of this mode?
        };

        climate::ClimateMode from_device_mode(DeviceMode);

        class DeviceProperty
        {
        public:
            DeviceProperty(espbt::ESPBTUUID s_uuid, espbt::ESPBTUUID c_uuid)
            {
                this->service_uuid = s_uuid;
                this->characteristic_uuid = c_uuid;
            }

            bool init_handle(esphome::ble_client::BLEClient *);
            uint16_t handle;

        private:
            espbt::ESPBTUUID service_uuid;
            espbt::ESPBTUUID characteristic_uuid;
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
            Command(CommandType t, DeviceProperty *p, uint8_t *v) : type(t), property(p), value(v) {}

            CommandType type; // 0 - read, 1 - write
            DeviceProperty *property;
            uint8_t *value;
        };

    } // namespace danfoss_eco
} // namespace esphome