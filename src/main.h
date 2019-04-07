#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#define POWER_EN_GPIO 15
#define MAX_SRV_CLIENTS 4

namespace Main {

enum {
    HWSERIAL = 0,
    BTSERIAL = 1,
    TCPSERIAL = 2,
    NUM_CHANNELS
};

void onCANReceive(int packetSize);
void init();
void loop();

Stream* channel(int num = -1);

extern int g_CurrentChannel;
extern bool g_Debug[NUM_CHANNELS];
extern char g_SerialBuffer[NUM_CHANNELS][255];
extern int g_SerialBufferPos[NUM_CHANNELS];

};

#endif
