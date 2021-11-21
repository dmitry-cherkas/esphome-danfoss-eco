#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
namespace esphome
{
    namespace danfoss_eco
    {
        static const char *const TAG = "danfoss_eco";
        static const uint16_t NAN_HANDLE = -1;

        static uint8_t *parse_hex_str(const char *value, size_t str_len)
        {
            size_t len = str_len / 2;
            uint8_t *buff = (uint8_t *)malloc(sizeof(uint8_t) * len);
            for (size_t i = 0; i < len; i++)
                buff[i] = (parse_hex(value[i * 2]).value() << 4) | parse_hex(value[i * 2 + 1]).value();

            return buff;
        }

        static int parse_int(uint8_t *data, int start_pos)
        {
            return int(data[start_pos] << 24 | data[start_pos + 1] << 16 | data[start_pos + 2] << 8 | data[start_pos + 3]);
        }

        static bool parse_bit(uint8_t value, int pos)
        {
            return (value & (1 << pos)) >> pos;
        }

        static void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buf)
        {
            for (int i = 0; i < len; i += 4)
            {
                int l = MIN(4, len - i); // limit for a chunk, 4 or what's left
                for (int j = 0; j < l; j++)
                {
                    reversed_buf[i + j] = data[i + (l - 1 - j)];
                }
            }
        }
    }
}