#pragma once

#include "xxtea.h"

#include <esp_bt_defs.h>

namespace esphome
{
    namespace danfoss_eco
    {
        using namespace std;

        const char *const TAG = "danfoss_eco";

        void encode_hex(const uint8_t *data, size_t len, char *buff);
        void parse_hex_str(const char *data, size_t str_len, uint8_t *buff);
        uint32_t parse_int(uint8_t *data, int start_pos);
        uint16_t parse_short(uint8_t *data, int start_pos);
        void write_int(uint8_t *data, int start_pos, int value);

        bool parse_bit(uint8_t data, int pos);
        bool parse_bit(uint16_t data, int pos);
        void set_bit(uint8_t data, int pos, bool value);

        void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buff);

        uint8_t *encrypt(shared_ptr<Xxtea> &xxtea, uint8_t *value, uint16_t value_len);
        uint8_t *decrypt(shared_ptr<Xxtea> &xxtea, uint8_t *value, uint16_t value_len);

        void copy_address(uint64_t, esp_bd_addr_t);
    }
}