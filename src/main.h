#ifndef MAIN_H
#define MAIN_H

#define POWER_EN_GPIO 15

namespace Main {

void onCANReceive(int packetSize);
void processSerial();

extern bool g_Debug;
extern char g_SerialBuffer[255];
extern int g_SerialBufferPos;

};

#endif
