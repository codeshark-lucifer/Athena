#pragma once
#include <windows.h>

extern HANDLE hSerial;

bool connect_serial(const char *portName, DWORD baudRate = CBR_115200);

void send_and_receive(const char* cmd);

void close_serial();
