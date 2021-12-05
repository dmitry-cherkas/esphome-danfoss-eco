#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
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
                LOG_CLIMATE("", "Danfoss Eco eTRV", this);
                LOG_SENSOR("", "Battery Level", this->battery_level_);
                LOG_SENSOR("", "Room Temperature", this->temperature_);
                LOG_BINARY_SENSOR("", "Problems", this->problems_);
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
            void set_problems(binary_sensor::BinarySensor *problems) { problems_ = problems; }

            sensor::Sensor *battery_level() { return this->battery_level_; }
            sensor::Sensor *temperature() { return this->temperature_; }
            binary_sensor::BinarySensor *problems() { return this->problems_; }

        protected:
            sensor::Sensor *battery_level_{nullptr};
            sensor::Sensor *temperature_{nullptr};
            binary_sensor::BinarySensor *problems_{nullptr};
        };

    } // namespace danfoss_eco
} // namespace esphome