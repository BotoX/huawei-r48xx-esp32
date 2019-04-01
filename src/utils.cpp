#include <string.h>
#include "utils.h"

static inline uint8_t _char2byte(char c)
{
    if('0' <= c && c <= '9') return (uint8_t)(c - '0');
    if('A' <= c && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if('a' <= c && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFF;
}

int hex2bytes(const char *str, uint8_t *bytes, int32_t length)
{
    int result;
    if(!str || !bytes || length <= 0)
        return -1;

    for(result = 0; *str; result++)
    {
        uint8_t msn = _char2byte(*str++);
        if(msn == 0xFF) return -1;

        uint8_t lsn = _char2byte(*str++);
        if(lsn == 0xFF) return -1;

        uint8_t bin = (msn << 4) + lsn;

        if(length-- <= 0)
            return -1;

        *bytes++ = bin;
    }

    return result;
}

void bytes2hex(const uint8_t *bytes, int32_t length, char *str, int32_t strLength)
{
    const char binHex[] = "0123456789ABCDEF";

    if(!str || strLength < 3)
        return;
    *str = 0;

    if(!bytes || length <= 0 || strLength <= 2 * length)
    {
        strncpy(str, "ERR", strLength);
        return;
    }

    for(; length > 0; length--, strLength -= 2)
    {
        uint8_t byte = *bytes++;

        *str++ = binHex[(byte >> 4) & 0x0F];
        *str++ = binHex[byte & 0x0F];
    }

    if(strLength-- <= 0)
        return;

    *str++ = 0;
}
