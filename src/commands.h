#ifndef COMMANDS_H
#define COMMANDS_H

namespace Commands {

int parseLine(char *line);

struct CommandEntry
{
    const char *cmd;
    int (*fun)(int argc, char *argv[]);
    const char *help;
};

extern CommandEntry g_Commands[];

}



#endif
