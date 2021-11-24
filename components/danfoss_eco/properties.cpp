#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"
#include "properties.h"
#include "helpers.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco
    {
        climate::ClimateMode from_device_mode(DeviceMode schedule_mode)
        {
            switch (schedule_mode)
            {
            case DeviceMode::MANUAL:
                return climate::ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

            case DeviceMode::VACATION:
            case DeviceMode::SCHEDULED:
                return climate::ClimateMode::CLIMATE_MODE_AUTO;

            case DeviceMode::HOLD:
                return climate::ClimateMode::CLIMATE_MODE_HEAT; // TODO: or OFF, depending on current vs target temperature?

            default:
                ESP_LOGW(TAG, "unexpected schedule_mode: %d", schedule_mode);
                return climate::ClimateMode::CLIMATE_MODE_HEAT; // unknown
            }
        }

        bool DeviceProperty::init_handle(esphome::ble_client::BLEClient *client)
        {
            ESP_LOGD(TAG, "[%s] resolving handler for service=%s, characteristic=%s", client->address_str().c_str(), this->service_uuid.to_string().c_str(), this->characteristic_uuid.to_string().c_str());
            auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
            if (chr == nullptr)
            {
                ESP_LOGW(TAG, "[%s] characteristic uuid=%s not found", client->address_str().c_str(), this->characteristic_uuid.to_string().c_str());
                return false;
            }

            this->handle = chr->handle;
            return true;
        }

    } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32