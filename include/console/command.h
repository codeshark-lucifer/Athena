#pragma once
#include <Arduino.h>
#include <vector>

#include "attacks/deauth.h"
#include "attacks/network.h"
using ActionFnPtr = void (*)();

typedef struct
{
    std::vector<AccessPoint> access_points;
    int selected_access_points;
    bool active;
    ActionFnPtr on_update;
} Context;
extern Context context;

void handleCommand(char *line, Stream &out);
void handle_perform_task();