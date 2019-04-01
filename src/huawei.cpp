#include <Arduino.h>
#include <CAN.h>

#include "utils.h"
#include "main.h"
#include "huawei.h"

namespace Huawei {

uint16_t s_UserVoltage = 0x00;
uint16_t s_UserCurrent = 0x00;

void OnRecvCAN(uint32_t msgid, uint8_t *data, uint8_t length)
{
    if(Main::g_Debug)
    {
        Serial.printf("%X:", msgid);
        for(uint8_t i = 0; i < length; i++)
        {
            Serial.printf(" %X", data[i]);
        }
        Serial.println();
    }
}

void SendCAN(uint32_t msgid, uint8_t *data, uint8_t length)
{
    CAN.beginExtendedPacket(msgid);
    CAN.write(data, length);
    CAN.endPacket();
}

void SetReg(uint8_t reg, uint16_t val)
{
    static const uint32_t msgid = 0x108180fe;
    static uint8_t data[8];

    data[0] = 0x01;
    data[1] = reg;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = (val >> 8) & 0xFF;
    data[7] = val & 0xFF;

    SendCAN(msgid, data, sizeof(data));
}

void SetVoltageHex(uint16_t hex, bool perm)
{
    const uint8_t reg = perm ? 0x01 : 0x00;

    if(hex < 0xA600)
        hex = 0xA600;
    if(hex > 0xEA00)
        hex = 0xEA00;

    if(perm)
    {
        if(hex < 0xC000)
            hex = 0xC000;
        if(hex > 0xE99A)
            hex = 0xE99A;
    }
    else
        s_UserVoltage = hex;

    SetReg(reg, hex);
}

void SetVoltage(float u, bool perm)
{
    // calibration, non-linearity measured on my own PSU
    u += (u - 49.0) / 110.0;

    if(u < 40.0)
        u = 40.0;
    if(u > 60.0)
        u = 60.0;

    uint16_t hex = u * 1020;
    SetVoltageHex(hex, perm);
}

void SetCurrentHex(uint16_t hex, bool perm)
{
    const uint8_t reg = perm ? 0x04 : 0x03;

    if(hex > 0x0420)
        hex = 0x0420;

    if(!perm)
        s_UserCurrent = hex;

    SetReg(reg, hex);
}

void SetCurrent(float i, bool perm)
{
    uint16_t hex = i * 30;

    SetCurrentHex(hex, perm);
}

}
