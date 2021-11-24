#include "esphome/core/log.h"
#include "properties.h"
#include "helpers.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco
    {
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