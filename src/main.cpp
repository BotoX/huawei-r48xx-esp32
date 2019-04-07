#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <CAN.h>
#include <BluetoothSerial.h>

#include "huawei.h"
#include "commands.h"
#include "main.h"

WiFiServer server(23);
WiFiClient serverClient;

BluetoothSerial SerialBT;

const char g_WIFI_SSID[] = "";
const char g_WIFI_Passphrase[] = "";

namespace Main
{

int g_CurrentChannel;
bool g_Debug[NUM_CHANNELS];
char g_SerialBuffer[NUM_CHANNELS][255];
int g_SerialBufferPos[NUM_CHANNELS];

unsigned long g_Time1000;


void onCANReceive(int packetSize)
{
    if(!CAN.packetExtended())
        return;
    if(CAN.packetRtr())
        return;

    uint32_t msgid = CAN.packetId();

    uint8_t data[packetSize];
    CAN.readBytes(data, sizeof(data));

    Huawei::onRecvCAN(msgid, data, packetSize);
}

void init()
{
    Serial.begin(115200);
    while(!Serial);
    Serial.println("BOOTED!");

    // PSU enable pin
    pinMode(POWER_EN_GPIO, OUTPUT_OPEN_DRAIN);
    digitalWrite(POWER_EN_GPIO, 0); // Default = ON

    WiFi.setHostname("ESP32-R4830G2");
    if(!WiFi.begin(g_WIFI_SSID, g_WIFI_Passphrase))
        Serial.println("WiFi config error!");
    else {
        WiFi.setAutoConnect(true);
    }

    SerialBT.begin("ESP32-R4830G2");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    })
    .onEnd([]() {
        Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();

    server.begin();
    server.setNoDelay(true);

    if(!CAN.begin(125E3)) {
        Serial.println("Starting CAN failed!");
        while(1);
    }

    // crashes when calling some functions inside interrupt
    //CAN.onReceive(onCANReceive);
}

Stream* channel(int num)
{
    if(num == -1)
        num = g_CurrentChannel;

    if(num == BTSERIAL && SerialBT.hasClient())
        return &SerialBT;
    else if(num == TCPSERIAL && serverClient)
        return &serverClient;

    return &Serial;
}

void loop()
{
    int packetSize = CAN.parsePacket();
    if(packetSize)
        onCANReceive(packetSize);

    ArduinoOTA.handle();

    if(server.hasClient())
    {
        if(serverClient) // disconnect current client if any
            serverClient.stop();
        serverClient = server.available();
    }
    if(!serverClient)
        serverClient.stop();

    for(int i = 0; i < NUM_CHANNELS; i++)
    {
        while(channel(i)->available())
        {
            g_CurrentChannel = i;
            int c = channel(i)->read();
            if(c == '\r' || c == '\n' || g_SerialBufferPos[i] == sizeof(*g_SerialBuffer))
            {
                g_SerialBuffer[i][g_SerialBufferPos[i]] = 0;

                if(g_SerialBufferPos[i])
                    Commands::parseLine(g_SerialBuffer[i]);

                g_SerialBufferPos[i] = 0;
                continue;
            }
            g_SerialBuffer[i][g_SerialBufferPos[i]] = c;
            ++g_SerialBufferPos[i];
        }
    }

    if((millis() - g_Time1000) > 1000)
    {
        Huawei::every1000ms();
        g_Time1000 = millis();
    }
}

}

void setup()
{
    Main::init();
}

void loop()
{
    Main::loop();
}
