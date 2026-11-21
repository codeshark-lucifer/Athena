#pragma once
#include <Arduino.h>

typedef struct
{
    uint8_t idx;
    String ssid;
    int32_t rssi;
} AccessPoint;

void scan_network(Stream &out);