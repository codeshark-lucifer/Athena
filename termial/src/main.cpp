#include <stdio.h>
#include <windows.h>
#include "console/serial.h"

static int split(char *str, char *args[], int max_args)
{
    int count = 0;
    char *context = NULL;
    char *token = strtok_s(str, " \t", &context);

    while (token != NULL && count < max_args - 1)
    {
        args[count++] = token;
        token = strtok_s(NULL, " \t", &context);
    }
    args[count] = NULL;
    return count;
}

int main(int argc, char *args[])
{
    char input[256];
    char rawCmd[256]; // Preserve unsplit copy to send over serial
    char *arg_s[64];

    printf("ESP32-C6 RISC-V Console Bridge\n");
    printf("Commands: /connect COMx | /disconnect | /help | /exit | <esp32_cmd>\n\n");

    if (argc >= 2)
    {
        DWORD baud = (argc >= 3) ? (DWORD)strtoul(args[2], NULL, 10) : CBR_115200;
        connect_serial(args[1], baud > 0 ? baud : CBR_115200);
    }

    while (true)
    {
        printf("~/ ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }
        input[strcspn(input, "\r\n")] = '\0';

        // Trim leading whitespace
        char *trimmed = input;
        while (*trimmed == ' ' || *trimmed == '\t')
        {
            trimmed++;
        }
        if (strlen(trimmed) == 0)
            continue;

        // Copy before strtok mutates `trimmed`
        snprintf(rawCmd, sizeof(rawCmd), "%s", trimmed);

        int arg_c = split(trimmed, arg_s, 64);
        if (arg_c == 0)
            continue;

        if (strcmp(arg_s[0], "/exit") == 0 || strcmp(arg_s[0], "/quit") == 0)
        {
            break;
        }
        else if (strcmp(arg_s[0], "/help") == 0)
        {
            printf("\n");
            printf("  /\\_/\\\\   Welcome to the tiny ESP32 console!\n");
            printf(" ( o.o )\n");
            printf("  > ^ <\n");
            printf("  +------------------------------------------+\n");
            printf("  | /connect COM3 [baud]  Connect (default 115200)\n");
            printf("  | /disconnect           Close the serial link\n");
            printf("  | /help                 Show this little guide\n");
            printf("  | /exit or /quit        Leave the console\n");
            printf("  | Anything else         Send it to the ESP32\n");
            printf("  +------------------------------------------+\n\n");
        }
        else if (strcmp(arg_s[0], "/connect") == 0)
        {
            if (arg_c < 2)
            {
                printf("Usage: /connect COM3 [baud]\n");
            }
            else
            {
                DWORD baud = (arg_c >= 3) ? (DWORD)strtoul(arg_s[2], NULL, 10) : CBR_115200;
                connect_serial(arg_s[1], baud > 0 ? baud : CBR_115200);
            }
        }
        else if (strcmp(arg_s[0], "/disconnect") == 0)
        {
            if (hSerial != INVALID_HANDLE_VALUE)
            {
                close_serial();
                printf("[OK] Disconnected.\n");
            }
            else
            {
                printf("[Info] Not currently connected.\n");
            }
        }
        else if (arg_s[0][0] == '/')
        {
            printf("[Warning] Unknown local command: %s (type /help for usage)\n", arg_s[0]);
        }
        else
        {
            send_and_receive(rawCmd);
        }
    }

    close_serial();
    return EXIT_SUCCESS;
}