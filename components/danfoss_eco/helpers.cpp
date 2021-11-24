#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "helpers.h"

#include <xxtea-lib.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
namespace esphome
{
    namespace danfoss_eco
    {
        uint8_t *parse_hex_str(const char *value, size_t str_len)
        {
            size_t len = str_len / 2;
            uint8_t *buff = (uint8_t *)malloc(sizeof(uint8_t) * len);
            for (size_t i = 0; i < len; i++)
                buff[i] = (parse_hex(value[i * 2]).value() << 4) | parse_hex(value[i * 2 + 1]).value();

            return buff;
        }

        int parse_int(uint8_t *data, int start_pos)
        {
            return int(data[start_pos] << 24 | data[start_pos + 1] << 16 | data[start_pos + 2] << 8 | data[start_pos + 3]);
        }

        bool parse_bit(uint8_t value, int pos)
        {
            return (value & (1 << pos)) >> pos;
        }

        void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buf)
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

        uint8_t *encrypt(uint8_t *value, uint16_t value_len)
        {
            uint8_t buffer[value_len], enc_buff[value_len];
            reverse_chunks(value, value_len, buffer);

            auto xxtea_status = xxtea_encrypt(buffer, value_len, enc_buff, (size_t *)&value_len);
            if (xxtea_status != XXTEA_STATUS_SUCCESS)
                ESP_LOGW(TAG, "xxtea_encrypt failed, status=%d", xxtea_status);
            else
                reverse_chunks(enc_buff, value_len, value);
            return value;
        }

        uint8_t *decrypt(uint8_t *value, uint16_t value_len)
        {
            uint8_t buffer[value_len];
            reverse_chunks(value, value_len, buffer);

            auto xxtea_status = xxtea_decrypt(buffer, value_len);
            if (xxtea_status != XXTEA_STATUS_SUCCESS)
                ESP_LOGW(TAG, "xxtea_decrypt failed, status=%d", xxtea_status);
            else
                reverse_chunks(buffer, value_len, value);
            return value;
        }

        void copy_address(uint64_t mac, esp_bd_addr_t bd_addr)
        {
            bd_addr[0] = (mac >> 40) & 0xFF;
            bd_addr[1] = (mac >> 32) & 0xFF;
            bd_addr[2] = (mac >> 24) & 0xFF;
            bd_addr[3] = (mac >> 16) & 0xFF;
            bd_addr[4] = (mac >> 8) & 0xFF;
            bd_addr[5] = (mac >> 0) & 0xFF;
        }

    }
}