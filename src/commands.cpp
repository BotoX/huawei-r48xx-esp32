#include <Arduino.h>

#include "utils.h"
#include "main.h"
#include "huawei.h"
#include "commands.h"

namespace Commands {

int parseLine(char *line)
{
    const int MAX_ARGV = 16;
    char *lineStart = line;
    char *argv[MAX_ARGV];
    int argc = 0;
    bool found = false;
    bool end = false;
    bool inString = false;

    while(1)
    {
		if(*line == '"' && (line == lineStart || *(line - 1) != '\\'))
		{
			inString ^= true;
			if(!inString)
			{
                found = true;
				end = true;
				lineStart++;
			}
		}

        if(!inString && *line == ' ')
            end = true;

        if(end || !*line)
        {
            if(found && argc < MAX_ARGV)
                argv[argc++] = lineStart;

            found = false;
            end = false;

            if(!*line)
                break;

            *line = 0;
            lineStart = ++line;
            continue;
        }

        line++;
        found = true;
    }

    if(!argc)
        return -1;

    for(int i = 0; g_Commands[i].cmd; i++)
    {
        if(strcmp(g_Commands[i].cmd, argv[0]) == 0)
        {
            int ret = g_Commands[i].fun(argc, argv);
            return ret;
        }
    }

    return -1;
}

int CMD_help(int argc, char **argv)
{
    Serial.println("Available Commands\n------------------");

    int i = 0;
    while(g_Commands[i].cmd)
    {
        Serial.printf("%17s %s\n", g_Commands[i].cmd, g_Commands[i].help);
        i++;
    }
    Serial.println();

    return 0;
}

int CMD_debug(int argc, char **argv)
{
    if(argc != 2) {
        Serial.println("Usage: debug <0|1>");
        return 1;
    }

    Main::g_Debug = strtoul(argv[1], NULL, 10);

    return 0;
}

int CMD_voltage(int argc, char **argv)
{
    if(argc < 2 || argc > 3) {
        Serial.println("Usage: voltage <volts> [perm]");
        return 1;
    }

    char *end = argv[1];
    float u = strtof(end, &end);

    bool perm = false;
    if(argc == 3) {
        end = argv[2];
        if(strtoul(end, NULL, 10))
            perm = true;
    }

    Huawei::SetVoltage(u, perm);

    return 0;
}

int CMD_current(int argc, char **argv)
{
    if(argc < 2 || argc > 3) {
        Serial.println("Usage: current <amps> [perm]");
        return 1;
    }

    char *end = argv[1];
    float i = strtof(end, &end);

    bool perm = false;
    if(argc == 3) {
        end = argv[2];
        if(strtoul(end, &end, 10))
            perm = true;
    }

    Huawei::SetCurrent(i, perm);

    return 0;
}

int CMD_can(int argc, char **argv)
{
    if(argc != 3) {
        Serial.println("Usage: can <msgid> <hex>");
        return 1;
    }

    uint32_t msgid = strtoul(argv[1], NULL, 16);
    uint8_t data[8];

    int len = hex2bytes(argv[2], data, sizeof(data));
    if(len <= 0)
        return 1;

    Huawei::SendCAN(msgid, data, len);

    return 0;
}

int CMD_onoff(int argc, char **argv)
{
    if(argc != 2) {
        Serial.println("Usage: onoff <0|1>");
        return 1;
    }

    int val = !strtoul(argv[1], NULL, 10);
    digitalWrite(POWER_EN_GPIO, val);

    return 0;
}


CommandEntry g_Commands[] =
{
    {"help",     CMD_help,      " : Display list of commands"},
    {"debug",    CMD_debug,     " : Debug <0|1>"},
    {"voltage",  CMD_voltage,   " : voltage <volts> [perm]"},
    {"current",  CMD_current,   " : current <amps> [perm]"},
    {"can",      CMD_can,       " : can <msgid> <hex>"},
    {"onoff",    CMD_onoff,     " : onoff <0|1>"},
    { 0, 0, 0 }
};

}
