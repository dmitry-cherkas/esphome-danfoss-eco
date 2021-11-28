#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/climate/climate.h"

#include "helpers.h"

namespace esphome
{
    namespace danfoss_eco
    {
        using namespace esphome::climate;

        class MyComponent : public Climate, public PollingComponent
        {
        public:
            float get_setup_priority() const override { return setup_priority::DATA; }

            void dump_config() override
            {
                LOG_CLIMATE(TAG, "Danfoss Eco eTRV", this);
                LOG_SENSOR(TAG, "Battery Level", this->battery_level_);
                LOG_SENSOR(TAG, "Room Temperature", this->temperature_);
            }

            ClimateTraits traits() override
            {
                auto traits = ClimateTraits();
                traits.set_supports_current_temperature(true);

                traits.set_supported_modes(std::set<ClimateMode>({ClimateMode::CLIMATE_MODE_HEAT, ClimateMode::CLIMATE_MODE_AUTO}));
                traits.set_visual_temperature_step(0.5);

                traits.set_supports_current_temperature(true); // supports reporting current temperature
                traits.set_supports_action(true);              // supports reporting current action
                return traits;
            }

            void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }
            void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }

            sensor::Sensor *battery_level() { return this->battery_level_; }
            sensor::Sensor *temperature() { return this->temperature_; }

        protected:
            sensor::Sensor *battery_level_{nullptr};
            sensor::Sensor *temperature_{nullptr};
            // TODO add frost_protection_temperature sensor (OR CONTROL?)
            // TODO add vacation_temperature sensor (OR CONTROL?)
        };

    } // namespace danfoss_eco
} // namespace esphome