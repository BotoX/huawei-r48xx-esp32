#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>

int hex2bytes(const char *str, uint8_t *bytes, int32_t length);
void bytes2hex(const uint8_t *bytes, int32_t length, char *str, int32_t strLength);

#endif
