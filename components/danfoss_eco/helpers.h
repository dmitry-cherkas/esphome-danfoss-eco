#pragma once

namespace esphome
{
    namespace danfoss_eco
    {
        const char *const TAG = "danfoss_eco";
        const uint16_t NAN_HANDLE = -1;

        uint8_t *parse_hex_str(const char *value, size_t str_len);
        int parse_int(uint8_t *data, int start_pos);
        bool parse_bit(uint8_t value, int pos);

        void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buf);
    }
}