#include "console/serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <string>

// #include <windows.h>

HANDLE hSerial = INVALID_HANDLE_VALUE;

void close_serial()
{
    if (hSerial != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
}

static void drain_peding_rx(DWORD max_duration_ms)
{
    if (hSerial == INVALID_HANDLE_VALUE)
        return;

    ULONGLONG deadline = GetTickCount64() + max_duration_ms;
    char discard[256];
    while (GetTickCount64() < deadline)
    {
        DWORD bytesRead = 0;
        if (!ReadFile(hSerial, discard, sizeof(discard), &bytesRead, NULL) || bytesRead == 0)
        {
            break;
        }
    }
    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

bool connect_serial(const char *portName, DWORD baudRate)
{
    close_serial();

    char fullPortName[64];
    if (strncmp(portName, "\\\\.\\", 4) == 0)
    {
        snprintf(fullPortName, sizeof(fullPortName), "%s", portName);
    }
    else
    {
        // Prefix with \\.\ to support COM10 and higher
        snprintf(fullPortName, sizeof(fullPortName), "\\\\.\\%s", portName);
    }

    hSerial = CreateFileA(
        fullPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
        {
            printf("[Error] Failed to open %s (Code: 5 - Port is in use by another program)\n", portName);
        }
        else if (err == ERROR_FILE_NOT_FOUND)
        {
            printf("[Error] Failed to open %s (Code: 2 - Port not found)\n", portName);
        }
        else
        {
            printf("[Error] Failed to open %s (Code: %lu)\n", portName, err);
        }
        return false;
    }

    SetupComm(hSerial, 4096, 4096);

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        printf("[Error] GetCommState failed (Code: %lu).\n", GetLastError());
        close_serial();
        return false;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fBinary = TRUE;
    dcbSerialParams.fParity = FALSE;
    dcbSerialParams.fOutxCtsFlow = FALSE;
    dcbSerialParams.fOutxDsrFlow = FALSE;
    // Keep DTR and RTS deasserted so ESP32-C6 auto-reset / GPIO9 boot logic stays idle
    dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
    dcbSerialParams.fDsrSensitivity = FALSE;
    dcbSerialParams.fTXContinueOnXoff = TRUE;
    dcbSerialParams.fOutX = FALSE;
    dcbSerialParams.fInX = FALSE;
    dcbSerialParams.fErrorChar = FALSE;
    dcbSerialParams.fNull = FALSE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParams.fAbortOnError = FALSE;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        printf("[Error] SetCommState failed (Code: %lu).\n", GetLastError());
        close_serial();
        return false;
    }

    EscapeCommFunction(hSerial, CLRDTR);
    EscapeCommFunction(hSerial, CLRRTS);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 200;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    // Give the chip a brief moment if opening the port triggered a reset, then drain boot logs
    Sleep(150);
    drain_peding_rx(400);

    printf("[OK] Connected to %s at %lu baud.\n", portName, baudRate);
    return true;
}

void send_and_receive(const char *cmd)
{
    if (hSerial == INVALID_HANDLE_VALUE)
    {
        printf("[Warning] Not connected. Use: /connect COMx\n");
        return;
    }

    // Discard any stale or asynchronous bytes (such as ROM boot banners) before sending
    COMSTAT comStat = {0};
    DWORD commErrors = 0;
    if (ClearCommError(hSerial, &commErrors, &comStat) && comStat.cbInQue > 0)
    {
        PurgeComm(hSerial, PURGE_RXCLEAR);
    }

    char txBuffer[260];
    int len = snprintf(txBuffer, sizeof(txBuffer), "%s\n", cmd);
    if (len < 0 || len >= (int)sizeof(txBuffer))
    {
        printf("[Error] Command too long.\n");
        return;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(hSerial, txBuffer, (DWORD)len, &bytesWritten, NULL) || bytesWritten != (DWORD)len)
    {
        DWORD err = GetLastError();
        printf("[Error] WriteFile failed (Code: %lu). Disconnecting.\n", err);
        close_serial();
        return;
    }

    // The firmware marks the end of each response so long-running commands can finish.
    static const char completionMarker[] = "\x1e" "ATHENA_DONE" "\x1e";
    const ULONGLONG startTick = GetTickCount64();
    const ULONGLONG totalTimeoutMs = 30000;
    const size_t markerLength = sizeof(completionMarker) - 1;
    char rxChunk[512];
    std::string response;
    bool gotData = false;

    while ((GetTickCount64() - startTick) < totalTimeoutMs)
    {
        DWORD bytesRead = 0;
        if (!ReadFile(hSerial, rxChunk, sizeof(rxChunk) - 1, &bytesRead, NULL))
        {
            DWORD err = GetLastError();
            printf("[Error] ReadFile failed (Code: %lu). Disconnecting.\n", err);
            close_serial();
            return;
        }

        if (bytesRead > 0)
        {
            gotData = true;
            response.append(rxChunk, bytesRead);
            const size_t markerPos = response.find(completionMarker);
            if (markerPos != std::string::npos)
            {
                fwrite(response.data(), 1, markerPos, stdout);
                fflush(stdout);
                return;
            }

            const size_t safeLength = response.size() >= markerLength
                ? response.size() - markerLength + 1
                : 0;
            if (safeLength > 0)
            {
                fwrite(response.data(), 1, safeLength, stdout);
                fflush(stdout);
                response.erase(0, safeLength);
            }
        }
    }

    if (!gotData)
    {
        printf("[Timeout] No response from ESP32-C6.\n");
    }
    else
    {
        fwrite(response.data(), 1, response.size(), stdout);
        printf("\n[Timeout] Response did not complete within 30 seconds.\n");
    }
}