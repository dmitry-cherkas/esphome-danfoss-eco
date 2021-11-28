#pragma once

#include <xxtea-lib.h>

// key Size is always fixed
#define MAX_XXTEA_KEY8 16
// 32 Bit
#define MAX_XXTEA_KEY32 4
// DWORD Size of Data Buffer
#define MAX_XXTEA_DATA32 (UINT32CALCBYTE(MAX_XXTEA_DATA8))

class Xxtea
{
public:
    Xxtea(uint8_t *key, size_t len)
    {
        status_ = XXTEA_STATUS_GENERAL_ERROR;
        size_t osz;

        do
        {
            // Parameter Check
            if (key == NULL || len <= 0 || len > MAX_XXTEA_KEY8)
            {
                status_ = XXTEA_STATUS_PARAMETER_ERROR;
                break;
            }

            osz = UINT32CALCBYTE(len);

            // Check for Size Errors
            if (osz > MAX_XXTEA_KEY32)
            {
                status_ = XXTEA_STATUS_SIZE_ERROR;
                break;
            }

            // Clear the Key
            memset((void *)this->xxtea_key, 0, MAX_XXTEA_KEY8);

            // Copy the Key from Buffer
            memcpy((void *)this->xxtea_key, (const void *)key, len);

            // We have Success
            status_ = XXTEA_STATUS_SUCCESS;
        } while (0);
    }

    int encrypt(uint8_t *data, size_t len, uint8_t *buf, size_t *maxlen);
    int decrypt(uint8_t *data, size_t len);
    
    int status() { return this->status_; }

private:
    int status_;
    uint32_t xxtea_data[MAX_XXTEA_DATA32];
    uint32_t xxtea_key[MAX_XXTEA_KEY32];
};