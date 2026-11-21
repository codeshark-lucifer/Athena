#include "console/command.h"
#include "config/config.h"
#include "drivers/led.h"

#include <cstdlib>
#include <cstring>

Context context;

void update_deauth()
{
    if (context.selected_access_points == -1)
        return;
    send_deauth(context.selected_access_points, DEAUTH_TYPE_SINGLE, 7);
}

static int splitArgs(char *str, char *args[], int maxArgs)
{
    int count = 0;
    char *token = strtok(str, " ,\t\r\n");
    while (token != nullptr && count < maxArgs - 1)
    {
        args[count++] = token;
        token = strtok(nullptr, " ,\t\r\n");
    }
    args[count] = nullptr;
    return count;
}

void handleCommand(char *line, Stream &out)
{
    char *argv[CMD_MAX_ARGS];
    int argc = splitArgs(line, argv, CMD_MAX_ARGS);
    if (argc > 0)
    {
        if (strcmp(argv[0], "ping") == 0)
        {
            out.println("PONG FROM ESP32");
        }
        else if (strcmp(argv[0], "led") == 0)
        {
            if (argc < 4)
            {
                out.println("Usage: led <r> <g> <b>");
            }
            else
            {
                int r = atoi(argv[1]);
                int g = atoi(argv[2]);
                int b = atoi(argv[3]);

                if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
                {
                    out.println("Out of range (0-255).");
                }
                else
                {
                    set_led(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
                    out.printf("LED set to R:%d G:%d B:%d\n", r, g, b);
                }
            }
        }
        else if (strcmp(argv[0], "scan") == 0)
        {
            out.println("Scanning network nearby...");
            out.flush();
            delay(10);

            scan_network(out);
        }
        else if (strcmp(argv[0], "select") == 0)
        {
            if (argc < 2)
            {
                out.println("Usage: select <idx>");
            }
            else if (atoi(argv[1]) > -1)
            {
                context.selected_access_points = atoi(argv[1]);
                out.printf("Select: %d\n", context.selected_access_points);
            }
            else
            {
                out.println("Outof range networks.");
            }
        }
        else if (strcmp(argv[0], "deauth") == 0)
        {
            if (argc < 2)
            {
                out.println("Usage: deauth <start/stop>");
            }
            else if (strcmp(argv[1], "start") == 0)
            {
                // start deauth
                out.println("START DEAUTH");
                context.active = true;
                context.on_update = update_deauth;
            }
            else if (strcmp(argv[1], "stop") == 0)
            {
                out.println("STOP DEAUTH");
                context.active = false;
                shutdown_deauth();
            }
            else
            {
                out.println("Usage: deauth <start/stop>");
            }
        }
        else
        {
            out.println("Invild Command!");
        }
    }

    out.write(static_cast<uint8_t>(0x1e));
    out.print("ATHENA_DONE");
    out.write(static_cast<uint8_t>(0x1e));
    out.flush();
}

void handle_perform_task()
{
    if (!context.active)
        return;

    context.on_update();

    set_led(30, 30, 0);
    delay(500);
    set_led(0, 0, 0);
    delay(500);
}