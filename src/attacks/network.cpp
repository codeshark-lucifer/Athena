#include "attacks/network.h"
#include "console/command.h"

#include <WiFi.h>
#include <esp_wifi.h>

void scan_network(Stream &out)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        out.println("No networks found.");
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            AccessPoint ap;
            ap.idx = i;
            ap.ssid = WiFi.SSID(i);
            ap.rssi = WiFi.RSSI(i); // dBm
            const char *security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "[OPEN]" : "[SECURED]";

            context.access_points.push_back(ap);

            char buf[128];
            snprintf(buf, sizeof(buf), "%d. %s (%d dBm) %s", i, ap.ssid.c_str(), static_cast<int>(ap.rssi), security);
            out.println(buf);
            delay(10);
        }
    }
}