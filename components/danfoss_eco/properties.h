#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco
    {
        namespace espbt = esphome::esp32_ble_tracker;

        class DeviceProperty
        {
        public:
            DeviceProperty(espbt::ESPBTUUID s_uuid, espbt::ESPBTUUID c_uuid)
            {
                this->service_uuid = s_uuid;
                this->characteristic_uuid = c_uuid;
            }

            espbt::ESPBTUUID service_uuid;
            espbt::ESPBTUUID characteristic_uuid;
            uint16_t handle;
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

#endif // USE_ESP32
