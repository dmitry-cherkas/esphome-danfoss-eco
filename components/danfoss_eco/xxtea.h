#pragma once

#include <xxtea-lib.h>

// key Size is always fixed
#define MAX_XXTEA_KEY8 16
// 32 Bit
#define MAX_XXTEA_KEY32 4
// DWORD Size of Data Buffer
#define MAX_XXTEA_DATA32 (UINT32CALCBYTE(MAX_XXTEA_DATA8))

#define XXTEA_STATUS_NOT_INITIALIZED -1

class Xxtea
{
public:
    Xxtea() : status_(XXTEA_STATUS_NOT_INITIALIZED){};

    int set_key(uint8_t *key, size_t len);

    int encrypt(uint8_t *data, size_t len, uint8_t *buf, size_t *maxlen);
    int decrypt(uint8_t *data, size_t len);

    int status() { return this->status_; }

private:
    int status_;
    uint32_t xxtea_data[MAX_XXTEA_DATA32];
    uint32_t xxtea_key[MAX_XXTEA_KEY32];
};