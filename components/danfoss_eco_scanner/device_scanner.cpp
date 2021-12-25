#include "esphome/core/log.h"

#include "device_scanner.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco_scanner
    {
        static const string eTRV_SUFFIX = string(";eTRV");

        void DanfossEcoScanner::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Danfoss Eco Scanner:");
            ESP_LOGCONFIG(TAG, "  Read Secret: %d", this->read_secret_);
        }

        bool DanfossEcoScanner::parse_device(const ESPBTDevice &device)
        {
            string name = device.get_name();
            int s_len = eTRV_SUFFIX.length();

            if (name.length() <= s_len || name.compare(name.length() - s_len, s_len, eTRV_SUFFIX) != 0)
                return false;

            ESP_LOGI(TAG, "Found Danfoss eTRV, MAC: %s, Name: %s", device.address_str().c_str(), name.c_str());

            uint8_t flags = (uint8_t)name.c_str()[0];
            if ((flags & 0x4) >> 2)
                ESP_LOGI(TAG, "Ready to read the secret key");

            return true;
        }

    } // namespace danfoss_eco_scanner
} // namespace esphome

#endif
