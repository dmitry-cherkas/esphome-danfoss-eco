#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco_scanner
    {
        using namespace std;
        using namespace esphome::esp32_ble_tracker;

        static auto DANFOSS_UUID = ESPBTUUID::from_uint16(0x042f);
        const char *const TAG = "danfoss_eco_scanner";

        class DanfossEcoScanner : public ESPBTDeviceListener, public Component
        {
        public:
            void dump_config() override;
            float get_setup_priority() const override { return setup_priority::DATA; }

            bool parse_device(const ESPBTDevice &device) override;

            void set_read_secret(bool read_secret) { this->read_secret_ = read_secret; }

        private:
            bool read_secret_{false};
        };

    } // namespace danfoss_eco_scanner
} // namespace esphome

#endif
