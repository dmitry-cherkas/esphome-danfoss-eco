#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "helpers.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
namespace esphome
{
    namespace danfoss_eco
    {

        void encode_hex(const uint8_t *data, size_t len, char *buff)
        {
            for (size_t i = 0; i < len; i++)
                sprintf(buff + (i * 2), "%02x", data[i]);
        }

        optional<int> parse_hex(const char chr)
        {
            int out = chr;
            if (out >= '0' && out <= '9')
                return (out - '0');
            if (out >= 'A' && out <= 'F')
                return (10 + (out - 'A'));
            if (out >= 'a' && out <= 'f')
                return (10 + (out - 'a'));
            return {};
        }

        void parse_hex_str(const char *data, size_t str_len, uint8_t *buff)
        {
            size_t len = str_len / 2;
            for (size_t i = 0; i < len; i++)
                buff[i] = (parse_hex(data[i * 2]).value() << 4) | parse_hex(data[i * 2 + 1]).value();
        }

        uint32_t parse_int(uint8_t *data, int start_pos)
        {
            return int(data[start_pos] << 24 | data[start_pos + 1] << 16 | data[start_pos + 2] << 8 | data[start_pos + 3]);
        }

        uint16_t parse_short(uint8_t *data, int start_pos)
        {
            return short(data[start_pos] << 8 | data[start_pos + 1]);
        }

        void write_int(uint8_t *data, int start_pos, int value)
        {
            data[start_pos] = value >> 24;
            data[start_pos + 1] = value >> 16;
            data[start_pos + 2] = value >> 8;
            data[start_pos + 3] = value;
        }

        bool parse_bit(uint8_t data, int pos) { return (data & (1 << pos)) >> pos; }

        bool parse_bit(uint16_t data, int pos) { return (data & (1 << pos)) >> pos; }

        void set_bit(uint8_t data, int pos, bool value)
        {
            data ^= (-value ^ data) & (1UL << pos);
        }

        void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buff)
        {
            for (int i = 0; i < len; i += 4)
            {
                int l = MIN(4, len - i); // limit for a chunk, 4 or what's left
                for (int j = 0; j < l; j++)
                {
                    reversed_buff[i + j] = data[i + (l - 1 - j)];
                }
            }
        }

        uint8_t *encrypt(shared_ptr<Xxtea> &xxtea, uint8_t *value, uint16_t value_len)
        {
            uint8_t buffer[value_len], enc_buff[value_len];
            reverse_chunks(value, value_len, buffer);

            auto xxtea_status = xxtea->encrypt(buffer, value_len, enc_buff, (size_t *)&value_len);
            if (xxtea_status != XXTEA_STATUS_SUCCESS)
                ESP_LOGW(TAG, "xxtea_encrypt failed, status=%d", xxtea_status);
            else
                reverse_chunks(enc_buff, value_len, value);
            return value;
        }

        uint8_t *decrypt(shared_ptr<Xxtea> &xxtea, uint8_t *value, uint16_t value_len)
        {
            uint8_t buffer[value_len];
            reverse_chunks(value, value_len, buffer);

            auto xxtea_status = xxtea->decrypt(buffer, value_len);
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