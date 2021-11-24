#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/climate/climate.h"

#include "helpers.h"
#include "properties.h"

namespace esphome
{
    namespace danfoss_eco
    {
        class MyComponent : public climate::Climate, public esphome::ble_client::BLEClientNode, public PollingComponent
        {
        public:
            void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }
            void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }

            sensor::Sensor *battery_level() { return this->battery_level_; }
            sensor::Sensor *temperature() { return this->temperature_; }


        protected:
            sensor::Sensor *battery_level_{nullptr};
            sensor::Sensor *temperature_{nullptr};
        };

    } // namespace danfoss_eco
} // namespace esphome