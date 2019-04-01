#include <Arduino.h>
#include <CAN.h>

#include "huawei.h"
#include "commands.h"
#include "main.h"

namespace Main
{

bool g_Debug = true;
char g_SerialBuffer[255];
int g_SerialBufferPos = 0;

void onCANReceive(int packetSize)
{
    if(!CAN.packetExtended())
        return;
    if(CAN.packetRtr())
        return;

    uint32_t msgid = CAN.packetId();

    uint8_t data[packetSize];
    CAN.readBytes(data, sizeof(data));

    Huawei::OnRecvCAN(msgid, data, packetSize);
}

void processSerial()
{
    while(Serial.available())
    {
        int c = Serial.read();
        if(c == '\r' || c == '\n' || g_SerialBufferPos == sizeof(g_SerialBuffer))
        {
            g_SerialBuffer[g_SerialBufferPos] = 0;

            if(g_SerialBufferPos)
                Commands::parseLine(g_SerialBuffer);

            g_SerialBufferPos = 0;
            continue;
        }
        g_SerialBuffer[g_SerialBufferPos] = c;
        ++g_SerialBufferPos;
    }
}

}

void setup()
{
    Serial.begin(115200);
    while(!Serial);

    pinMode(POWER_EN_GPIO, OUTPUT_OPEN_DRAIN);
    digitalWrite(POWER_EN_GPIO, 1);

    Serial.println("BOOTED!");

    if(!CAN.begin(125E3)) {
        Serial.println("Starting CAN failed!");
        while(1);
    }

    CAN.onReceive(Main::onCANReceive);
}

void loop()
{
    Main::processSerial();
}

