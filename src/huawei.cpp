#include <Arduino.h>
#include <CAN.h>

#include "utils.h"
#include "main.h"
#include "huawei.h"

namespace Huawei {

bool g_Ready = false;
uint16_t g_UserVoltage = 0x00;
uint16_t g_UserCurrent = 0x00;
float g_Current = 0.0;
float g_CoulombCounter = 0.0;
HuaweiInfo g_PSU;

void onRecvCAN(uint32_t msgid, uint8_t *data, uint8_t length)
{
    HuaweiEAddr eaddr = HuaweiEAddr::unpack(msgid);

    switch(eaddr.cmdId)
    {
        case HUAWEI_R48XX_MSG_CURRENT_ID: {
            if(!data[3])
                g_Ready = true;
            g_Current = __builtin_bswap16(*(uint16_t *)&data[6]) / 30.0;
            if(!eaddr.fromSrc)
                g_CoulombCounter += g_Current * 0.377; // every 377ms
        } return;

        case HUAWEI_R48XX_MSG_DATA_ID: {
            uint16_t id = __builtin_bswap16(*(uint16_t *)&data[0]);
            uint32_t val = __builtin_bswap32(*(uint32_t *)&data[4]);
            id &= ~0x3000;

            switch(id)
            {
                case 0x0170: {
                    g_PSU.input_power = val / 1024.0;
                } return;
                case 0x0171: {
                    g_PSU.input_freq = val / 1024.0;
                } return;
                case 0x0172: {
                    g_PSU.input_current = val / 1024.0;
                } return;
                case 0x0173: {
                    g_PSU.output_power = val / 1024.0;
                } return;
                case 0x0174: {
                    g_PSU.efficiency = val / 1024.0;
                } return;
                case 0x0175: {
                    g_PSU.output_voltage = val / 1024.0;
                } return;
                case 0x0176: {
                    g_PSU.output_current_max = val / 30.0;
                } return;
                case 0x0178: {
                    g_PSU.input_voltage = val / 1024.0;
                } return;
                case 0x017F: {
                    g_PSU.output_temp = val / 1024.0;
                } return;
                case 0x0180: {
                    g_PSU.input_temp = val / 1024.0;
                } return;
                case 0x0181:
                case 0x0182: {
                    g_PSU.output_current = val / 1024.0;
                } return;
            }
        } return;

        case HUAWEI_R48XX_MSG_INFO_ID: {
            if(data[1] == 1)
                Main::channel()->println("--- HUAWEI R48XX INFO ---");

            switch(data[1]) {
                case 1: {
                    uint32_t val = __builtin_bswap32(*(uint32_t *)&data[4]);
                    uint16_t rated_current = ((val >> 16) & 0x03FF) >> 1;
                    printf("Rated Current: %u\n", rated_current);
                } return;
            }
        } break;

        case HUAWEI_R48XX_MSG_DESC_ID: {
            if(data[1] == 1)
                Main::channel()->println("--- HUAWEI R48XX DESCRIPTION ---");

            Main::channel()->write(&data[2], 6);

            if(!eaddr.count)
                Main::channel()->println();
        } return;
    }

    // Debug
    for(int c = 0; c < Main::NUM_CHANNELS; c++)
    {
        if(Main::g_Debug[c])
        {
            Main::channel(c)->printf("%08X:", msgid);
            for(uint8_t i = 0; i < length; i++)
            {
                Main::channel(c)->printf(" %02X", data[i]);
            }
            Main::channel(c)->println();
        }
    }
}

void every1000ms()
{
    if(g_Ready)
        sendGetData();
/*
    static uint8_t count10 = 0;
    if(count10 == 10)
    {
        if(g_UserVoltage)
            setVoltageHex(g_UserVoltage, false);
        if(g_UserCurrent)
            setCurrentHex(g_UserCurrent, false);
    }
    count10++;
*/
}

void sendCAN(uint32_t msgid, uint8_t *data, uint8_t length, bool rtr)
{
    CAN.beginExtendedPacket(msgid, -1, rtr);
    CAN.write(data, length);
    CAN.endPacket();
}

void setReg(uint8_t reg, uint16_t val)
{
    HuaweiEAddr eaddr = {HUAWEI_R48XX_PROTOCOL_ID, 0x00, HUAWEI_R48XX_MSG_CONTROL_ID, 0x01, 0x3F, 0x00};
    uint8_t data[8];
    data[0] = 0x01;
    data[1] = reg;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = (val >> 8) & 0xFF;
    data[7] = val & 0xFF;

    sendCAN(eaddr.pack(), data, sizeof(data));
}

void setVoltageHex(uint16_t hex, bool perm)
{
    uint8_t reg = perm ? 0x01 : 0x00;

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
        g_UserVoltage = hex;

    setReg(reg, hex);
}

void setVoltage(float u, bool perm)
{
    // calibration, non-linearity measured on my own PSU
    u += (u - 49.0) / 110.0;

    if(u < 40.0)
        u = 40.0;
    if(u > 60.0)
        u = 60.0;

    uint16_t hex = u * 1020.0;
    setVoltageHex(hex, perm);
}

void setCurrentHex(uint16_t hex, bool perm)
{
    uint8_t reg = perm ? 0x04 : 0x03;

    if(hex > 0x0499)
        hex = 0x0499;

    if(!perm)
        g_UserCurrent = hex;

    setReg(reg, hex);
}

void setCurrent(float i, bool perm)
{
    uint16_t hex = i * 30.0;

    setCurrentHex(hex, perm);
}

void sendGetData()
{
    HuaweiEAddr eaddr = {HUAWEI_R48XX_PROTOCOL_ID, 0x00, HUAWEI_R48XX_MSG_DATA_ID, 0x01, 0x3F, 0x00};
    uint8_t data[8] = {0x00};
    sendCAN(eaddr.pack(), data, sizeof(data));
}

void sendGetInfo()
{
    HuaweiEAddr eaddr = {HUAWEI_R48XX_PROTOCOL_ID, 0x00, HUAWEI_R48XX_MSG_INFO_ID, 0x01, 0x3F, 0x00};
    uint8_t data[8] = {0x00};
    sendCAN(eaddr.pack(), data, sizeof(data));
}

void sendGetDescription()
{
    HuaweiEAddr eaddr = {HUAWEI_R48XX_PROTOCOL_ID, 0x00, HUAWEI_R48XX_MSG_DESC_ID, 0x01, 0x3F, 0x00};
    uint8_t data[8] = {0x00};
    sendCAN(eaddr.pack(), data, sizeof(data));
}

HuaweiEAddr HuaweiEAddr::unpack(uint32_t val)
{
    HuaweiEAddr s;
    s.protoId = (val>>23) & 0x3F;
    s.addr = (val>>16) & 0x7F;
    s.cmdId = (val>>8) & 0xFF;
    s.fromSrc = (val>>7) & 0x01;
    s.rev = (val>>1) & 0x3F;
    s.count = val & 0x01;
    return s;
}

uint32_t HuaweiEAddr::pack()
{
    return protoId << 23 |
           addr << 16 |
           cmdId << 8 |
           fromSrc << 7 |
           rev << 1 |
           count;
}

}
