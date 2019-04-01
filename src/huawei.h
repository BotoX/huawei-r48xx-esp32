#ifndef HUAWEI_H
#define HUAWEI_H
#include <stdint.h>

namespace Huawei {

void OnRecvCAN(uint32_t msgid, uint8_t *data, uint8_t length);
void SendCAN(uint32_t msgid, uint8_t *data, uint8_t length);

void SetReg(uint8_t reg, uint16_t val);

void SetVoltageHex(uint16_t hex, bool perm);
void SetVoltage(float u, bool perm);

void SetCurrentHex(uint16_t hex, bool perm);
void SetCurrent(float i, bool perm);

}
#endif
