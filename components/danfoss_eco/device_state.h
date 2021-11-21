#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace danfoss_eco
    {
        class DeviceState
        {
        public:
            bool is_connected() { return this->connected_; }
            bool set_connected(bool state) { return this->connected_ = state; }
            
            bool has_pending_state_request() { return this->has_pending_state_request_; }
            void set_pending_state_request(bool val) { this->has_pending_state_request_ = val; }

        protected:
            bool connected_;
            bool has_pending_state_request_;
        };
    }
}